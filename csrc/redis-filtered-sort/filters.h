#ifndef __FILTERS_H
#define __FILTERS_H 1

#include "jansson.h"
#include "general.h"

#include "hashtable.h"
#include "regex.h"

typedef enum FilterFunc {
  FILTER_UNK = -1,
  FILTER_SUBFILTER = 0,
  FILTER_MATCH,
  FILTER_GTE,
  FILTER_LTE,
  FILTER_EQ,
  FILTER_NE,
  FILTER_EXISTS,
  FILTER_ISEMPTY,
  FILTER_SOME,
  FILTER_ANY
} FilterFunc_t;

typedef enum FilterType {
  FILTERTYPE_MULTI = 1,
  FILTERTYPE_SCALAR,
  FILTERTYPE_OBJECT,
  FILTERTYPE_ARR,
} FilterType_t;

struct Filter {
  const char * field;
  const char * value;
  struct FilterBlock * subFilters;
  FilterType_t type;
  FilterFunc_t filterFunc;
  const char * err;
};

struct FilterBlock {
  size_t size;
  size_t fieldsCount;
  const char * err;
  const char ** fieldsUsed;
  struct Filter **filters;
};


typedef struct Filter Filter_t;
typedef struct FilterBlock FilterBlock_t;

FilterBlock_t *filter_block_create();
void filter_block_insert(FilterBlock_t *,Filter_t *filter);
void filter_block_add_field(FilterBlock_t *, const char *);
void filter_block_free(FilterBlock_t *);
FilterBlock_t *filter_create_filters(json_t *filter);

int filter_filter_match(HashTable *data, FilterBlock_t * filters);
int filter_match_string(const char *str, const char * str2);

#endif