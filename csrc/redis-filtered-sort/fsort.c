#include "fsort.h"
#include "fsort_utils.h"
#include "string.h"

RedisModuleString * fsort_char_rms(FSortObj_t *sort, const char *str) {
  return RedisModule_CreateStringPrintf(sort->ctx,"%s", str);
}


void fsort_cache_buster(FSortObj_t *sortObj, const char *keyStr) {
  long long ttl;
  ttl = sortObj->curTime + sortObj->expire;
  RedisModule_ThreadSafeContextLock(sortObj->ctx);
  RedisModuleCallReply *rep = RedisModule_Call(sortObj->ctx, "PEXPIRE", "cl", keyStr, sortObj->expire);
  RedisModule_FreeCallReply(rep);
  rep = RedisModule_Call(sortObj->ctx, "ZADD", "clc", sortObj->tempKeysSet,ttl,keyStr);
  RedisModule_FreeCallReply(rep);
  RedisModule_ThreadSafeContextUnlock(sortObj->ctx);
  
}

void fsort_key_pexpire(FSortObj_t *sort, const char *keyStr) {
  RedisModule_ThreadSafeContextLock(sort->ctx);
  RedisModuleCallReply *rep = RedisModule_Call(sort->ctx, "PEXPIRE", "cl", keyStr, sort->expire);
  RedisModule_FreeCallReply(rep);
  RedisModule_ThreadSafeContextUnlock(sort->ctx);
}

int fsort_parse_args(FSortObj_t *sObj, RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

  sObj->ctx = ctx;
  sObj->idSet = fsort_read_string_param(argv[1]);
  sObj->metaKey = fsort_read_string_param(argv[2]);
  sObj->hashKey = fsort_read_string_param(argv[3]);
  sObj->filter = argv[5];
  sObj->curTime = fsort_read_long_param(argv[6], 0);
  sObj->filters = NULL;

  const char * orderStr = strdup(RedisModule_StringPtrLen(argv[4], NULL));
  sObj->order = orderStr;

  if (argc > 7 ) sObj->offset = fsort_read_long_param(argv[7], 0);
  if (argc > 8 ) sObj->limit = fsort_read_long_param(argv[8], 10);
  if (argc > 9 ) sObj->expire = fsort_read_long_param(argv[9], 30000);

  if (argc == 11 && argv[10] != NULL) {
    sObj->keyOnly = 1;
  } else {
    sObj->keyOnly = 0;
  }
  sObj->tempKeysSet = malloc(strlen(sObj->idSet) + 17 + 1);
  sprintf((char*)sObj->tempKeysSet, "%s::fsort_temp_keys", sObj->idSet );
  
  size_t json_str_len;
  json_error_t *json_err = NULL;
  const char * filterString = RedisModule_StringPtrLen(sObj->filter, &json_str_len);
  sObj->_json = json_loads(filterString, json_str_len, json_err);
  
  if (json_err != NULL) {
    char* err = malloc(400);
    sprintf(err,"Json parse error: %d", json_error_code(json_err));
    RedisModule_ReplyWithError(ctx, err);
    free(err);
    return REDISMODULE_ERR;
  }

  FilterBlock_t *filters = filter_create_filters(sObj->_json);
  if (filters->err != NULL) {
    RedisModule_ReplyWithError(sObj->ctx, filters->err);
    filter_block_free(filters);
    return REDISMODULE_ERR;
  }
  sObj->filters = filters;    

  return REDISMODULE_OK;
}

FSortObj_t *fsort_new_fsort() {
  FSortObj_t *sobj = malloc(sizeof(FSortObj_t));
  
  sobj->expire = 30000;
  sobj->offset = 0;
  sobj->limit = 10;
  sobj->sortData = str_arr_create();
  sobj->outputData = str_arr_create();
  sobj->filters = NULL;
  sobj->order = NULL;
  sobj->metaKey = NULL;
  sobj->idSet = NULL;
  sobj->fflKey = NULL;
  sobj->tempKeysSet = NULL;

  return sobj;
}

