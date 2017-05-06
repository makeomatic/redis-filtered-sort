-- cached id list key
local idListKey = KEYS[1];
-- meta key
local metadataKey = KEYS[2];
-- stringified [key]: [aggregateMethod] pairs
local aggregates = ARGV[1];

-- local cache
local rcall = redis.call;
local tinsert = table.insert;

local jsonAggregates = cjson.decode(aggregates);
local aggregateKeys = {};
local result = {};

local function aggregateSum(value1, value2)
  value1 = value1 or 0;
  value2 = value2 or 0;
  return value1 + value2;
end

local aggregateType = {
  sum = aggregateSum
};

for key, method in pairs(jsonAggregates) do
  tinsert(aggregateKeys, key);
  result[key] = 0;

  if type(aggregateType[method]) ~= "function" then
    return error("not supported op: " .. method);
  end
end

local valuesToGroup = rcall("LRANGE", idListKey, 0, -1);

-- group
for _, id in ipairs(valuesToGroup) do
  -- metadata is stored here
  local metaKey = metadataKey:gsub("*", id, 1);
  -- pull information about required aggregate keys
  -- only 1 operation is supported now - sum
  -- but we can calculate multiple values
  local values = rcall("HMGET", metaKey, unpack(aggregateKeys));

  for i, aggregateKey in ipairs(aggregateKeys) do
    local aggregateMethod = aggregateType[jsonAggregates[aggregateKey]];
    local value = tonumber(values[i] or 0);
    result[aggregateKey] = aggregateMethod(result[aggregateKey], value);
  end
end

return cjson.encode(result);