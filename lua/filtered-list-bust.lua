-- must be a set of ids
local idSet = KEYS[1];
-- current time
local curTime = ARGV[1];
-- caching time
local expiration = tonumber(ARGV[2] or 30000);

--
local tempKeysSet = getIndexTempKeys(idSet);
local keys = redis.call("ZRANGEBYSCORE", tempKeysSet, curTime - expiration, '+inf');

if #keys > 0 then
  redis.call("DEL", unpack(keys));
  redis.call("DEL", tempKeysSet);
end