void fsort_free_fsort(FSortObj_t *sort) {
  if (sort->outputData != NULL) {
    str_arr_free(sort->outputData);
  }

  if (sort->sortData != NULL) {
    str_arr_free(sort->sortData);
  }
  
  json_decref(sort->_json);
  if (sort->filters != NULL) filter_block_free(sort->filters);
  if (sort->pssKey != NULL) free((char *)sort->pssKey);
  if (sort->fflKey != NULL) free((char *) sort->fflKey);
  if (sort->tempKeysSet != NULL) free((char *)sort->tempKeysSet);
  if (sort->hashKey != NULL) free((char*)sort->hashKey);
  if (sort->idSet != NULL) free((char *)sort->idSet);
  if (sort->metaKey != NULL) free((char *)sort->metaKey);
  if (sort->order != NULL) free((char *)sort->order);
  sort->ctx = NULL;

  free(sort);
}

void fsort_form_keys(FSortObj_t *sort) {
  array(const char *) ffKeyOpts = array_init();
  array(const char *) psKeyOpts = array_init();

  array_push(ffKeyOpts, sort->idSet);
  array_push(ffKeyOpts, sort->order);

  array_push(psKeyOpts, sort->idSet);
  array_push(psKeyOpts, sort->order);
  
  size_t filterSize = json_object_size(sort->_json);

  if (sort->metaKey != NULL) {
    if (sort->hashKey != NULL) {
      array_push(ffKeyOpts, sort->metaKey);
      array_push(psKeyOpts, sort->metaKey);
      array_push(ffKeyOpts, sort->hashKey);
      array_push(psKeyOpts, sort->hashKey);
    }

    if (json_object_size(sort->_json) > 0) {
      array_push(ffKeyOpts, RedisModule_StringPtrLen(sort->filter, NULL) );
    }    
  } else if (filterSize >= 1) {
    json_t *idFilter = json_object_get(sort->_json, "#");
    if (json_is_string(idFilter)) {
      json_t *filterString = json_sprintf("{\"#\":\"%s\"}", json_string_value(idFilter));
      const char  * filterChar = json_string_value(filterString);
      array_push(ffKeyOpts, filterChar);
      json_decref(filterString);
    }
  }

  const char * charFflKey = joinStrArray(sort->ctx,ffKeyOpts.data,ffKeyOpts.length, ":");
  const char * charPssKey = joinStrArray(sort->ctx, psKeyOpts.data,psKeyOpts.length, ":");
  
  sort->fflKey = strdup(charFflKey);
  sort->pssKey = strdup(charPssKey);
  free((char *)charFflKey);
  free((char *)charPssKey);
  array_free(ffKeyOpts);
  array_free(psKeyOpts);  
}

