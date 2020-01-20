#include "./util.hpp"
#include <map>
#include <iostream>

using namespace ms::redis;

int GlobalUtil::CreateCommand(RedisModuleCtx *ctx, string fName, RedisModuleCmdFunc cmdfunc, string strflags, int firstkey, int lastkey, int keystep)
{
  return RedisModule_CreateCommand(ctx, fName.c_str(), cmdfunc, strflags.c_str(), firstkey, lastkey, keystep);
}

int GlobalUtil::RedisModuleInit(RedisModuleCtx *ctx, const char *name, int ver, int apiver)
{
  if (RedisModule_Init(ctx, "FilterSortModule", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }
  return REDISMODULE_OK;
}

RedisModuleCtx *GlobalUtil::GetThreadSafeContext(RedisModuleBlockedClient *bc)
{
  return RedisModule_GetThreadSafeContext(bc);
}

int GlobalUtil::UnblockClient(RedisModuleBlockedClient *bc, void *privdata)
{
  return RedisModule_UnblockClient(bc, privdata);
}

RedisModuleBlockedClient *GlobalUtil::BlockClient(RedisModuleCtx *ctx, RedisModuleCmdFunc reply_callback, RedisModuleCmdFunc timeout_callback, void (*free_privdata)(RedisModuleCtx *, void *), long long timeout_ms)
{
  return RedisModule_BlockClient(ctx, reply_callback, timeout_callback, free_privdata, timeout_ms);
}

int GlobalUtil::WrongArity(RedisModuleCtx *ctx)
{
  return RedisModule_WrongArity(ctx);
}

void GlobalUtil::AutoMemory(RedisModuleCtx *ctx)
{
  RedisModule_AutoMemory(ctx);
}

int GlobalUtil::GetContextFlags(RedisModuleCtx *ctx)
{
  return RedisModule_GetContextFlags(ctx);
}

/* Utils */
long long Utils::readLong(RedisModuleString *redisString, long long defaultValue)
{
  long long num = 0;
  if (redisString != NULL)
  {
    RedisModule_StringToLongLong(redisString, &num);
  }
  return num != 0 ? num : defaultValue;
}

string Utils::readString(RedisModuleString *redisString)
{
  size_t stringSize;
  const char *cString = RedisModule_StringPtrLen(redisString, &stringSize);
  if (stringSize > 0)
  {
    return string(cString, stringSize);
  }
  return string();
}

/////////////////////////////// OLD

RedisCommander::RedisCommander(RedisModuleCtx *ctx)
{
  this->ctx = ctx;
}

int RedisCommander::registerCaches(string key, string tempKeysSet, long long timeStamp, long long ttl)
{
  RedisModuleCtx *ctx = this->ctx;

  RedisModule_ThreadSafeContextLock(ctx);

  RedisModuleCallReply *rep = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
  RedisModule_FreeCallReply(rep);

  rep = RedisModule_Call(ctx, "ZADD", "clc", tempKeysSet.c_str(), timeStamp + ttl, key.c_str());

  RedisModule_FreeCallReply(rep);
  RedisModule_ThreadSafeContextUnlock(ctx);
  return 1;
};
