#ifndef __REDIS_MODULE_DEFINES
#define __REDIS_MODULE_DEFINES

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

/* Error status return values. */
#define REDISMODULE_OK 0
#define REDISMODULE_ERR 1

/* API versions. */
#define REDISMODULE_APIVER_1 1

/* API flags and constants */
#define REDISMODULE_READ (1 << 0)
#define REDISMODULE_WRITE (1 << 1)

#define REDISMODULE_LIST_HEAD 0
#define REDISMODULE_LIST_TAIL 1

/* Key types. */
#define REDISMODULE_KEYTYPE_EMPTY 0
#define REDISMODULE_KEYTYPE_STRING 1
#define REDISMODULE_KEYTYPE_LIST 2
#define REDISMODULE_KEYTYPE_HASH 3
#define REDISMODULE_KEYTYPE_SET 4
#define REDISMODULE_KEYTYPE_ZSET 5
#define REDISMODULE_KEYTYPE_MODULE 6

/* Reply types. */
#define REDISMODULE_REPLY_UNKNOWN -1
#define REDISMODULE_REPLY_STRING 0
#define REDISMODULE_REPLY_ERROR 1
#define REDISMODULE_REPLY_INTEGER 2
#define REDISMODULE_REPLY_ARRAY 3
#define REDISMODULE_REPLY_NULL 4

/* Postponed array length. */
#define REDISMODULE_POSTPONED_ARRAY_LEN -1

/* Expire */
#define REDISMODULE_NO_EXPIRE -1

/* Sorted set API flags. */
#define REDISMODULE_ZADD_XX (1 << 0)
#define REDISMODULE_ZADD_NX (1 << 1)
#define REDISMODULE_ZADD_ADDED (1 << 2)
#define REDISMODULE_ZADD_UPDATED (1 << 3)
#define REDISMODULE_ZADD_NOP (1 << 4)

/* Hash API flags. */
#define REDISMODULE_HASH_NONE 0
#define REDISMODULE_HASH_NX (1 << 0)
#define REDISMODULE_HASH_XX (1 << 1)
#define REDISMODULE_HASH_CFIELDS (1 << 2)
#define REDISMODULE_HASH_EXISTS (1 << 3)

/* Context Flags: Info about the current context returned by
 * RM_GetContextFlags(). */

/* The command is running in the context of a Lua script */
#define REDISMODULE_CTX_FLAGS_LUA (1 << 0)
/* The command is running inside a Redis transaction */
#define REDISMODULE_CTX_FLAGS_MULTI (1 << 1)
/* The instance is a master */
#define REDISMODULE_CTX_FLAGS_MASTER (1 << 2)
/* The instance is a slave */
#define REDISMODULE_CTX_FLAGS_SLAVE (1 << 3)
/* The instance is read-only (usually meaning it's a slave as well) */
#define REDISMODULE_CTX_FLAGS_READONLY (1 << 4)
/* The instance is running in cluster mode */
#define REDISMODULE_CTX_FLAGS_CLUSTER (1 << 5)
/* The instance has AOF enabled */
#define REDISMODULE_CTX_FLAGS_AOF (1 << 6)
/* The instance has RDB enabled */
#define REDISMODULE_CTX_FLAGS_RDB (1 << 7)
/* The instance has Maxmemory set */
#define REDISMODULE_CTX_FLAGS_MAXMEMORY (1 << 8)
/* Maxmemory is set and has an eviction policy that may delete keys */
#define REDISMODULE_CTX_FLAGS_EVICT (1 << 9)
/* Redis is out of memory according to the maxmemory flag. */
#define REDISMODULE_CTX_FLAGS_OOM (1 << 10)
/* Less than 25% of memory available according to maxmemory. */
#define REDISMODULE_CTX_FLAGS_OOM_WARNING (1 << 11)

#define REDISMODULE_NOTIFY_GENERIC (1 << 2)                                                                                                                                                                                                                                          /* g */
#define REDISMODULE_NOTIFY_STRING (1 << 3)                                                                                                                                                                                                                                           /* $ */
#define REDISMODULE_NOTIFY_LIST (1 << 4)                                                                                                                                                                                                                                             /* l */
#define REDISMODULE_NOTIFY_SET (1 << 5)                                                                                                                                                                                                                                              /* s */
#define REDISMODULE_NOTIFY_HASH (1 << 6)                                                                                                                                                                                                                                             /* h */
#define REDISMODULE_NOTIFY_ZSET (1 << 7)                                                                                                                                                                                                                                             /* z */
#define REDISMODULE_NOTIFY_EXPIRED (1 << 8)                                                                                                                                                                                                                                          /* x */
#define REDISMODULE_NOTIFY_EVICTED (1 << 9)                                                                                                                                                                                                                                          /* e */
#define REDISMODULE_NOTIFY_STREAM (1 << 10)                                                                                                                                                                                                                                          /* t */
#define REDISMODULE_NOTIFY_ALL (REDISMODULE_NOTIFY_GENERIC | REDISMODULE_NOTIFY_STRING | REDISMODULE_NOTIFY_LIST | REDISMODULE_NOTIFY_SET | REDISMODULE_NOTIFY_HASH | REDISMODULE_NOTIFY_ZSET | REDISMODULE_NOTIFY_EXPIRED | REDISMODULE_NOTIFY_EVICTED | REDISMODULE_NOTIFY_STREAM) /* A */

