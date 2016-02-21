
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
local offset = tonumber(ARGV[4] or 0);
-- limit of items to return in a single call
local limit = tonumber(ARGV[5] or 10);
-- caching time
local expiration = tonumber(ARGV[6] or 30000);

-- local caches
local strlower = string.lower;
local strfind = string.find;
local tinsert = table.insert;
local tcontact = table.concat;
local tsort = table.sort;
local rcall = redis.call;
local unpack = unpack;
local pairs = pairs;

local function catch(what)
  return what[1]
end

local function try(what)
  local status, result = pcall(what[1]);
  if not status then
    return what[2](result);
  end

  return result;
end

local function isempty(s)
  return s == nil or s == '';
end

local function isnumber(a)
  return try {
    function()
      local num = tonumber(a);
      return num ~= nil and num or tonumber(cjson.decode(a));
    end,

    catch {
      function()
        return nil;
      end
    }
  }
end

local function subrange(t, first, last)
  local sub = {};
  for i=first + 1,last do
    sub[#sub + 1] = t[i];
  end

  return sub;
end

local function massive_redis_command(command, key, t)
  local i = 1;
  local temp = {};
  while (i <= #t) do
    tinsert(temp, t[i]);
    tinsert(temp, t[i + 1]);
    if #temp >= 1000 then
      rcall(command, key, unpack(temp));
      temp = {};
    end
    i = i + 2;
  end
  if #temp > 0 then
    rcall(command, key, unpack(temp));
  end
end

local function hashmapSize(table)
  local numItems = 0;
  for k,v in pairs(table) do
    numItems = numItems + 1;
  end

  return numItems;
end

local function updateExpireAndReturnWithSize(key)
  rcall("PEXPIRE", key, expiration);
  local ret = rcall("LRANGE", key, offset, offset + limit - 1);
  tinsert(ret, rcall("LLEN", key));
  return ret;
end

-- create filtered list name
local finalFilteredListKeys = { idSet };
local preSortedSetKeys = { idSet };

-- decoded filters
local jsonFilter = cjson.decode(filter);
local totalFilters = hashmapSize(jsonFilter);

-- order always exists
tinsert(finalFilteredListKeys, order);
tinsert(preSortedSetKeys, order);

if isempty(metadataKey) == false then
  if isempty(hashKey) == false then
    tinsert(finalFilteredListKeys, metadataKey);
    tinsert(finalFilteredListKeys, hashKey);
    tinsert(preSortedSetKeys, metadataKey);
    tinsert(preSortedSetKeys, hashKey);
  end

  -- do we have filter?
  if totalFilters > 0 then
    tinsert(finalFilteredListKeys, filter);
  end
elseif totalFilters >= 1 and type(jsonFilter["#"]) == "string" then
  -- the rest of the filters are ignored, since we cant access them!
  tinsert(finalFilteredListKeys, "{\"#\":" .. jsonFilter["#"] .. "}");
end

-- get final filtered key set
local FFLKey = tcontact(finalFilteredListKeys, ":");
local PSSKey = tcontact(preSortedSetKeys, ":");

-- do we have existing filtered set?
if rcall("EXISTS", FFLKey) == 1 then
  return updateExpireAndReturnWithSize(FFLKey);
end

-- do we have an existing sorted set?
local valuesToSort;
if rcall("EXISTS", PSSKey) == 0 then
  valuesToSort = rcall("SMEMBERS", idSet);

  -- if we sort the given set
  if isempty(metadataKey) == false and isempty(hashKey) == false then
    local arr = {};
    for i,v in pairs(valuesToSort) do
      local metaKey = metadataKey:gsub("*", v, 1);
      arr[v] = rcall("HGET", metaKey, hashKey);
    end

    local function sortFuncASC(a, b)
      local sortA = arr[a];
      local sortB = arr[b];

      if isempty(sortA) and isempty(sortB) then
        return true;
      elseif isempty(sortA) then
        return false;
      elseif isempty(sortB) then
        return true;
      else
        local numA = isnumber(sortA);
        local numB = isnumber(sortB);

        if numA ~= nil and numB ~= nil then
          return numA < numB;
        end

        return strlower(sortA) < strlower(sortB);
      end
    end

    local function sortFuncDESC(a, b)
      local sortA = arr[a];
      local sortB = arr[b];

      if isempty(sortA) and isempty(sortB) then
        return false;
      elseif isempty(sortA) then
        return true;
      elseif isempty(sortB) then
        return false;
      else
        local numA = isnumber(sortA);
        local numB = isnumber(sortB);

        if numA ~= nil and numB ~= nil then
          return numA > numB;
        end

        return strlower(sortA) > strlower(sortB);
      end
    end

    if order == "ASC" then
      tsort(valuesToSort, sortFuncASC);
    else
      tsort(valuesToSort, sortFuncDESC);
    end
  else
    if order == "ASC" then
      tsort(valuesToSort, function (a, b) return a < b end);
    else
      tsort(valuesToSort, function (a, b) return a > b end);
    end
  end

  if #valuesToSort > 0 then
    massive_redis_command("RPUSH", PSSKey, valuesToSort);
    rcall("PEXPIRE", PSSKey, expiration);
  else
    return {0};
  end

  if FFLKey == PSSKey then
    -- early return if we have no filter
    local ret = subrange(valuesToSort, offset, offset + limit);
    tinsert(ret, #valuesToSort);
    return ret;
  end
else

  if FFLKey == PSSKey then
    -- early return if we have no filter
    return updateExpireAndReturnWithSize(PSSKey);
  end

  -- populate in-memory data
  -- update expiration timer
  rcall("PEXPIRE", PSSKey, expiration);
  valuesToSort = rcall("LRANGE", PSSKey, 0, -1);
end

-- filtered list holder
local output = {};

-- filter function
local function filterString(value, filter)
  if isempty(value) then
    return false;
  end

  return strfind(strlower(value), strlower(filter)) ~= nil;
end

-- filter: gte
local function gte(value, filter)
  if isempty(value) then
    return false;
  end

  return isnumber(value) >= filter;
end

-- filter: lte
local function lte(value, filter)
  if isempty(value) then
    return false;
  end

  return isnumber(value) <= filter;
end

-- filter: eq
local function eq(value, filter)
  return value == filter;
end

-- filter: not equal
local function ne(value, filter)
  return value ~= filter;
end

local function exists(value)
  return isempty(value) == false;
end

-- supported op type table
local opType = {
  gte = gte,
  lte = lte,
  match = filterString,
  eq = eq,
  ne = ne,
  exists = exists,
  isempty = isempty
};

local function filter(op, opFilter, fieldValue)
  local thunk = opType[op];
  if type(thunk) ~= "function" then
    return error("not supported op: " .. op);
  end

  return thunk(fieldValue, opFilter);
end

-- when we match against a table
local function tableFilter(valueToFilter, filterValue)
  for op, opFilter in pairs(filterValue) do
    if filter(op, opFilter, valueToFilter) ~= true then
      return false;
    end
  end

  return true;
end

local filterType = {
  table = tableFilter,
  string = filterString
};

-- if no metadata key, but we are still here
if isempty(metadataKey) then
  -- only sort by value, which is id
  local filterValue = jsonFilter["#"];
  -- iterate over filtered set
  for i, idValue in pairs(valuesToSort) do
    -- compare strings and insert if they match
    if filterString(idValue, filterValue) then
      tinsert(output, idValue);
    end
  end
-- we actually have metadata
else
  for i, idValue in pairs(valuesToSort) do
    local metaKey = metadataKey:gsub("*", idValue, 1);
    local matched = true;

    for fieldName, filterValue in pairs(jsonFilter) do
      if fieldName == '#multi' then
        local fieldValues = rcall('hmget', metaKey, unpack(filterValue["fields"]));
        local anyMatched = false;
        local matchValue = filterValue["match"];

        for _, fieldValue in pairs(fieldValues) do
          if filterString(fieldValue, matchValue) then
            anyMatched = true;
            break;
          end
        end

        if anyMatched == false then
          matched = false;
          break;
        end
      else
        -- get data that we are filter
        local fieldValue = (fieldName == "#") and idValue or rcall("hget", metaKey, fieldName);

        -- traverse filter types and perform filtering
        if filterType[type(filterValue)](fieldValue, filterValue) ~= true then
          matched = false;
          break;
        end
      end
    end

    if matched then
      tinsert(output, idValue);
    end
  end
end

if #output > 0 then
  massive_redis_command("RPUSH", FFLKey, output);
  return updateExpireAndReturnWithSize(FFLKey);
else
  return {0};
end
