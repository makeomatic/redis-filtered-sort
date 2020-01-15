#include "redis_commands.hpp"
#include <map>
#include <iostream>
using namespace ms;

RedisCommander::RedisCommander()
{
}

RedisCommander::~RedisCommander()
{
}

RedisCommander::RedisCommander(RedisModuleCtx *ctx)
{
    this->redisContext = ctx;
}

void RedisCommander::lockContext()
{
    RedisModule_ThreadSafeContextLock(this->redisContext);
}

void RedisCommander::unlockContext()
{
    RedisModule_ThreadSafeContextUnlock(this->redisContext);
}

void RedisCommander::respondLong(long long res)
{
    RedisModule_ReplyWithLongLong(this->redisContext, res);
}

void RedisCommander::respondEmptyArray()
{
    RedisModule_ReplyWithArray(this->redisContext, 1);
    this->respondLong(0);
}

void RedisCommander::respondString(string res)
{
    RedisModule_ReplyWithStringBuffer(this->redisContext, res.c_str(), res.length());
}

string RedisCommander::getHashField(string key, string field)
{
    RedisModuleCtx *ctx = this->redisContext;

    RedisModuleString *metaKeyString = RedisModule_CreateString(ctx, key.c_str(), key.length());
    RedisModuleKey *metaKey = (RedisModuleKey *)RedisModule_OpenKey(ctx, metaKeyString, REDISMODULE_READ);

    string fieldValue = "";

    if (RedisModule_KeyType(metaKey) == REDISMODULE_KEYTYPE_HASH)
    {
        RedisModuleString *value;
        RedisModule_HashGet(metaKey, REDISMODULE_HASH_CFIELDS, field.c_str(), &value, NULL);

        size_t redisValueSize = 0;
        // pointer must be empty or contain some weird data
        const char *redisValue = RedisModule_StringPtrLen(value, &redisValueSize);

        if (value != NULL && redisValueSize > 0)
        {
            fieldValue = string(redisValue);
        }
    }

    RedisModule_CloseKey(metaKey);

    return fieldValue;
}

void RedisCommander::saveList(string key, vector<string> data)
{
    this->lockContext();
    RedisModuleCtx *ctx = this->redisContext;
    RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, key.c_str(), key.length());

    RedisModuleKey *psskey = (RedisModuleKey *)RedisModule_OpenKey(ctx, redisKeyName, REDISMODULE_WRITE);

    for (std::vector<string>::iterator it = data.begin(); it != data.end(); ++it)
    {
        RedisModuleString *redisValue = RedisModule_CreateString(ctx, it->c_str(), it->length());
        RedisModule_ListPush(psskey, REDISMODULE_LIST_TAIL, redisValue);
        RedisModule_FreeString(ctx, redisValue);
    }

    RedisModule_CloseKey(psskey);
    this->unlockContext();
}

vector<string> RedisCommander::loadList(string key)
{
    RedisModuleCtx *ctx = this->redisContext;
    vector<string> result;

    this->lockContext();
    RM_LOG_DEBUG(ctx, "Getting list key %s", key.c_str());
    RedisModuleCallReply *reply = RedisModule_Call(this->redisContext, "lrange", "cll", key.c_str(), 0, -1);
    this->unlockContext();
    auto replyType = RedisModule_CallReplyType(reply);

    if (replyType == REDISMODULE_REPLY_ARRAY)
    {
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
    else
    {
        throw boost::str(boost::format("Unable to LRANGE %s") % key);
    }
}

vector<string> RedisCommander::loadSet(string key)
{
    RedisModuleCtx *ctx = this->redisContext;
    vector<string> result;

    this->lockContext();
    RM_LOG_DEBUG(ctx, "Getting key %s", key.c_str());
    RedisModuleCallReply *reply = RedisModule_Call(this->redisContext, "SMEMBERS", "c", key.c_str());
    this->unlockContext();
    auto replyType = RedisModule_CallReplyType(reply);

    if (replyType == REDISMODULE_REPLY_ARRAY)
    {
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
        RM_LOG_DEBUG(ctx, "Got %d records", ds);
        return result;
    }
    else
    {
        throw boost::str(boost::format("Unable to SMEMBERS %s") % key);
    }
}
int RedisCommander::respondList(string key, int offset, int limit, int keyOnly)
{
    RedisModuleCtx *ctx = this->redisContext;

    if (keyOnly == 1)
    {
        RedisModule_ReplyWithSimpleString(ctx, key.c_str());
        return REDISMODULE_OK;
    }

    RedisModule_ThreadSafeContextLock(ctx);

    RedisModuleCallReply *replyCount = RedisModule_Call(ctx, "LLEN", "c", key.c_str());
    RedisModuleCallReply *reply = RedisModule_Call(ctx, "LRANGE", "cll", key.c_str(), offset, offset + limit - 1);

    RedisModule_ThreadSafeContextUnlock(ctx);

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

        RedisModule_FreeCallReply(reply);
        RedisModule_FreeCallReply(replyCount);

        return REDISMODULE_OK;
    }
    else
    {
        RedisModule_FreeCallReply(reply);
        RedisModule_FreeCallReply(replyCount);

        return REDISMODULE_ERR;
    }
}

