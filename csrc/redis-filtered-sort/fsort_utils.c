#include "fsort_utils.h"

KvStrArr_t *kvstr_arr_create() {
  KvStrArr_t *kvstr=malloc(sizeof(KvStrArr_t));
  kvstr->size = 0;
  kvstr->values = malloc(sizeof(KvStr_t*));
  return kvstr;
}

int kvstr_arr_insert(KvStrArr_t *arr, KvStr_t *v) {
  arr->size +=1;
  arr->values = realloc(arr->values, sizeof(KvStr_t*) * (arr->size));
  arr->values[arr->size-1] = v;
  return 1;
}

int kvstr_arr_free(KvStrArr_t *arr) {
  for (size_t i = 0; i < arr->size; i++) {
    free(arr->values[i]);
  }
  free(arr);
  arr = NULL;
  return 1;
}

StrArr_t *str_arr_create() {
  StrArr_t *strarr=malloc(sizeof(StrArr_t));
  strarr->size = 0;
  strarr->values = NULL;
  return strarr;
}

int str_arr_insert(StrArr_t *arr, const char*v) {
  arr->size +=1;
  if (arr->values == NULL) {
    arr->values = malloc(sizeof(const char *) * (arr->size));
  } else {
    arr->values = realloc(arr->values, sizeof(const char *) * (arr->size));
  }
  arr->values[arr->size-1] = v;
  return 1;
}

int str_arr_free(StrArr_t *arr) {
  for (size_t i = 0; i < arr->size; i++) {
    if (arr->values[i] != NULL)
      free((char *)arr->values[i]);
  }
  free(arr->values);
  free(arr);
  arr = NULL;
  return 1;
}


KvHashArr_t *kvhash_arr_create() {
  KvHashArr_t *kvhash=malloc(sizeof(KvHashArr_t));
  kvhash->size = 0;
  kvhash->values = malloc(sizeof(HashTable*));
  return kvhash;
}

int kvhash_arr_insert(KvHashArr_t *arr, HashTable *v) {
  arr->size +=1;
  arr->values = realloc(arr->values, sizeof(HashTable*) * (arr->size));
  arr->values[arr->size-1] = v;
  return 1;
}

int kvhash_arr_free(KvHashArr_t *arr) {
  for (size_t i = 0; i < arr->size; i++) {
    ht_free(arr->values[i]);
  }
  free(arr);
  arr = NULL;
  return 1;
}

int kvstr_free(KvStr_t *str) {
  free((char*)str->key);
  free((char*)str->value);
  free(str);
  return 1;
}

const char * GetArgvString(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {

  RedisModuleString *s = RedisModule_CreateString(ctx, "", 0);
  int i;
  for (i = 0; i < argc; i++) {
      size_t arglen;
      const char *arg = RedisModule_StringPtrLen(argv[i], &arglen);

      if (i >= 1) RedisModule_StringAppendBuffer(ctx, s, "!", 1);
      RedisModule_StringAppendBuffer(ctx, s, arg, arglen);
  }
  
  size_t strLen;
  return RedisModule_StringPtrLen(s, &strLen);
}

char *joinStrArray(RedisModuleCtx *ctx, const char **data, size_t len, char *delim) {
  
  char *result = RedisModule_Strdup("");
  size_t delimLength = strlen(delim);
  size_t strLength = 0;

  for (size_t i = 0; i < len; i++) {
    const char *el = data[i];
    if (strlen(el)>0) {
      if (i > 0) {
        strLength = strlen(result)+delimLength+1;
        result = RedisModule_Realloc(result, strLength);
        strcat(result, delim);    
      }
      strLength = strlen(result)+strlen(el)+1;
      result = RedisModule_Realloc(result, strLength);
      strcat(result, el);
    }
  }
  result[strLength -1] = '\0';
  return result;
}

const char *fsort_read_string_param(RedisModuleString *arg) {
  size_t _strSize;
  const char * resStr;
  const char * str = RedisModule_StringPtrLen(arg,&_strSize);
  if (_strSize < 1) {
    resStr = NULL;
  } else {
    resStr = strdup(str);
  }
  return resStr; 
}

long long fsort_read_long_param(RedisModuleString *str, long long def) {
  long long num = 0;
  
  if (str){
    RedisModule_StringToLongLong(str, &num);
  }
  
  return num != 0 ? num : def;
}

int fsort_compare_string_desc(const void *a, const void *b) {
  const char *aStr = *(const char**)a;
  const char *bStr = *(const char**)b;
  return strcasecmp(bStr, aStr);
}

int fsort_compare_string_asc(const void *a, const void *b) {
  const char *aStr = *(const char**)a;
  const char *bStr = *(const char**)b;
  return strcasecmp(aStr, bStr);
}

int fsort_compare_meta_desc(const void *a, const void *b) {
  KvStr_t *mrA = *(KvStr_t**)a;
  KvStr_t *mrB = *(KvStr_t**)b;
  return strcasecmp(mrA->value, mrB->value);
}


int fsort_compare_meta_asc(const void *a, const void *b) {
  KvStr_t *mrA = *(KvStr_t**)a;
  KvStr_t *mrB = *(KvStr_t**)b;
  return strcasecmp(mrB->value, mrA->value);
}

const char * _encode_json(json_t * obj) {
  size_t size = json_dumpb(obj, NULL, 0, 0);
  if (size == 0)
    return NULL;

  char *buf = malloc(size+1);

  size = json_dumpb(obj, buf, size, 0);
  if (size == 0) {
    free(buf);
    return strdup("{}");
  }
  buf[size] = '\0';
  return buf;
}