HashTable  **fsort_get_meta_hashtable(FSortObj_t *sort, const char **sData, size_t len, const char **fieldsNeeded, size_t fieldsCount, size_t *resLen){
  HashTable **data = malloc(sizeof(HashTable*));
  size_t hashLen = 0;
  for (size_t i = 0; i < len; i++) {
    const char *keyName = sort->metaKey;
    const char *keyValue = sData[i];

    char * metaKeyName = repl_str(keyName, "*", keyValue);
    size_t metaKeyNameLen = strlen(metaKeyName);
    RedisModuleString *metaKeyString = RedisModule_CreateString(sort->ctx, metaKeyName, metaKeyNameLen);
    RedisModule_ThreadSafeContextLock(sort->ctx);
    RedisModuleKey *metaKey = RedisModule_OpenKey(sort->ctx, metaKeyString , REDISMODULE_READ || REDISMODULE_WRITE);
    RedisModule_ThreadSafeContextUnlock(sort->ctx);
    if (RedisModule_KeyType(metaKey) == REDISMODULE_KEYTYPE_HASH) {
      HashTable *dataHash = ht_create(fieldsCount);
      RedisModule_ThreadSafeContextLock(sort->ctx);
      for (size_t j = 0; j < fieldsCount; j++) {
        const char *dbStr;
        size_t dbStrLen = 0;
        RedisModuleString *value;

        if (strcmp(fieldsNeeded[j], "#") == 0) {
          dbStr = strdup(sData[i]);
        } else {
          RedisModule_HashGet(metaKey,REDISMODULE_HASH_NONE|REDISMODULE_HASH_CFIELDS, fieldsNeeded[j], &value, NULL);
          dbStr = strdup(RedisModule_StringPtrLen(value, &dbStrLen));
          
          if (value == NULL) {
            free((char *)dbStr);
            dbStr = strdup("");
          }
          free(value);
        }
        
        ht_put(dataHash, fieldsNeeded[j], dbStr);
        free((char*)dbStr);
      }
      RedisModule_ThreadSafeContextUnlock(sort->ctx);
      hashLen += 1;
      data = realloc(data, hashLen * sizeof(HashTable*)); 
      data[hashLen-1] = dataHash;
    }

    RedisModule_FreeString(sort->ctx,metaKeyString);
    free(metaKeyString);
    free(metaKey);
    free(metaKeyName);
  }
  *resLen = hashLen > 0 ? hashLen : 0;

  return data;
}

KvStr_t ** fsort_get_meta(FSortObj_t *sort, const char **sData, size_t len, size_t *resLen) {
  *resLen = 0;
  KvStr_t **data = malloc(sizeof(KvStr_t*));

  RedisModule_ThreadSafeContextLock(sort->ctx);
  for (size_t i = 0; i < len; i++) {
    const char *keyName = sort->metaKey;
    const char *keyValue = sData[i];

    char * metaKeyName = repl_str(keyName, "*", keyValue);

    RedisModuleString *metaKeyString = RedisModule_CreateString(sort->ctx, metaKeyName, strlen(metaKeyName));
    RedisModuleKey *metaKey = RedisModule_OpenKey(sort->ctx, metaKeyString , REDISMODULE_READ);

    if (RedisModule_KeyType(metaKey) == REDISMODULE_KEYTYPE_HASH) {
      const char *dbStr;
      size_t dbStrLen = 0;
      RedisModuleString *value;
   
      RedisModule_HashGet(metaKey,REDISMODULE_HASH_NONE|REDISMODULE_HASH_CFIELDS, sort->hashKey, &value, NULL);

      *resLen += 1;

      data = realloc(data, sizeof(KvStr_t*) * *resLen);
      
      KvStr_t *r = malloc(sizeof(KvStr_t));
      r->key = strdup(sData[i]);
      
      dbStr = RedisModule_StringPtrLen(value, &dbStrLen);
      if (value == NULL) {
        dbStr = strdup("");
      }
      r->value = strdup(dbStr);
      data[*resLen-1] = r;
      free(value);
    }
    RedisModule_CloseKey(metaKey);
    free(metaKeyString);
    free(metaKeyName);
  }
  RedisModule_ThreadSafeContextUnlock(sort->ctx);
  return data;
}

KvStr_t  **_get_aggregate_fields(json_t *json, size_t *size) {
  KvStr_t **records = malloc(json_object_size(json) * sizeof(KvStr_t*));
  const char * field;
  json_t * json_aggregate;
  size_t arrSize = 0;
  json_object_foreach(json, field, json_aggregate) {
    KvStr_t * r = malloc(sizeof(KvStr_t));
    const char * val = json_string_value(json_aggregate);
    r->key = strdup(field);

    if (val == NULL) {
      r->value =strdup("");
    } else {
      r->value =strdup(val);
    }

    records[arrSize] = r;
    arrSize ++;
  }
  *size = arrSize;
  return records;
}

