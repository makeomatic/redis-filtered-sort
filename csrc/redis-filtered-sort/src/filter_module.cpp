#include <thread>
#include "redis/redismodule.h"
#include "command/aggregate-command.hpp"
#include "command/bust-command.hpp"
#include "command/sort-command.hpp"

#include "util/arg_parser.hpp"
#include "./redis/util.hpp"

int FSortBust_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  ms::redis::GlobalUtil::AutoMemory(ctx);

  int ctxFlags = ms::redis::GlobalUtil::GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti)
  {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc < 3 || argc > 4)
  {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }

  //pthread_t tid;
  RedisModuleBlockedClient *bc = ms::redis::GlobalUtil::BlockClient(ctx, NULL, NULL, NULL, 0);

  std::thread t([bc, argv, argc]() {
    auto redis = ms::redis::Context(ms::redis::GlobalUtil::GetThreadSafeContext(bc));
    auto cmdArgs = ms::arg_parser().parseBustCmdArgs(argv, argc);
    auto cmd = ms::BustCommand(cmdArgs);
    cmd.execute(redis);
    ms::redis::GlobalUtil::UnblockClient(bc, NULL);
  });

  t.detach();
  return REDISMODULE_OK;
}

int FSortAggregate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  ms::redis::GlobalUtil::AutoMemory(ctx);

  int ctxFlags = ms::redis::GlobalUtil::GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti)
  {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc != 4)
  {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }

  RedisModuleBlockedClient *bc = ms::redis::GlobalUtil::BlockClient(ctx, NULL, NULL, NULL, 0);

  std::thread t([bc, argv, argc]() {
    auto redis = ms::redis::Context(ms::redis::GlobalUtil::GetThreadSafeContext(bc));
    auto cmdArgs = ms::arg_parser().parseAggregateCmdArgs(argv, argc);
    auto cmd = ms::AggregateCommand(cmdArgs);
    cmd.execute(redis);
    ms::redis::GlobalUtil::UnblockClient(bc, NULL);
  });

  t.detach();
  //tpool_add_work(sortPool, ms::fsort_cpp::aggregate_thread, targ);

  return REDISMODULE_OK;
}

int FSort_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  ms::redis::GlobalUtil::AutoMemory(ctx);

  if (RedisModule_IsBlockedTimeoutRequest(ctx)) {
    RedisModule_ReplyWithError(ctx, "Timeout response");
    return REDISMODULE_OK;
  }

  if (RedisModule_IsBlockedReplyRequest(ctx)) {
    RedisModule_ReplyWithSimpleString(ctx, "Blocked response");
    return REDISMODULE_OK;
  }


  int ctxFlags = ms::redis::GlobalUtil::GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti)
  {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc < 7)
  {
    return ms::redis::GlobalUtil::WrongArity(ctx);
  }

  RedisModuleBlockedClient *bc = ms::redis::GlobalUtil::BlockClient(ctx, NULL, NULL, NULL, 0);

  std::thread t([bc, argv, argc]() {
    auto redis = ms::redis::Context(ms::redis::GlobalUtil::GetThreadSafeContext(bc));
    auto cmdArgs = ms::arg_parser().parseSortCmdArgs(argv, argc);
    auto cmd = ms::SortCommand(cmdArgs);
    cmd.execute(redis);
    ms::redis::GlobalUtil::UnblockClient(bc, NULL);
  });

  t.detach();

  //tpool_add_work(sortPool, ms::fsort_cpp::fsort_thread, targ);

  return REDISMODULE_OK;
}

extern "C" int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  // long long poolSize = 4;
  if (ms::redis::GlobalUtil::RedisModuleInit(ctx, "FilterSortModule", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (ms::redis::GlobalUtil::CreateCommand(ctx, "fsort", FSort_RedisCommand, "write deny-script", 1, 2, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (ms::redis::GlobalUtil::CreateCommand(ctx, "fsortBust", FSortBust_RedisCommand, "write deny-script", 1, 1, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  if (ms::redis::GlobalUtil::CreateCommand(ctx, "fsortaggregate", FSortAggregate_RedisCommand, "write deny-script", 1, 2, 1) == REDISMODULE_ERR)
  {
    return REDISMODULE_ERR;
  }

  // if (argc == 1)
  // {
  //   RedisModule_StringToLongLong(argv[0], &poolSize);
  //   if (poolSize == 0)
  //   {
  //     poolSize = 4;
  //   }
  // }

  return REDISMODULE_OK;
}
