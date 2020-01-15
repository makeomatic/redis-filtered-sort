
#ifndef __GENERAL_H
#define __GENERAL_H

extern "C"
{

#define REDISMODULE_EXPERIMENTAL_API
#define REDIS_MODULE_TARGET 1

#include "redismodule.h"

#define RM_LOG_DEBUG(ctx, ...) RedisModule_Log(ctx, "debug", __VA_ARGS__)
#define RM_LOG_VERBOSE(ctx, ...) RedisModule_Log(ctx, "verbose", __VA_ARGS__)
#define RM_LOG_NOTICE(ctx, ...) RedisModule_Log(ctx, "notice", __VA_ARGS__)
#define RM_LOG_WARNING(ctx, ...) RedisModule_Log(ctx, "warning", __VA_ARGS__)
}

#endif