int _peform_aggr(const char * func,const char * field, json_t *doc, const char * val) {

  if (strcasecmp(func, "sum") == 0) {
      json_int_t jval = 0;
      json_t *vf = json_object_get(doc, field);
      if (vf == NULL) {
        vf = json_object();
        json_integer_set(vf, jval);
        json_object_set(doc, field, vf);
        json_decref(vf);
      } else
        jval = json_integer_value(vf);

      long long llNew = atoll(val);
      jval += llNew;

      json_t *intVal = json_integer(jval);
      json_object_set(doc,field,intVal);
      json_decref(intVal);
  }
  return 1;
}

json_t *fsort_aggregate_data(HashTable **data, KvStr_t **aggr, const char **fneeded, size_t data_size, size_t  fneededsize) {
  json_t *root = json_object();

  for (size_t i = 0; i < data_size; i++) {
    HashTable *rec = data[i];
    
    for (size_t j = 0; j < fneededsize; j++){
        const char *aggrFunc = aggr[j]->value;
        const char *recValue = ht_get(rec, fneeded[j]);
        _peform_aggr(aggrFunc, fneeded[j], root, recValue);
    }
  
  }
  return root;
}

int fsort_respond_list(FSortObj_t *sort, const char *keyStr) {
  fsort_cache_buster(sort, keyStr);
  fsort_key_pexpire(sort, keyStr);
  if (sort->keyOnly == 1) {
    RedisModule_ReplyWithSimpleString(sort->ctx, keyStr);
    return REDISMODULE_OK;
  }

  RedisModule_ThreadSafeContextLock(sort->ctx);
  RedisModuleCallReply *replyCount = RedisModule_Call(sort->ctx, "LLEN", "c", keyStr);
  RedisModuleCallReply *reply = RedisModule_Call(sort->ctx, "LRANGE","cll", keyStr, sort->offset, sort->offset + sort->limit -1);
  RedisModule_ThreadSafeContextUnlock(sort->ctx);

  if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ARRAY) {
    size_t elCount = RedisModule_CallReplyLength(reply);
    RedisModule_ReplyWithArray(sort->ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
    
    for (size_t i = 0; i < elCount; i++) {
      RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
      RedisModule_ReplyWithCallReply(sort->ctx, subreply);
    }
    
    RedisModule_ReplyWithCallReply(sort->ctx, replyCount);
    RedisModule_ReplySetArrayLength(sort->ctx, elCount + 1);

    RedisModule_FreeCallReply(reply);
    RedisModule_FreeCallReply(replyCount);

    return REDISMODULE_OK;
  } else {
    RedisModule_FreeCallReply(reply);
    RedisModule_FreeCallReply(replyCount);

    return REDISMODULE_ERR;
  }
}

int fsort_sort_data(FSortObj_t *sort) {
  const char **dataToSort = sort->sortData->values;
  size_t elCount = sort->sortData->size;
  
  if (sort->metaKey != NULL && sort->hashKey != NULL) {
    size_t metaSize = 0;
    KvStr_t **data = fsort_get_meta(sort, dataToSort, elCount, &metaSize);
    
    if (strcasecmp(sort->order, "asc")) {
      qsort(data, metaSize, sizeof(KvStr_t*), fsort_compare_meta_asc );
    } else {
      qsort(data, metaSize, sizeof(KvStr_t*), fsort_compare_meta_desc );
    }

    for(size_t i = 0; i<metaSize; i++) {
      KvStr_t *d = data[i];
      free((char *)dataToSort[i]);
      dataToSort[i] = strdup(d->key);
      kvstr_free(data[i]);
    }
    free(data);
    
  } else {
    if (strcasecmp(sort->order, "asc") == 0) {
      qsort(dataToSort, elCount, sizeof(const char*), fsort_compare_string_asc ); 
    } else {
      qsort(dataToSort, elCount, sizeof(const char*), fsort_compare_string_desc ); 
    }
  }

  return 0;
}

