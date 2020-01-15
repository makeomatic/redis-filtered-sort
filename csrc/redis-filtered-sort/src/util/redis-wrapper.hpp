#ifndef __REDIS_COMMANDS_H
#define __REDIS_COMMANDS_H

#include <string>
#include <vector>

#include <boost/format.hpp>

extern "C"
{
#include "redis/redismodule-defs.h"
};

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

class Data
{
private:
  RedisModuleCtx *ctx;

public:
  Data(RedisModuleCtx *ctx);
  void saveList(string key, vector<string> data);
};

class Command
{
private:
  RedisModuleCtx *ctx;

public:
  Command(RedisModuleCtx *ctx);
  int type(string keyName);
  int pexpire(string key, long long ttl);
  int zadd(string key, string value, long long score);
  int del(string key);

  string hget(string key, string field);

  vector<string> zrangebyscore(string key, long long start, long long end);

  long long llen(string key);
  vector<string> lrange(string key, long long offset, long long limit);
  vector<string> smembers(string key);
};

class Context
{
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

class Utils
{

public:
  static long long readLong(RedisModuleString *redisString, long long defaultValue);
  static string readString(RedisModuleString *redisString);
};

} // namespace ms::redis

#endif