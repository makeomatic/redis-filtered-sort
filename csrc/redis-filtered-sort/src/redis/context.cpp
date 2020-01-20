#include "./context.hpp"

using namespace ms::redis;

/* Context */
Context::Context()
{
  // throw invalid_argument("Must pass redis context");
}

Context::~Context() {}

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