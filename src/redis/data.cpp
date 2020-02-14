#ifndef __REDIS_DATA
#define __REDIS_DATA

#include "redis/redismodule.h"
#include <string>
#include <vector>

namespace ms::redis {
  using namespace std;

  class Data {
  private:
    RedisModuleCtx *ctx;

  public:
    explicit Data(RedisModuleCtx *ctx): ctx(ctx) {
    };

    void saveList(const string& key, vector<string> &data) {
      RedisModule_ThreadSafeContextLock(ctx);
      RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, key.c_str(), key.length());

      auto *psskey = (RedisModuleKey *) RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_WRITE);

      for (auto &it : data) {
        RedisModuleString *redisValue = RedisModule_CreateString(ctx, it.c_str(), it.length());
        RedisModule_ListPush(psskey, REDISMODULE_LIST_TAIL, redisValue);
        RedisModule_FreeString(ctx, redisValue);
      }

      RedisModule_CloseKey(psskey);
      RedisModule_ThreadSafeContextUnlock(ctx);
    };

    int registerCaches(const string& key, const string& tempKeysSet, long long timeStamp, long long ttl) {
      RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
      RedisModule_Call(ctx, "ZADD", "clc", tempKeysSet.c_str(), timeStamp + ttl, key.c_str());
      return 1;
    };
  };

}

#endif
