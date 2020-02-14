#ifndef __REDIS_COMMAND
#define __REDIS_COMMAND

#include "redis/redismodule.h"
#include <string>
#include <vector>

namespace ms::redis {
  using namespace std;

  class Command {
  private:
    RedisModuleCtx *ctx;

    static vector<string> readArrayReply(RedisModuleCallReply *reply) {
      vector<string> result;

      int ds = RedisModule_CallReplyLength(reply);
      if (ds > 0) {
        for (auto i = 0; i < ds; i++) {
          RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
          RedisModuleString *strrep = RedisModule_CreateStringFromCallReply(subreply);
          string row = string(RedisModule_StringPtrLen(strrep, nullptr));
          result.push_back(row);
        }
      }
      return result;
    }

  public:
    explicit Command(RedisModuleCtx *ctx) : ctx(ctx) {};

    int type(const string& keyName) {
      RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, keyName.c_str(), keyName.length());

      auto *key = (RedisModuleKey *) RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_READ);
      int keyType = RedisModule_KeyType(key);
      RedisModule_CloseKey(key);
      return keyType;
    };

    int pexpire(const string& key, long long ttl) {
      auto reply = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
      return RedisModule_CallReplyType(reply);
    };

    int zadd(const string& key, const string& value, long long score) {
      auto reply = RedisModule_Call(ctx, "ZADD", "ccl", key.c_str(), value.c_str(), score);
      return RedisModule_CallReplyType(reply);
    };

    int zrem(const string& key, const string& value) {
      auto reply = RedisModule_Call(ctx, "ZREM", "cc", key.c_str(), value.c_str());
      return RedisModule_CallReplyType(reply);
    };

    int del(const string& key) {
      auto reply = RedisModule_Call(ctx, "DEL", "c", key.c_str());
      return RedisModule_CallReplyType(reply);
    };

    string hget(const string& key, const string& field) {
      string fieldValue;
      RedisModuleString *value;

      RedisModuleString *metaKeyString = RedisModule_CreateString(ctx, key.c_str(), key.length());
      auto *metaKey = (RedisModuleKey *) RedisModule_OpenKey(ctx, metaKeyString, REDISMODULE_READ);

      RedisModule_HashGet(metaKey, REDISMODULE_HASH_CFIELDS, field.c_str(), &value, NULL);

      size_t redisValueSize = 0;
      const char *redisValue = RedisModule_StringPtrLen(value, &redisValueSize);

      if (value != nullptr && redisValueSize > 0) {
        fieldValue = string(redisValue);
      }
      return fieldValue;
    };

    vector<string> zrangebyscore(const string& key, long long start, long long stop) {
      RedisModuleCallReply *reply = RedisModule_Call(ctx, "ZRANGEBYSCORE", "cll", key.c_str(), start, stop);

      auto replyType = RedisModule_CallReplyType(reply);
      if (replyType == REDISMODULE_REPLY_ARRAY) {
        return readArrayReply(reply);
      }
      return {};
    };

    vector<string> zrangebyscore(const string& key, const string& start, const string& stop) {
      RedisModuleCallReply *reply = RedisModule_Call(ctx, "ZRANGEBYSCORE", "ccc", key.c_str(), start.c_str(), stop.c_str());

      auto replyType = RedisModule_CallReplyType(reply);
      if (replyType == REDISMODULE_REPLY_ARRAY) {
        return readArrayReply(reply);
      }
      return {};
    };

    long long llen(const string& key) {
      RedisModuleCallReply *replyCount = RedisModule_Call(ctx, "LLEN", "c", key.c_str());
      return RedisModule_CallReplyLength(replyCount);
    };

    vector<string> lrange(const string& key, long long start, long long stop) {
      RedisModuleCallReply *reply = RedisModule_Call(this->ctx, "lrange", "cll", key.c_str(), start, stop);

      auto replyType = RedisModule_CallReplyType(reply);
      if (replyType == REDISMODULE_REPLY_ARRAY) {
        return readArrayReply(reply);
      }
      return {};
    };

    vector<string> smembers(const string& key) {
      RedisModuleCallReply *reply = RedisModule_Call(ctx, "SMEMBERS", "c", key.c_str());

      auto replyType = RedisModule_CallReplyType(reply);
      if (replyType == REDISMODULE_REPLY_ARRAY) {
        return readArrayReply(reply);
      }
      return {};
    };
  };

}

#endif