#ifndef __UTIL_H
#define __UTIL_H 1

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "redismodule.h"

#define array(type)  \
  struct {           \
      type* data;    \
      size_t length; \
  }

#define array_init() \
  {                  \
      .data = NULL,  \
      .length = 0   \
  };

#define array_free(array) \
  do {                    \
        for (size_t i=0; i < array.length; i ++) { \
        RedisModule_Free((char*)array.data[i]); \
      } \
      RedisModule_Free(array.data);   \
      array.data = NULL;  \
      array.length = 0;   \
  } while (0)

#define array_push(array, element)                \
  do {                                            \
      array.data = RedisModule_Realloc(array.data,sizeof(*array.data) * (array.length + 1)); \
      array.data[array.length] = strdup(element);         \
      array.length++;                             \
  } while (0)


char * strlwr(const char *str);
char *repl_str(const char *str, const char *from, const char *to);


#endif