int RedisCommander::expireKey(string key, long long ttl)
{
    RedisModuleCtx *ctx = this->redisContext;

    RedisModule_ThreadSafeContextLock(ctx);

    RedisModuleCallReply *rep = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
    RedisModule_FreeCallReply(rep);

    RedisModule_ThreadSafeContextUnlock(ctx);
    return 1;
}

int RedisCommander::registerCaches(string key, string tempKeysSet, long long timeStamp, long long ttl)
{
    RedisModuleCtx *ctx = this->redisContext;

    RedisModule_ThreadSafeContextLock(ctx);

    RedisModuleCallReply *rep = RedisModule_Call(ctx, "PEXPIRE", "cl", key.c_str(), ttl);
    RedisModule_FreeCallReply(rep);

    rep = RedisModule_Call(ctx, "ZADD", "clc", tempKeysSet.c_str(), timeStamp + ttl, key.c_str());

    RedisModule_FreeCallReply(rep);
    RedisModule_ThreadSafeContextUnlock(ctx);
    return 1;
};

int RedisCommander::cleanCaches(string idSet, long long curTime, long long expire)
{
    std::cerr << "Cleaning caches " << idSet.c_str() << " Curtime" << curTime << " Expire " << expire << "Boundary: " << curTime - expire << " \n";
    long long boundary = curTime - expire;
    RedisModuleCallReply *reply = RedisModule_Call(this->redisContext, "ZRANGEBYSCORE", "clc", idSet.c_str(), boundary, "+inf");
    int rKeyType = RedisModule_CallReplyType(reply);

    if (rKeyType == REDISMODULE_REPLY_ARRAY)
    {
        size_t elCount = RedisModule_CallReplyLength(reply);
        std::cerr << "cleaning keys " << elCount << "\n";
        if (elCount > 0)
        {
            RedisModule_ThreadSafeContextLock(this->redisContext);

            for (size_t i = 0; i < elCount; i++)
            {
                RedisModuleCallReply *keyNameReply = RedisModule_CallReplyArrayElement(reply, i);
                RedisModuleString *keyName = RedisModule_CreateStringFromCallReply(keyNameReply);
                RedisModule_Call(this->redisContext, "del", "s", keyName);
            }

            RedisModule_Call(this->redisContext, "del", "c", idSet.c_str());
            std::cerr << "Tempset clean \n";
            RedisModule_ThreadSafeContextUnlock(this->redisContext);
        }
        this->respondLong(elCount);
        return 0;
    }
    else
    {
        std::cerr << "No keys in idSet " << idSet << "!" << errno << "\n";
        // RedisModule_FreeCallReply(reply);
    }

    // RedisModule_FreeCallReply(reply);
    std::cerr << "Respond long long " << idSet << "\n ===== \n";
    this->respondLong(0);
    return REDISMODULE_OK;
}

int RedisCommander::getKeyType(string keyName)
{
    RedisModuleCtx *ctx = this->redisContext;
    RedisModuleString *redisKeyName = RedisModule_CreateString(ctx, keyName.c_str(), keyName.length());

    RedisModule_ThreadSafeContextLock(this->redisContext);

    RedisModuleKey *key = (RedisModuleKey *)RedisModule_OpenKey(this->redisContext, redisKeyName, REDISMODULE_READ);
    int keyType = RedisModule_KeyType(key);

    RedisModule_CloseKey(key);
    RedisModule_ThreadSafeContextUnlock(this->redisContext);
    return keyType;
}
