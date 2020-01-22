#ifndef __REDIS_COMMAND_H
#define __REDIS_COMMAND_H

#include "redis/redismodule.h"
#include <string>
#include <vector>

namespace ms::redis {
  using namespace std;

  class Command {
  private:
    RedisModuleCtx *ctx;

  public:
    Command(RedisModuleCtx *ctx);

    int type(string keyName);

    int pexpire(string key, long long ttl);

    int zadd(string key, string value, long long score);

    int zrem(string key, string value);

    int del(string key);

    string hget(string key, string field);

    vector<string> zrangebyscore(string key, long long start, long long end);

    long long llen(string key);

    vector<string> lrange(string key, long long offset, long long limit);

    vector<string> smembers(string key);
  };

}

#endif