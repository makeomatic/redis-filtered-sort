#ifndef __REDIS_COMMANDS_H
#define __REDIS_COMMANDS_H

#include "general.h"

#include <string>
#include <vector>

#include <boost/format.hpp>

namespace ms
{
using namespace std;

class RedisCommander
{
private:
    RedisModuleCtx *redisContext;

public:
    RedisCommander();
    RedisCommander(RedisModuleCtx *ctx);
    ~RedisCommander();

    int getKeyType(string keyName);
    int registerCaches(string key, string keySet, long long timeStamp, long long ttl);
    int cleanCaches(string idSet, long long curTime, long long expire);
    int expireKey(string key, long long ttl);

    int respondList(string key, int offset, int limit, int keyOnly);
    void respondLong(long long response);
    void respondString(string response);
    void respondEmptyArray();

    vector<string> loadList(string key);
    vector<string> loadSet(string key);
    void saveList(string key, vector<string> data);

    string getHashField(string key, string field);

    void lockContext();
    void unlockContext();
};
} // namespace ms

#endif