const char **fsort_load_idset_data(FSortObj_t *sort, size_t *dataSize) {
  
  RedisModule_ThreadSafeContextLock(sort->ctx);
  RedisModuleCallReply *reply = RedisModule_Call(sort->ctx, "SMEMBERS", "c", sort->idSet);
  RedisModule_ThreadSafeContextUnlock(sort->ctx);

  size_t elCount = 0;
  const char **data = NULL;

  if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ARRAY) {
    elCount = RedisModule_CallReplyLength(reply);
    if (elCount > 0) {
      data = malloc(elCount * sizeof(const char*));
      for (size_t i = 0; i < elCount; i++) {
        RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
        RedisModuleString *strrep = RedisModule_CreateStringFromCallReply(subreply);
        size_t loadLen = 0;
        data[i] = strdup(RedisModule_StringPtrLen(strrep,&loadLen));
        RedisModule_FreeString(sort->ctx, strrep);
      }
      *dataSize = elCount;
    } 

  } else {
    *dataSize = elCount;
  }

  RedisModule_FreeCallReply(reply);
  return data;
}

const char **fsort_load_pss_data(FSortObj_t *sort,size_t *dataSize) {
  const char **data;
 
  RedisModule_ThreadSafeContextLock(sort->ctx);
  RedisModuleCallReply *reply = RedisModule_Call(sort->ctx, "LRANGE", "cll", sort->pssKey, 0, -1);
  RedisModule_ThreadSafeContextUnlock(sort->ctx);
 
  if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ARRAY) {
    size_t ds = 0;
    ds = RedisModule_CallReplyLength(reply);
    if (ds > 0) {
      data = malloc(ds * sizeof(const char*));

      for (size_t i = 0; i < ds; i++) {
        RedisModuleCallReply *subreply = RedisModule_CallReplyArrayElement(reply, i);
        RedisModuleString *strrep = RedisModule_CreateStringFromCallReply(subreply);
        data[i] = strdup(RedisModule_StringPtrLen(strrep,NULL));  
        free(strrep);
      }  
    }

    *dataSize = ds;
    RedisModule_FreeCallReply(reply);
    return ds > 0 ? data: NULL;
    
  } else {
    RedisModule_FreeCallReply(reply);
    RedisModule_ReplyWithError(sort->ctx, "Unable to LRANGE PreSortedKey!");
    return NULL;
  }
}

const char **fsort_filter_data(FSortObj_t *sort, size_t *outputDataSize) {
  const char **dataToSort = sort->sortData->values;
  size_t elCount = sort->sortData->size;
  size_t ds = 0;
  const char **outputData = malloc(sizeof(const char *));

  if (!sort->metaKey) {
    json_t *idFilter = json_object_get(sort->_json, "#");
    const char * idFilterStr = json_string_value(idFilter);

    for (size_t i = 0; i < elCount; i++){
      if (filter_match_string(dataToSort[i],idFilterStr)==1) {
        ds += 1;
        outputData = realloc(outputData,ds * sizeof(const char*));
        outputData[ds-1] = strdup(dataToSort[i]); 
      }
    }
  } else {
      size_t dataHashesLen = 0;
      HashTable **dataHashes = fsort_get_meta_hashtable(sort, dataToSort, elCount, sort->filters->fieldsUsed, sort->filters->fieldsCount, &dataHashesLen);
      if (dataHashesLen > 0) {        
        for (size_t i = 0; i < dataHashesLen; i++){
          if (filter_filter_match(dataHashes[i], sort->filters) == 1) {
            ds += 1;
            outputData = realloc(outputData, ds * sizeof(const char*));
            outputData[ds -1] = strdup(dataToSort[i]); 
          }
        }

        for (size_t i = 0; i < dataHashesLen; i++) {
          ht_free(dataHashes[i]);
        }
      } else {
        ds = 0;
      }
      free(dataHashes);      
  }

  *outputDataSize = ds;
  return outputData;
}

int fsort_key_type(FSortObj_t *sort, const char *keyStr) {
  RedisModuleString *keyName = fsort_char_rms(sort, keyStr);
  RedisModuleKey *key = RedisModule_OpenKey(sort->ctx, keyName, REDISMODULE_READ);
  int kType = RedisModule_KeyType(key);
  RedisModule_CloseKey(key);
  RedisModule_FreeString(sort->ctx, keyName);
  return kType;
}

