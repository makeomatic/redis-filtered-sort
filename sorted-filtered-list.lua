-- must be a set of ids
local idSet = KEYS[1];
-- must be a key, which holds metadata for a given id, with a `*` as a substitute for an id from idSet. Optional
local metadataKey = KEYS[2];

-- hashKey to use for sorting, which is taken from `metadataKey` hash, Optional.
local hashKey = ARGV[1];
-- ASC / DESC
local order = ARGV[2];
-- stringified instructions for filtering
local filter = ARGV[3];
-- pagination offset
local offset = tonumber(ARGV[4]);
-- limit of items to return in a single call
local limit = tonumber(ARGV[5]);
-- caching time
local expiration = tonumber(ARGV[6] or 30000);

local function isempty(s)
  return s == nil or s == '';
end

local function isnumber(a)
  return tonumber(a) ~= nil;
end

local function subrange(t, first, last)
  local sub = {};
  for i=first + 1,last do
    sub[#sub + 1] = t[i];
  end

  return sub;
end

local function hashmapSize(table)
  local numItems = 0;
  for k,v in pairs(table) do
    numItems = numItems + 1;
  end

  return numItems;
end

local function updateExpireAndReturnWithSize(key)
  redis.call("PEXPIRE", key, expiration);
  local ret = redis.call("LRANGE", key, offset, offset + limit - 1);
  table.insert(ret, redis.call("LLEN", key));
  return ret;
end

-- create filtered list name
local finalFilteredListKeys = { idSet };
local preSortedSetKeys = { idSet };

-- decoded filters
local jsonFilter = cjson.decode(filter);
local totalFilters = hashmapSize(jsonFilter);

-- order always exists
table.insert(finalFilteredListKeys, order);
table.insert(preSortedSetKeys, order);

if isempty(metadataKey) == false then
  if isempty(hashKey) == false then
    table.insert(finalFilteredListKeys, metadataKey);
    table.insert(finalFilteredListKeys, hashKey);
    table.insert(preSortedSetKeys, metadataKey);
    table.insert(preSortedSetKeys, hashKey);
  end

  -- do we have filter?
  if totalFilters > 0 then
    table.insert(finalFilteredListKeys, filter);
  end
elseif totalFilters == 1 and type(jsonFilter["#"]) == "string" then
  table.insert(finalFilteredListKeys, filter);
end

-- get final filtered key set
local FFLKey = table.concat(finalFilteredListKeys, ":");
local PSSKey = table.concat(preSortedSetKeys, ":");

-- do we have existing filtered set?
if redis.call("EXISTS", FFLKey) == 1 then
  return updateExpireAndReturnWithSize(FFLKey);
end

-- do we have existing sorted set?
local valuesToSort;
if redis.call("EXISTS", PSSKey) == 0 then
  valuesToSort = redis.call("SMEMBERS", idSet);

  -- if we sort the given set
  if isempty(metadataKey) == false and isempty(hashKey) == false then
    local arr = {};
    for i,v in pairs(valuesToSort) do
      local metaKey = metadataKey:gsub("*", v, 1);
      arr[v] = redis.call("HGET", metaKey, hashKey);
    end

    if order == "ASC" then
      local function sortFuncASC(a, b)
        local sortA = arr[a];
        local sortB = arr[b];

        if isempty(sortA) and isempty(sortB) then
          return true;
        elseif isempty(sortA) then
          return false;
        elseif isempty(sortB) then
          return true;
        elseif isnumber(sortA) and isnumber(sortB) then
          return tonumber(sortA) < tonumber(sortB);
        else
          return string.lower(sortA) < string.lower(sortB);
        end
      end

      table.sort(valuesToSort, sortFuncASC);
    else
      local function sortFuncDESC(a, b)
        local sortA = arr[a];
        local sortB = arr[b];

        if isempty(sortA) and isempty(sortB) then
          return false;
        elseif isempty(sortA) then
          return true;
        elseif isempty(sortB) then
          return false;
        elseif isnumber(sortA) and isnumber(sortB) then
          return tonumber(sortA) > tonumber(sortB);
        else
          return string.lower(sortA) > string.lower(sortB);
        end
      end

      table.sort(valuesToSort, sortFuncDESC);
    end
  else
    if order == "ASC" then
      table.sort(valuesToSort, function (a, b) return a < b end);
    else
      table.sort(valuesToSort, function (a, b) return a > b end);
    end
  end

  if #valuesToSort > 0 then
    redis.call("RPUSH", PSSKey, unpack(valuesToSort));
    redis.call("PEXPIRE", PSSKey, expiration);
  else
    return {0};
  end

  if FFLKey == PSSKey then
    -- early return if we have no filter
    local ret = subrange(valuesToSort, offset, offset + limit);
    table.insert(ret, #valuesToSort);
    return ret;
  end
else

  if FFLKey == PSSKey then
    -- early return if we have no filter
    return updateExpireAndReturnWithSize(PSSKey);
  end

  -- populate in-memory data
  -- update expiration timer
  redis.call("PEXPIRE", PSSKey, expiration);
  valuesToSort = redis.call("LRANGE", PSSKey, 0, -1);
end

-- filtered list holder
local output = {};

-- filter function
local function filterString(value, filter)
  if isempty(value) then
    return nil;
  end

  return string.find(string.lower(value), string.lower(filter));
end

-- filter: gte
local function gte(value, filter)
  if isempty(value) then
    return nil;
  end

  return tonumber(value) >= tonumber(filter);
end

-- filter: lte
local function lte(value, filter)
  if isempty(value) then
    return nil;
  end

  return tonumber(value) <= tonumber(filter);
end

-- if no metadata key, but we are still here
if isempty(metadataKey) then
  -- only sort by value, which is id
  local filterValue = jsonFilter["#"];
  -- iterate over filtered set
  for i,idValue in pairs(valuesToSort) do
    -- compare strings and insert if they match
    if filterString(idValue, filterValue) ~= nil then
      table.insert(output, idValue);
    end
  end
-- we actually have metadata
else
  for i,idValue in pairs(valuesToSort) do
    local metaKey = metadataKey:gsub("*", idValue, 1);
    local matched = true;

    for fieldName, filterValue in pairs(jsonFilter) do
      local fieldValue;

      -- special case
      if fieldName == "#" then
        fieldValue = idValue;
      else
        fieldValue = redis.call("hget", metaKey, fieldName);
      end

      if type(filterValue) == 'table' then
        for op, opFilter in pairs(filterValue) do
          if op == 'gte' then
            if gte(fieldValue, opFilter) ~= true then
              matched = false;
              break;
            end
          elseif op == 'lte' then
            if lte(fieldValue, opFilter) ~= true then
              matched = false;
              break;
            end
          else
            return redis.error_reply("not supported op: " .. op);
          end
        end

        if matched == false then
          break;
        end
      else
        if filterString(fieldValue, filterValue) == nil then
          matched = false;
          break;
        end
      end
    end

    if matched then
      table.insert(output, idValue);
    end
  end
end

if #output > 0 then
  redis.call("RPUSH", FFLKey, unpack(output));
  return updateExpireAndReturnWithSize(FFLKey);
else
  return {0};
end
