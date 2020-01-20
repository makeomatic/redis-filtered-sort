#ifndef __REDIS_UTILS_H
#define __REDIS_UTILS_H

#include <string>
#include <vector>

#include <boost/format.hpp>

#include "./redismodule.h"

namespace ms::redis
{
using namespace std;

class RedisCommander
{
private:
  RedisModuleCtx *ctx;

public:
  RedisCommander(RedisModuleCtx *ctx);
  ~RedisCommander();
  int registerCaches(string key, string keySet, long long timeStamp, long long ttl);
};

class GlobalUtil
{
public:
  static int CreateCommand(RedisModuleCtx *ctx, string fnName, RedisModuleCmdFunc cmdfunc, string strflags, int firstkey, int lastkey, int keystep);
  static int RedisModuleInit(RedisModuleCtx *ctx, const char *name, int ver, int apiver);
  static RedisModuleCtx *GetThreadSafeContext(RedisModuleBlockedClient *bc);
  static int UnblockClient(RedisModuleBlockedClient *bc, void *privdata);
  static RedisModuleBlockedClient *BlockClient(RedisModuleCtx *ctx, RedisModuleCmdFunc reply_callback, RedisModuleCmdFunc timeout_callback, void (*free_privdata)(RedisModuleCtx *, void *), long long timeout_ms);
  static int WrongArity(RedisModuleCtx *ctx);
  static void AutoMemory(RedisModuleCtx *ctx);
  static int GetContextFlags(RedisModuleCtx *ctx);
};

class Utils
{

public:
  static long long readLong(RedisModuleString *redisString, long long defaultValue);
  static string readString(RedisModuleString *redisString);
};

} // namespace ms::redis

#endif