int fsort_fsort(FSortObj_t *sort) {
  sort->fflKeyName = fsort_char_rms(sort, sort->fflKey); 
  sort->pssKeyName = fsort_char_rms(sort, sort->pssKey);  
  
  RedisModule_ThreadSafeContextLock(sort->ctx);
      
  RedisModuleKey *fflkey = RedisModule_OpenKey(sort->ctx, sort->fflKeyName, REDISMODULE_READ);
  RedisModuleKey *psskey = RedisModule_OpenKey(sort->ctx, sort->pssKeyName, REDISMODULE_READ);

  int pssKType = RedisModule_KeyType(psskey);
  int fflKType = RedisModule_KeyType(fflkey);

  RedisModule_CloseKey(fflkey);
  RedisModule_CloseKey(psskey);
  RedisModule_ThreadSafeContextUnlock(sort->ctx);

  if (fflKType == REDISMODULE_KEYTYPE_LIST) {
    fsort_key_pexpire(sort, sort->pssKey);
    fsort_cache_buster(sort, sort->pssKey);
    return fsort_respond_list(sort, sort->fflKey);
  } 

  if (pssKType == REDISMODULE_KEYTYPE_LIST) {
    //Respond pss
    if (strcasecmp(sort->fflKey, sort->pssKey) == 0) {
      return fsort_respond_list(sort, sort->pssKey);
    }

    //Load pss cache if sort requested
    size_t sdataSize = 0;
    const char **data = fsort_load_pss_data(sort, &sdataSize);
    
    if (sdataSize > 0) {
      sort->sortData->values = data;
    }

    sort->sortData->size = sdataSize;
    
    if (sort->sortData->size == 0) {
      return REDISMODULE_OK;
    }

  } else  {
    size_t loadData = 0;
    const char **data =  fsort_load_idset_data(sort, &loadData);
    if (loadData > 0) {
      sort->sortData->values = data;
      sort->sortData->size = loadData;
    } else {
      sort->sortData->size = 0;
    }
    
    if (sort->sortData->size > 0) {
      fsort_sort_data(sort);        
      //insert into db for next use
      if (fsort_key_type(sort, sort->pssKey) == REDISMODULE_KEYTYPE_EMPTY) {  
        RedisModule_ThreadSafeContextLock(sort->ctx);
        psskey = RedisModule_OpenKey(sort->ctx, sort->pssKeyName, REDISMODULE_WRITE);       
        for(size_t i = 0; i< sort->sortData->size; i++) {
          const char **v = sort->sortData->values;
          RedisModuleString *insVal = RedisModule_CreateString(sort->ctx,v[i], strlen(v[i]));
          RedisModule_ListPush(psskey, REDISMODULE_LIST_TAIL, insVal);
          RedisModule_FreeString(sort->ctx, insVal);
        }
        RedisModule_CloseKey(psskey);
        RedisModule_ThreadSafeContextUnlock(sort->ctx);
      } else {
        RM_LOG_VERBOSE(sort->ctx,"Key Already created key %s ommiting write", sort->pssKey);
      }
      fsort_key_pexpire(sort, sort->pssKey);
      fsort_cache_buster(sort, sort->pssKey);
    } else {
     
      if (sort->keyOnly == 1) {
        RedisModule_ReplyWithSimpleString(sort->ctx, sort->pssKey);
        return REDISMODULE_OK;
      }
      RedisModule_ReplyWithLongLong(sort->ctx, 0);
      return REDISMODULE_OK;
    }

    if (strcmp(sort->fflKey, sort->pssKey) == 0) {
      return fsort_respond_list(sort, sort->pssKey);
    }

    fsort_key_pexpire(sort, sort->pssKey);
    fsort_cache_buster(sort, sort->pssKey);
  }
    

  sort->outputData->values = fsort_filter_data(sort, &sort->outputData->size);
  size_t outputDataSize = sort->outputData->size;

  const char** outputData = sort->outputData->values;
    
  if (outputDataSize > 0) {
    if (fsort_key_type(sort, sort->fflKey) == REDISMODULE_KEYTYPE_EMPTY) {  
      RedisModule_ThreadSafeContextLock(sort->ctx);
    
      fflkey = RedisModule_OpenKey(sort->ctx, sort->fflKeyName, REDISMODULE_WRITE);
        
      for(size_t i = 0; i<outputDataSize; i++) {
        RedisModuleString *insStr = RedisModule_CreateString(sort->ctx, strdup(outputData[i]),strlen(outputData[i]));
        RedisModule_ListPush(fflkey, REDISMODULE_LIST_TAIL, insStr);
        free(insStr);
      }

      RedisModule_CloseKey(fflkey);
      RedisModule_ThreadSafeContextUnlock(sort->ctx);
    } else {
      RM_LOG_VERBOSE(sort->ctx,"Filter Key already created created key %s: ommiting write", sort->fflKey);
    }
    return fsort_respond_list(sort, sort->fflKey);
  }

  if (sort->keyOnly == 1) {
    RedisModule_ReplyWithSimpleString(sort->ctx, sort->fflKey);
    return REDISMODULE_OK;
  }

  RedisModule_ReplyWithArray(sort->ctx,1);
  RedisModule_ReplyWithLongLong(sort->ctx, 0);
  return REDISMODULE_OK;

}

