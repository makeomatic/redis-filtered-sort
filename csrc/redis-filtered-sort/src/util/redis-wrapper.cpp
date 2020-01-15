#include "redis-wrapper.hpp"
#include <map>
#include <iostream>

#include "redis/redismodule-cpp.h"

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

RedisModuleBlockedClient *BlockClient(RedisModuleCtx *ctx, RedisModuleCmdFunc reply_callback, RedisModuleCmdFunc timeout_callback, void (*free_privdata)(RedisModuleCtx *, void *), long long timeout_ms)
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

/* Data */

Data::Data(RedisModuleCtx *ctx)
{
  this->ctx = ctx;
}

void Data::saveList(string key, vector<string> data)
{
  RedisModuleCtx *ctx = this->ctx;
  RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, key.c_str(), key.length());

  RedisModuleKey *psskey = (RedisModuleKey *)RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_WRITE);

  for (auto &it : data)
  {
    RedisModuleString *redisValue = RedisModule_CreateString(ctx, it.c_str(), it.length());
    RedisModule_ListPush(psskey, REDISMODULE_LIST_TAIL, redisValue);
    RedisModule_FreeString(ctx, redisValue);
  }

  RedisModule_CloseKey(psskey);
}

/* Command */
Command::Command(RedisModuleCtx *ctx)
{
  this->ctx = ctx;
}

string Command::hget(string key, string field)
{
  RedisModuleCtx *ctx = this->ctx;
  string fieldValue = "";
  RedisModuleString *value;

  RedisModuleString *metaKeyString = RedisModule_CreateString(ctx, key.c_str(), key.length());
  RedisModuleKey *metaKey = (RedisModuleKey *)RedisModule_OpenKey(ctx, metaKeyString, REDISMODULE_READ);

  RedisModule_HashGet(metaKey, REDISMODULE_HASH_CFIELDS, field.c_str(), &value, NULL);

  size_t redisValueSize = 0;
  const char *redisValue = RedisModule_StringPtrLen(value, &redisValueSize);

  if (value != NULL && redisValueSize > 0)
  {
    fieldValue = string(redisValue);
  }
  return fieldValue;
}

int Command::pexpire(string key, long long ttl)
{
  RedisModuleCtx *ctx = this->ctx;
  auto reply = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
  return RedisModule_CallReplyType(reply);
}

int Command::zadd(string key, string value, long long score)
{
  RedisModuleCtx *ctx = this->ctx;

  auto reply = RedisModule_Call(ctx, "ZADD", "ccl", key.c_str(), value.c_str(), score);
  return RedisModule_CallReplyType(reply);
}

vector<string> readArrayReply(RedisModuleCallReply *reply)
{
  vector<string> result;

  int ds = RedisModule_CallReplyLength(reply);
  if (ds > 0)
  {
    for (auto i = 0; i < ds; i++)
    {
      RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
      RedisModuleString *strrep = RedisModule_CreateStringFromCallReply(subreply);
      string row = string(RedisModule_StringPtrLen(strrep, NULL));
      result.push_back(row);
    }
  }
  return result;
}

vector<string> Command::zrangebyscore(string key, long long start, long long stop)
{
  RedisModuleCtx *ctx = this->ctx;

  RM_LOG_DEBUG(ctx, "Getting key %s", key.c_str());
  RedisModuleCallReply *reply = RedisModule_Call(ctx, "ZRANGEBYSCORE", "cll", key.c_str(), start, stop);

  auto replyType = RedisModule_CallReplyType(reply);
  if (replyType == REDISMODULE_REPLY_ARRAY)
  {
    return readArrayReply(reply);
  }
  return {};
}

vector<string> Command::lrange(string key, long long start, long long stop)
{
  RedisModuleCtx *ctx = this->ctx;

  RM_LOG_DEBUG(ctx, "Getting key %s", key.c_str());
  RedisModuleCallReply *reply = RedisModule_Call(this->ctx, "lrange", "cll", key.c_str(), start, stop);

  auto replyType = RedisModule_CallReplyType(reply);
  if (replyType == REDISMODULE_REPLY_ARRAY)
  {
    return readArrayReply(reply);
  }
  return {};
}

vector<string> Command::smembers(string key)
{
  RedisModuleCtx *ctx = this->ctx;

  RM_LOG_DEBUG(ctx, "Getting key %s", key.c_str());
  RedisModuleCallReply *reply = RedisModule_Call(ctx, "SMEMBERS", "cll", key.c_str());

  auto replyType = RedisModule_CallReplyType(reply);
  if (replyType == REDISMODULE_REPLY_ARRAY)
  {
    return readArrayReply(reply);
  }
  return {};
}

int Command::del(string key)
{
  RedisModuleCtx *ctx = this->ctx;
  auto reply = RedisModule_Call(ctx, "DEL", "c", key.c_str());
  return RedisModule_CallReplyType(reply);
}

long long Command::llen(string key)
{
  RedisModuleCtx *ctx = this->ctx;
  RedisModuleCallReply *replyCount = RedisModule_Call(ctx, "LLEN", "c", key.c_str());
  return RedisModule_CallReplyLength(replyCount);
}

int Command::type(string keyName)
{
  RedisModuleCtx *ctx = this->ctx;
  RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, keyName.c_str(), keyName.length());

  RedisModuleKey *key = (RedisModuleKey *)RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_READ);
  int keyType = RedisModule_KeyType(key);
  RedisModule_CloseKey(key);
  return keyType;
}

/* Context */
Context::Context(RedisModuleCtx *ctx)
{
  this->ctx = ctx;
}

Command Context::getCommand()
{
  return Command(this->ctx);
}

Data Context::getData()
{
  return Data(this->ctx);
}

void Context::lockContext()
{
  RedisModule_ThreadSafeContextLock(this->ctx);
}

void Context::unlockContext()
{
  RedisModule_ThreadSafeContextUnlock(this->ctx);
}

void Context::respondLong(long long res)
{
  RedisModule_ReplyWithLongLong(this->ctx, res);
}

void Context::respondEmptyArray()
{
  RedisModule_ReplyWithArray(this->ctx, 1);
  this->respondLong(0);
}

void Context::respondString(string res)
{
  RedisModule_ReplyWithStringBuffer(this->ctx, res.c_str(), res.length());
}

void Context::respondError(string err)
{
  RedisModule_ReplyWithError(this->ctx, err.c_str());
}

int Context::respondList(string key, long long offset, long long limit)
{
  RedisModuleCtx *ctx = this->ctx;

  RedisModuleCallReply *replyCount = RedisModule_Call(ctx, "LLEN", "c", key.c_str());
  RedisModuleCallReply *reply = RedisModule_Call(ctx, "LRANGE", "cll", key.c_str(), offset, offset + limit - 1);

  if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ARRAY)
  {
    size_t elCount = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);

    for (size_t i = 0; i < elCount; i++)
    {
      RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
      RedisModule_ReplyWithCallReply(ctx, subreply);
    }

    RedisModule_ReplyWithCallReply(ctx, replyCount);
    RedisModule_ReplySetArrayLength(ctx, elCount + 1);

    return REDISMODULE_OK;
  }
  else
  {
    return REDISMODULE_ERR;
  }
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
