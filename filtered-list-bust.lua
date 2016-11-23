-- 
local idSet = KEYS[1];

--
local expiration = tonumber(ARGV[2] or 30000);

--
local tempKeysSet = getIndexTempKeys(idSet);
local keys = redis.call("ZRANGEBYSCORE", tempKeysSet, curTime - expiration, curTime);

if #keys > 0 then
  redis.call("DEL", unpack(keys));
  redis.del(tempKeysSet);
end