int fsort_fsort_aggregate(FSortObj_t *sort) {
  size_t dataLen = 0;
  const char **data = fsort_load_pss_data(sort, &dataLen);
  
  if (dataLen >0) {
    sort->sortData->values = data;
    sort->sortData->size = dataLen;

    size_t fneededLen=0;
    KvStr_t **aggregateKv = _get_aggregate_fields(sort->_json, &fneededLen);
    const char **fneeded = calloc(fneededLen, sizeof(const char *));
 
    for (size_t i = 0; i < fneededLen; i++){
      fneeded[0] = aggregateKv[i]->key;
    }
    
    if (fneededLen >0) {
      size_t metaDataLen = 0;
      HashTable **metaData = fsort_get_meta_hashtable(sort, data, dataLen, fneeded, fneededLen, &metaDataLen);
      json_t *obj = fsort_aggregate_data(metaData, aggregateKv, fneeded, metaDataLen, fneededLen);
      
      const char *json_encoded = _encode_json(obj);
      json_decref(obj);
      RedisModule_ReplyWithSimpleString(sort->ctx, json_encoded);

      for (size_t i = 0; i < metaDataLen; i++){
        ht_free(metaData[i]);
      }

      for (size_t i = 0; i < fneededLen; i++){
        kvstr_free(aggregateKv[i]);
      }
      free(metaData);
      free(aggregateKv);
      free((void*)json_encoded);
      
      free(fneeded);
      return REDISMODULE_OK;
    }
  }
  RedisModule_ReplyWithSimpleString(sort->ctx,"{}");
  return REDISMODULE_ERR;
}

