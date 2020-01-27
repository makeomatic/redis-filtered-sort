#include <thread>
#include "redis/redismodule.h"
#include "command/aggregate-command.cpp"
#include "command/bust-command.cpp"
#include "command/sort-command.cpp"

#include "util/arg-parser.cpp"
#include "redis/util.cpp"
#include "util/thread-pool.cpp"
#include "util/task-registry.cpp"

ms::ThreadPool *threadPool;
ms::TaskRegistry taskRegistry;

template<typename Fn, Fn fn, typename... Args>
typename std::result_of<Fn(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)>::type
redisFunctionWrapper(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  ms::redis::GlobalUtil::AutoMemory(ctx);

  auto ctxFlags = ms::redis::GlobalUtil::GetContextFlags(ctx);
  auto isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  auto isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti) {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_ERR;
  }

  fn(ctx, argv, argc);
  return REDISMODULE_OK;
}

#define FUNCTIONWRAP(FUNC) redisFunctionWrapper<decltype(&FUNC), &FUNC>

int FSortBust_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc < 3 || argc > 4) {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }

  RedisModuleBlockedClient *bc = ms::redis::GlobalUtil::BlockClient(ctx, nullptr, nullptr, nullptr, 0);

  threadPool->Enqueue([bc, argv, argc]() {
    auto redis = ms::redis::Context(ms::redis::GlobalUtil::GetThreadSafeContext(bc));
    auto cmdArgs = ms::ArgParser().parseBustCmdArgs(argv, argc);
    auto cmd = ms::BustCommand(cmdArgs);
    cmd.execute(redis);
    ms::redis::GlobalUtil::UnblockClient(bc, nullptr);
  });

  return REDISMODULE_OK;
}

int FSortAggregate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc != 4) {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }
  RedisModuleBlockedClient *bc = ms::redis::GlobalUtil::BlockClient(ctx, nullptr, nullptr, nullptr, 0);

  threadPool->Enqueue([bc, argv, argc]() {
    auto redis = ms::redis::Context(ms::redis::GlobalUtil::GetThreadSafeContext(bc));
    auto cmdArgs = ms::ArgParser().parseAggregateCmdArgs(argv, argc);
    auto cmd = ms::AggregateCommand(cmdArgs);
    cmd.execute(redis);
    ms::redis::GlobalUtil::UnblockClient(bc, nullptr);
  });

  return REDISMODULE_OK;
}

int FSort_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  if (argc < 7) {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }

  auto cmdArgs = ms::ArgParser::parseSortCmdArgs(argv, argc);
  auto cmd = new ms::SortCommand(cmdArgs, ctx, taskRegistry);

  cmd->execute(threadPool);

  return REDISMODULE_OK;
}

extern "C" int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  long long poolSize = 4;
  if (ms::redis::GlobalUtil::RedisModuleInit(ctx, "FilterSortModule", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (ms::redis::GlobalUtil::CreateCommand(ctx, "fsort", FUNCTIONWRAP(FSort_RedisCommand), "write deny-script", 1, 2, 1) ==
      REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (ms::redis::GlobalUtil::CreateCommand(ctx, "fsortBust", FUNCTIONWRAP(FSortBust_RedisCommand), "write deny-script", 1, 1, 1) ==
      REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (
    ms::redis::GlobalUtil::CreateCommand(ctx, "fsortaggregate", FUNCTIONWRAP(FSortAggregate_RedisCommand), "write deny-script", 1, 2,
                                         1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

   if (argc == 1)
   {
     RedisModule_StringToLongLong(argv[0], &poolSize);
     if (poolSize == 0)
     {
       poolSize = 4;
     }
   }

   threadPool = new ms::ThreadPool(poolSize);
  return REDISMODULE_OK;
}
