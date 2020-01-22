#ifndef __REDIS_CONTEXT_H
#define __REDIS_CONTEXT_H

#include <string>
#include <vector>

#include "redis/redismodule.h"

#include "./command.hpp"
#include "./data.hpp"

namespace ms::redis {
  using namespace std;

  class Context {
  private:
    RedisModuleCtx *ctx;

  public:
    Context();

    Context(RedisModuleCtx *ctx);

    ~Context();

    Command getCommand();

    Data getData();

    void respondLong(long long response);

    void respondString(string response);

    void respondError(string error);

    void respondEmptyArray();

    int respondList(string key, long long offset, long long limit);

    void lockContext();

    void unlockContext();
  };

} // namespace ms::redis

#endif