#include "./command.hpp"

using namespace ms::redis;

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

  auto result = RedisModule_HashGet(metaKey, REDISMODULE_HASH_CFIELDS, field.c_str(), &value, NULL);

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

int Command::zrem(string key, string value)
{
  RedisModuleCtx *ctx = this->ctx;

  auto reply = RedisModule_Call(ctx, "ZREM", "cc", key.c_str(), value.c_str());
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

  RM_LOG_DEBUG(ctx, "zrangebyscore Getting key %s", key.c_str());
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

  RM_LOG_DEBUG(ctx, "lrange Getting key %s", key.c_str());
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
  RedisModuleCallReply *reply = RedisModule_Call(ctx, "SMEMBERS", "c", key.c_str());

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