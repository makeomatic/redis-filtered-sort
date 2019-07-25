#ifndef __FSORT_H
#define __FSORT_H

#include "general.h"
#include "jansson.h"
#include "fsort_types.h"
#include "hashtable.h"
#include "utils.h"
#include "filters.h"

void *fsort_fsort_thread(void *arg);

void *fsort_aggregate_thread(void *arg);
void *fsort_bust_thread(void *arg);

const char **fsort_load_pss_data(FSortObj_t *sort,size_t *dataSize);
const char **fsort_load_idset_data(FSortObj_t *sort, size_t *dataSize);
int fsort_sort_data(FSortObj_t *sort);
int fsort_respond_list(FSortObj_t *sort, const char *key);
json_t *fsort_aggregate_data(HashTable **data, KvStr_t **aggr, const char **fneeded, size_t data_size, size_t  fneededsize);
KvStr_t ** fsort_get_meta(FSortObj_t *sort, const char **sData, size_t len, size_t *resLen);
HashTable  **fsort_get_meta_hashtable(FSortObj_t *sort, const char **sData, size_t len, const char **fieldsNeeded, size_t fieldsCount, size_t *resLen);
void fsort_form_keys(FSortObj_t *sort);

int fsort_parse_args(FSortObj_t *sObj, RedisModuleCtx *ctx, RedisModuleString **argv, int argc);
FSortObj_t *fsort_new_fsort();

void fsort_cache_buster(FSortObj_t *sortObj, const char *key);
void fsort_free_fsort(FSortObj_t *sort);

int fsort_key_type(FSortObj_t *sort, const char *keyName);


#endif