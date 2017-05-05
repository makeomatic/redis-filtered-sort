-- cached id list key
local idListKey = KEYS[1];
-- groupBy column
local groupByKey = ARGV[1];
-- stringified [key]: [aggregateMethod] pairs
local aggregates = ARGV[2];
-- pagination offset
local offset = tonumber(ARGV[3] or 0);
-- item limit
local limit = tonumber(ARGV[4] or 10);

-- local cache
local rcall = redis.call;
local tinsert = table.insert;

local jsonAggregates = cjson.decode(aggregates);
local aggregateKeys = {};

for key, _ in pairs(jsonAggregates) do
  tinsert(aggregateKeys, key);
end

local function aggregateSum(value1, value2)
  value1 = value1 or 0;
  value2 = value2 or 0;

  return value1 + value2;
end

local aggregateType = {
  sum = aggregateSum
};

if rcall("EXISTS", idListKey) == 0 then
  return {0};
end

local valuesToGroup = rcall("LRANGE", idListKey, 0, -1);
local result = {};

-- group
for _, id in ipairs(valuesToGroup) do
  local values = rcall("HMGET", id, groupByKey, unpack(aggregateKeys));

  if result[values[1]] == nil then
    result[values[1]] = {};
  end

  for i, aggregateKey in ipairs(aggregateKeys) do
    local aggregateMethod = aggregateType[jsonAggregates[aggregateKey]];

    result[values[1]][aggregateKey] = aggregateMethod(
      result[values[1]][aggregateKey],
      values[i + 1]
    );
  end
end

-- repack table into array of arrays
local output = {};

for idKey, aggregatesTable in pairs(result) do
  local outputRow = {};
  tinsert(outputRow, groupByKey);
  tinsert(outputRow, idKey);

  for aggregateName, aggregateValue in pairs(aggregatesTable) do
    tinsert(outputRow, aggregateName);
    tinsert(outputRow, aggregateValue);
  end

  tinsert(output, outputRow);
end

-- paging
local page = {};

if limit ~= 0 then
  for i = offset, offset + limit - 1 do
    tinsert(page, output[i + 1]);
  end
else
  page = output;
end

if #page > 0 then
  return page;
else
  return {0}
end
