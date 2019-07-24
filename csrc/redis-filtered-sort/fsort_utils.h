#ifndef __FSORT_UTILS_H
#define __FSORT_UTILS_H

#include "general.h"
#include "fsort_types.h"

int fsort_compare_string_desc(const void *a, const void *b);
int fsort_compare_string_asc(const void *a, const void *b);
int fsort_compare_meta_desc(const void *a, const void *b);
int fsort_compare_meta_asc(const void *a, const void *b);
const char * GetArgvString(RedisModuleCtx *ctx, RedisModuleString **argv, int argc);
char *joinStrArray(RedisModuleCtx *ctx, const char **data, size_t len, char *delim);

const char *fsort_read_string_param(RedisModuleString *arg);
long long fsort_read_long_param(RedisModuleString *str, long long def);

const char * _encode_json(json_t * obj);

KvStrArr_t *kvstr_arr_create();
int kvstr_arr_insert(KvStrArr_t *arr, KvStr_t *v);
int kvstr_arr_free(KvStrArr_t *arr);

KvHashArr_t *kvhash_arr_create();
int kvhash_arr_insert(KvHashArr_t *arr, HashTable *v);
int kvhash_arr_free(KvHashArr_t *arr);

StrArr_t *str_arr_create();
int str_arr_insert(StrArr_t *arr, const char *v);
int str_arr_free(StrArr_t *arr);

int kvstr_free(KvStr_t *str);

#endif