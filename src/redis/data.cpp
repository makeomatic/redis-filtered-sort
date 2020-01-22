#include "./data.hpp"

using namespace ms::redis;

Data::Data(RedisModuleCtx *ctx) {
  this->ctx = ctx;
}

void Data::saveList(string key, vector<string> data) {
  RedisModuleCtx *ctx = this->ctx;
  RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, key.c_str(), key.length());

  RedisModuleKey *psskey = (RedisModuleKey *) RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_WRITE);

  for (auto &it : data) {
    RedisModuleString *redisValue = RedisModule_CreateString(ctx, it.c_str(), it.length());
    RedisModule_ListPush(psskey, REDISMODULE_LIST_TAIL, redisValue);
    RedisModule_FreeString(ctx, redisValue);
  }

  RedisModule_CloseKey(psskey);
}

int Data::registerCaches(string key, string tempKeysSet, long long timeStamp, long long ttl) {
  RedisModuleCtx *ctx = this->ctx;

  RedisModule_ThreadSafeContextLock(ctx);

  RedisModuleCallReply *rep = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
  RedisModule_FreeCallReply(rep);

  rep = RedisModule_Call(ctx, "ZADD", "clc", tempKeysSet.c_str(), timeStamp + ttl, key.c_str());

  RedisModule_FreeCallReply(rep);
  RedisModule_ThreadSafeContextUnlock(ctx);
  return 1;
};