int fsort_fsort_bust(RedisModuleCtx * ctx, RedisModuleString **argv, int argc) {
  RedisModuleString *idSetKey = RedisModule_CreateStringPrintf(ctx, "%s::fsort_temp_keys", RedisModule_StringPtrLen(argv[1], NULL) );
  
  long long expire = 30000;
  if (argc > 3) fsort_read_long_param(argv[3], 30000);

  long long curtime = fsort_read_long_param(argv[2], 0);


  RedisModuleCallReply *reply = RedisModule_Call(ctx, "ZRANGEBYSCORE", "slc", idSetKey, curtime-expire, "+inf");
  int rKeyType = RedisModule_CallReplyType(reply);

  if (rKeyType == REDISMODULE_REPLY_ARRAY) {
    size_t elCount = RedisModule_CallReplyLength(reply);
    if (elCount > 0) {
      RedisModule_ThreadSafeContextLock(ctx);

      for (size_t i = 0; i < elCount; i++) {
        RedisModuleCallReply *keyNameReply = RedisModule_CallReplyArrayElement(reply, i);
        RedisModuleString *keyName = RedisModule_CreateStringFromCallReply(keyNameReply);
        RedisModuleCallReply *rDel = RedisModule_Call(ctx, "del", "s", keyName);
        RedisModule_FreeCallReply(rDel);
        RedisModule_FreeString(ctx, keyName);
      }

      RedisModuleCallReply *rDelSet = RedisModule_Call(ctx, "del", "s", idSetKey);
      RedisModule_FreeCallReply(rDelSet);
      RedisModule_FreeString(ctx, idSetKey);

      RedisModule_ThreadSafeContextUnlock(ctx);
    }
  } else {
    RedisModule_FreeCallReply(reply);
    return REDISMODULE_ERR;
  }
  
  RedisModule_FreeCallReply(reply);   
  RedisModule_ReplyWithLongLong(ctx, 0);
  return REDISMODULE_OK;
}

void *fsort_bust_thread(void *arg) {
  void **targ = arg;
  RedisModuleBlockedClient *bc = (RedisModuleBlockedClient*)targ[0];
  RedisModuleCtx *ctx = RedisModule_GetThreadSafeContext(bc);

  int argc = (unsigned long)targ[1];
  RedisModuleString **argv = targ[2];
  fsort_fsort_bust(ctx, argv, argc);
  
  RedisModule_UnblockClient(bc,NULL);
  RedisModule_FreeThreadSafeContext(ctx);

  return NULL;
}

void *fsort_aggregate_thread(void *arg) {
  void **targ = (void*)arg;
  RedisModuleBlockedClient *bc = (RedisModuleBlockedClient*)targ[0];
  //int argc = (unsigned long)targ[1];
  RedisModuleString **argv = targ[2];

  RedisModuleCtx *ctx = RedisModule_GetThreadSafeContext(bc); 

  RedisModuleString *aggregates = argv[3];
  
  FSortObj_t *sort = fsort_new_fsort();
  sort->ctx = ctx;
  sort->pssKey = strdup(RedisModule_StringPtrLen(argv[1], NULL));
  sort->metaKey = strdup(RedisModule_StringPtrLen(argv[2], NULL));
  sort->idSet = NULL;
  sort->fflKey = NULL;
  sort->tempKeysSet = NULL;
  
  sort->hashKey = strdup("");
  size_t json_str_len = 0;
  
  json_error_t *json_err = NULL;
  const char * filterString = strdup(RedisModule_StringPtrLen(aggregates, &json_str_len));
  
  if (json_str_len == 0) {
    RedisModule_ReplyWithError(sort->ctx, "Incorrect aggregate parameter");
    RedisModule_UnblockClient(bc, NULL);
    return NULL;
  }

  sort->_json = json_loads(filterString, json_str_len, json_err);
  if (json_err != NULL) {
    RM_LOG_WARNING(ctx, "Json parse error: %s", json_error_code(json_err));
  }

  fsort_fsort_aggregate(sort);  
  fsort_free_fsort(sort);
  RedisModule_UnblockClient(bc, NULL);
  RedisModule_FreeThreadSafeContext(ctx);

  return NULL;
}

void *fsort_fsort_thread(void *arg) {
  void **targ = (void*)arg;
  RedisModuleBlockedClient *bc =targ[0];
  FSortObj_t *sort = targ[1];
  
  RedisModuleCtx *ctx = RedisModule_GetThreadSafeContext(bc); 
  sort->ctx = ctx;
  fsort_fsort(sort);

  RedisModule_FreeThreadSafeContext(ctx);
  fsort_free_fsort(sort);

  RedisModule_UnblockClient(bc, NULL);
  
  return NULL;
} 


