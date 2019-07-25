#ifndef __FSORT_TYPES_H
#define __FSORT_TYPES_H
#include "general.h"
#include "jansson.h"
#include "hashtable.h"
#include "filters.h"

typedef struct KvStr {
  const char * key;
  const char * value;
} KvStr_t;

typedef struct KvHashArr {
  size_t size;
  HashTable **values;
} KvHashArr_t;

typedef struct KvStrArr {
  size_t size;
  KvStr_t **values;
} KvStrArr_t;

typedef struct StrArr {
  size_t size;
  const char **values;
} StrArr_t;

typedef struct FSortObj {
  const char *fflKey;
  const char *pssKey;
  RedisModuleString *fflKeyName;
  RedisModuleString *pssKeyName;

  const char *tempKeysSet;
  const char *idSet;
  const char *metaKey;
  const char *hashKey;
  const char *order;
  RedisModuleString *filter;
  long long curTime;
  long long offset;
  long long limit;
  long long expire;
  long long keyOnly;

  int threaded;  
  json_t * _json;

  StrArr_t *sortData;
  StrArr_t *outputData;
  FilterBlock_t *filters;
   //Store redis context
  RedisModuleCtx *ctx;

} FSortObj_t;

#endif