/* A special pointer that we can use between the core and the module to signal
 * field deletion, and that is impossible to be a valid pointer. */
#define REDISMODULE_HASH_DELETE ((RedisModuleString *)(long)1)

/* Error messages. */
#define REDISMODULE_ERRORMSG_WRONGTYPE "WRONGTYPE Operation against a key holding the wrong kind of value"

#define REDISMODULE_POSITIVE_INFINITE (1.0 / 0.0)
#define REDISMODULE_NEGATIVE_INFINITE (-1.0 / 0.0)

/* Cluster API defines. */
#define REDISMODULE_NODE_ID_LEN 40
#define REDISMODULE_NODE_MYSELF (1 << 0)
#define REDISMODULE_NODE_MASTER (1 << 1)
#define REDISMODULE_NODE_SLAVE (1 << 2)
#define REDISMODULE_NODE_PFAIL (1 << 3)
#define REDISMODULE_NODE_FAIL (1 << 4)
#define REDISMODULE_NODE_NOFAILOVER (1 << 5)

#define REDISMODULE_CLUSTER_FLAG_NONE 0
#define REDISMODULE_CLUSTER_FLAG_NO_FAILOVER (1 << 1)
#define REDISMODULE_CLUSTER_FLAG_NO_REDIRECTION (1 << 2)

#define REDISMODULE_NOT_USED(V) ((void)V)

/* This type represents a timer handle, and is returned when a timer is
 * registered and used in order to invalidate a timer. It's just a 64 bit
 * number, because this is how each timer is represented inside the radix tree
 * of timers that are going to expire, sorted by expire time. */
typedef uint64_t RedisModuleTimerID;

#ifndef REDISMODULE_CORE

typedef long long mstime_t;

/* Incomplete structures for compiler checks but opaque access. */
typedef struct RedisModuleCtx RedisModuleCtx;
typedef struct RedisModuleKey RedisModuleKey;
typedef struct RedisModuleString RedisModuleString;
typedef struct RedisModuleCallReply RedisModuleCallReply;
typedef struct RedisModuleIO RedisModuleIO;
typedef struct RedisModuleType RedisModuleType;
typedef struct RedisModuleDigest RedisModuleDigest;
typedef struct RedisModuleBlockedClient RedisModuleBlockedClient;
typedef struct RedisModuleClusterInfo RedisModuleClusterInfo;
typedef struct RedisModuleDict RedisModuleDict;
typedef struct RedisModuleDictIter RedisModuleDictIter;

typedef int (*RedisModuleCmdFunc)(RedisModuleCtx *ctx, RedisModuleString **argv, int argc);
typedef void (*RedisModuleDisconnectFunc)(RedisModuleCtx *ctx, RedisModuleBlockedClient *bc);
typedef int (*RedisModuleNotificationFunc)(RedisModuleCtx *ctx, int type, const char *event, RedisModuleString *key);
typedef void *(*RedisModuleTypeLoadFunc)(RedisModuleIO *rdb, int encver);
typedef void (*RedisModuleTypeSaveFunc)(RedisModuleIO *rdb, void *value);
typedef void (*RedisModuleTypeRewriteFunc)(RedisModuleIO *aof, RedisModuleString *key, void *value);
typedef size_t (*RedisModuleTypeMemUsageFunc)(const void *value);
typedef void (*RedisModuleTypeDigestFunc)(RedisModuleDigest *digest, void *value);
typedef void (*RedisModuleTypeFreeFunc)(void *value);
typedef void (*RedisModuleClusterMessageReceiver)(RedisModuleCtx *ctx, const char *sender_id, uint8_t type, const unsigned char *payload, uint32_t len);
typedef void (*RedisModuleTimerProc)(RedisModuleCtx *ctx, void *data);

#define REDISMODULE_TYPE_METHOD_VERSION 1
typedef struct RedisModuleTypeMethods
{
    uint64_t version;
    RedisModuleTypeLoadFunc rdb_load;
    RedisModuleTypeSaveFunc rdb_save;
    RedisModuleTypeRewriteFunc aof_rewrite;
    RedisModuleTypeMemUsageFunc mem_usage;
    RedisModuleTypeDigestFunc digest;
    RedisModuleTypeFreeFunc free;
} RedisModuleTypeMethods;

#endif

#endif