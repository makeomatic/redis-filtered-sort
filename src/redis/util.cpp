#ifndef __REDIS_UTILS
#define __REDIS_UTILS

#include <string>

#include "redis/redismodule.h"

namespace ms::redis {
  using namespace std;

  class GlobalUtil {
  public:
    static int CreateCommand(
      RedisModuleCtx *ctx, const string& fName,
      RedisModuleCmdFunc cmdfunc, const string& strflags,
      int firstkey, int lastkey, int keystep
    ) {
      return RedisModule_CreateCommand(ctx, fName.c_str(), cmdfunc, strflags.c_str(), firstkey, lastkey, keystep);
    };

    static int RedisModuleInit(
      RedisModuleCtx *ctx, const char *name,
      int ver, int apiver
    ) {
      if (RedisModule_Init(ctx, "FilterSortModule", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
      }
      return REDISMODULE_OK;
    };

    static RedisModuleCtx *GetThreadSafeContext(RedisModuleBlockedClient *bc) {
      return RedisModule_GetThreadSafeContext(bc);
    };

    static int UnblockClient(RedisModuleBlockedClient *bc, void *privdata) {
      return RedisModule_UnblockClient(bc, privdata);
    };

    static RedisModuleBlockedClient *BlockClient(
      RedisModuleCtx *ctx, RedisModuleCmdFunc reply_callback,
      RedisModuleCmdFunc timeout_callback, void (*free_privdata)(RedisModuleCtx *, void *), long long timeout_ms
    ) {
      return RedisModule_BlockClient(ctx, reply_callback, timeout_callback, free_privdata, timeout_ms);
    };

    static int WrongArity(RedisModuleCtx *ctx) {
      return RedisModule_WrongArity(ctx);
    }

    static void AutoMemory(RedisModuleCtx *ctx) {
      RedisModule_AutoMemory(ctx);
    }

    static int GetContextFlags(RedisModuleCtx *ctx) {
      return RedisModule_GetContextFlags(ctx);
    }
  };

  class Utils {
  public:
    static long long readLong(RedisModuleString *redisString, long long defaultValue) {
      long long num = 0;
      if (redisString != nullptr) {
        RedisModule_StringToLongLong(redisString, &num);
      }
      return num != 0 ? num : defaultValue;
    }

    static string readString(RedisModuleString *redisString) {
      size_t stringSize;
      const char *cString = RedisModule_StringPtrLen(redisString, &stringSize);
      if (stringSize > 0) {
        return string(cString, stringSize);
      }
      return string();
    }

  };

}
#endif