#include "filter_module.h"
#include "fsort.h"
#include "fsort_utils.h"
#include "thread_pool.h"

static FSortPool_t *sortPool;

int FSortBust_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);
  
  int ctxFlags = RedisModule_GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti) {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc <3 || argc > 4 ) {
    return RedisModule_WrongArity(ctx);
  }

  pthread_t tid;
  RedisModuleBlockedClient *bc = RedisModule_BlockClient(ctx,NULL,NULL,NULL,0);

  void **targ = RedisModule_Alloc(sizeof(void*)*3);
  targ[0] = bc;
  targ[1] = (void*)(unsigned long) argc;
  targ[2] = argv;

  if (pthread_create(&tid,NULL,fsort_bust_thread,targ) != 0) {
    RedisModule_AbortBlock(bc);
    return RedisModule_ReplyWithError(ctx,"-ERR Can't start thread");
  }

  return REDISMODULE_OK;
}

int FSortAggregate_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);

  int ctxFlags = RedisModule_GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti) {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc != 4) {
    return RedisModule_WrongArity(ctx);
  }

  RedisModuleBlockedClient *bc = RedisModule_BlockClient(ctx,NULL,NULL,NULL,0);

  void **targ = RedisModule_Alloc(sizeof(void*)*3);
  targ[0] = bc;
  //targ[1] = argc;
  targ[2] = argv;

  tpool_add_work(sortPool, fsort_aggregate_thread, targ);
 
  return REDISMODULE_OK; 
}

int FSort_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx);
  
  int ctxFlags = RedisModule_GetContextFlags(ctx);
  int isLua = ctxFlags & REDISMODULE_CTX_FLAGS_LUA;
  int isMulti = ctxFlags & REDISMODULE_CTX_FLAGS_MULTI;

  if (isLua || isMulti) {
    RedisModule_ReplyWithError(ctx, "Can't work in MULTI mode or LUA script!");
    return REDISMODULE_OK;
  }

  if (argc < 7 ) {
    return RedisModule_WrongArity(ctx);
  }
  
  RedisModuleBlockedClient *bc = RedisModule_BlockClient(ctx,NULL,NULL,NULL,0);
  
  FSortObj_t *sort = fsort_new_fsort();
  sort->ctx = ctx;
  int parseRes = fsort_parse_args(sort, ctx, argv, argc);

  fsort_form_keys(sort);
  
  if (parseRes != REDISMODULE_OK) {
    fsort_free_fsort(sort);
    RedisModule_UnblockClient(bc,NULL);
    return REDISMODULE_ERR;
  }

  void **targ = RedisModule_Alloc(sizeof(void*)*2);
  targ[0] = bc;
  targ[1] = sort;
 
   tpool_add_work(sortPool, fsort_fsort_thread, targ);

  return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  long long poolSize = 4;
  if (RedisModule_Init(ctx, "FilterSortModule", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "fsort", FSort_RedisCommand, "write deny-script", 1, 2, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "fsortBust", FSortBust_RedisCommand, "write deny-script", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  if (RedisModule_CreateCommand(ctx, "fsortaggregate", FSortAggregate_RedisCommand, "write deny-script", 1, 2, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  
  if (argc == 1) {
    RedisModule_StringToLongLong(argv[0], &poolSize);
    if (poolSize == 0) {
      poolSize = 4;
    }
  }
  

  sortPool = tpool_create(poolSize);
  return REDISMODULE_OK;
}
