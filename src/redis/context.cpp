#ifndef __REDIS_CONTEXT
#define __REDIS_CONTEXT

#include <string>
#include "redis/redismodule.h"

#include "./command.cpp"
#include "./data.cpp"

namespace ms::redis {

  class Context {
  public:
    RedisModuleCtx *ctx;

    explicit Context(RedisModuleCtx *ctx) : ctx(ctx) {};

    ~Context() = default;;

    Command getCommand() {
      return Command(ctx);
    };

    Data getData() {
      return Data(ctx);
    }

    void lockContext() {
      RedisModule_ThreadSafeContextLock(ctx);
    }

    void unlockContext() {
      RedisModule_ThreadSafeContextUnlock(ctx);
    }

    void respondLong(long long res) {
      RedisModule_ReplyWithLongLong(ctx, res);
    }

    void respondEmptyArray() {
      RedisModule_ReplyWithArray(ctx, 1);
      this->respondLong(0);
    }

    void respondString(string &res) {
      RedisModule_ReplyWithStringBuffer(ctx, res.c_str(), res.length());
    }

    void respondError(string &err) {
      RedisModule_ReplyWithError(ctx, err.c_str());
    }

    int respondList(string &key, long long offset, long long limit) {
      RedisModuleCallReply *replyCount = RedisModule_Call(ctx, "LLEN", "c", key.c_str());
      RedisModuleCallReply *reply = RedisModule_Call(ctx, "LRANGE", "cll", key.c_str(), offset, offset + limit - 1);

      if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ARRAY) {
        long elCount = RedisModule_CallReplyLength(reply);
        RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);

        for (auto i = 0; i < elCount; i++) {
          RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
          RedisModule_ReplyWithCallReply(ctx, subreply);
        }

        RedisModule_ReplyWithCallReply(ctx, replyCount);
        RedisModule_ReplySetArrayLength(ctx, elCount + 1);

        return REDISMODULE_OK;
      } else {
        return REDISMODULE_ERR;
      }
    }
  };
}
#endif