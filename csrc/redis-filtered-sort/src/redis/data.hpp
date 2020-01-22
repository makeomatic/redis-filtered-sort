#ifndef __REDIS_DATA_H
#define __REDIS_DATA_H

#include "redis/redismodule.h"
#include <string>
#include <vector>

namespace ms::redis
{
using namespace std;

class Data
{
private:
  RedisModuleCtx *ctx;

public:
  Data(RedisModuleCtx *ctx);
  void saveList(string key, vector<string> data);
  int registerCaches(string key, string tempKeysSet, long long timeStamp, long long ttl);
};

}

#endif