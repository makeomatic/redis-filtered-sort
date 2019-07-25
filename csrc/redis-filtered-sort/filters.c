#include "filters.h"
#include "utils.h"
#include "string_match.h"

void filter_filter_free(Filter_t *filter);

FilterBlock_t *filter_block_create() {
  FilterBlock_t *filterBlock = malloc(sizeof(FilterBlock_t));
  filterBlock->size=0;
  filterBlock->fieldsUsed = malloc(sizeof(const char *));
  filterBlock->fieldsCount = 0;
  filterBlock->filters = NULL;
  filterBlock->err = NULL;
  return filterBlock;
}

void filter_block_free(FilterBlock_t *fb) {
  for (size_t i = 0; i < fb->size; i++){
    filter_filter_free(fb->filters[i]);
  }
  
  if (fb->err != NULL) free((char*)fb->err);
  
  for (size_t i = 0; i < fb->fieldsCount; i++){
    const char *field = fb->fieldsUsed[i];
    free((char *)field);
  }

  free(fb->filters);
  free((void*)fb->fieldsUsed);
  free(fb);
}

Filter_t *filter_filter_create() {
  Filter_t *filter = malloc(sizeof(Filter_t));
  filter->subFilters = filter_block_create();
  filter->value = NULL;
  filter->err = NULL;
  filter->field = NULL;
  return filter;
}


void filter_filter_free(Filter_t *filter) {
  if (filter->subFilters != NULL) {
    filter_block_free(filter->subFilters);
    filter->subFilters = NULL;
  }

  if (filter->err != NULL) {
    free((char*)filter->err);
  }

  if (filter->value != NULL) {
    free((char *) filter->value);
  }

  free((char *)filter->field);
  free(filter);
}

void filter_block_insert(FilterBlock_t *fb, Filter_t *filter) {
  fb->size ++;
  if (fb->filters != NULL) {
    fb->filters = realloc(fb->filters,fb->size * sizeof(Filter_t*));
  } else {
    fb->filters = malloc(fb->size * sizeof(Filter_t*));
  }
  fb->filters[fb->size-1] = filter;
}

void filter_block_add_field(FilterBlock_t *fb, const char * field) {
  fb->fieldsCount ++;
  fb->fieldsUsed = realloc(fb->fieldsUsed, sizeof(const char *) * fb->fieldsCount);
  fb->fieldsUsed[fb->fieldsCount -1] = strdup(field);
}


int filter_match_string(const char *str, const char * expr)  {
  MatchResult_t *res = str_match(strlwr(str), expr);
  if (res->size > 0 && res->error_count == 0) {
    free_match_result(res);
    return(1);
  } 
  free_match_result(res);
  return(0);
}

int _filter_eq(const char * str, const char * str2) { 
  if (strcasecmp(str, str2) == 0) {
    return 1;
  } else {
    return 0;
  }
}

int _filter_ne(const char * str, const char * str2) { 
  if (strcasecmp(str, str2) == 0) {
    return 0;
  } else {
    return 1;
  }
}

int _filter_lte(const char *str, const char *str2 ) {
  long double a = 0,b = 0;
  a = atof(str);
  b = atof(str2);
  
  return a <= b ? 1 : 0;
}

int _filter_gte(const char *str, const char *str2 ) {
  long a = 0,b = 0;
  a = atof(str);
  b = atof(str2);    

  return a >= b ? 1 : 0;
}

// Processes data with selected filter
int _filter_process(HashTable *data, Filter_t *filter, int level) {
  int match = 1;
  
  const char *valueStr = ht_get(data, filter->field);
  
  switch(filter->filterFunc) {
    case FILTER_UNK :
      match = 0;
    break;
    case FILTER_MATCH: {
     match = filter_match_string(valueStr, filter->value);
    }
    break;
    case FILTER_EQ: {
      match = _filter_eq(valueStr, filter->value);
    };
    break;
    case FILTER_NE: {
      match = _filter_ne(valueStr, filter->value);
    }
    break;
    case FILTER_GTE: {
      match = _filter_gte(valueStr, filter->value);
    }
    break;
    case FILTER_LTE: {
      match = _filter_lte(valueStr, filter->value);
    }
    break;
    case FILTER_EXISTS: {
      
      if (strlen(valueStr) == 0) {
        match = 0;
      } else {
        match = 1;
      }
      
    }
    break;
    case FILTER_ISEMPTY: {
      match = strlen(valueStr) == 0 ? 1 : 0;
    }
    break;
    case FILTER_SOME: {
      if (strlen(valueStr) == 0) {
        match = 0;
      }
      match = _filter_eq(valueStr, filter->value);
    }
    break;
    case FILTER_ANY: {
      if (filter->subFilters == NULL) {
        match = 0;
      } else {
        int andMatched = 1;
        level ++;

        for (size_t j = 0; j < filter->subFilters->size; j++) {

          Filter_t *subfilter = filter->subFilters->filters[j];
          int mr = _filter_process(data, subfilter, level);

          if (subfilter->type == FILTERTYPE_SCALAR) {
            if (andMatched && mr) andMatched = 1;
            if (!(andMatched && mr)) andMatched = 0;
          } else {
            if (mr == 1) {
              andMatched = 1;
              break;
            } else {
              andMatched = 0;
            }
          }
        }
        match = andMatched;
      }
    }
    break;
    case FILTER_SUBFILTER:
    break;
  };
  return match;
}

int _filter_process_obj(HashTable *data, Filter_t *filter) {
  int match = 1;
  if (filter->filterFunc == FILTER_ANY || filter->filterFunc == FILTER_SOME) {
    int filterMatch = 0;
    
    FilterBlock_t *anyfilters = filter->subFilters;
    for (size_t i = 0; i < anyfilters->size; i++){
      Filter_t *subFilter = anyfilters->filters[i];
      filterMatch = _filter_process(data, subFilter, 101);
      if (filterMatch == 1) {
        match = filterMatch;
        break;
      }
    }
    match = filterMatch;
    
  }  else {

    for (size_t i = 0; i < filter->subFilters->size; i++){
      Filter_t *subFilter = filter->subFilters->filters[i];
      int mr = _filter_process(data, subFilter, 1);
      if (mr == 0) {
        match = 0;
        break;
      }

    }
  }
  return match;
}


int filter_filter_match(HashTable *data, FilterBlock_t *filterBlock) {
  int matched = 1;
  for (size_t i = 0; i < filterBlock->size; i++) {
    Filter_t *filter = filterBlock->filters[i];

    if (filter->type == FILTERTYPE_SCALAR) {
      matched = _filter_process(data, filter,1);
    } else if (filter->type == FILTERTYPE_OBJECT) {
      matched = _filter_process_obj(data, filter);
    } else if (filter->type == FILTERTYPE_MULTI) {
      int anyMatched = 0;
      for (size_t j = 0; j < filter->subFilters->size; j++) {
        Filter_t *f = filter->subFilters->filters[j];
        anyMatched = _filter_process(data, f,1);
        
        if (anyMatched == 1) break;
      }
      matched = anyMatched;
    }
    if (matched == 0) break;
        
  }
  return matched;
}

FilterFunc_t filter_function_type(const char *str) {
  if (strcasecmp(str, "match") == 0) {
    return FILTER_MATCH;
  } else if (strcasecmp(str, "gte") == 0) {
    return FILTER_GTE;
  } else if (strcasecmp(str, "lte") == 0) {
    return FILTER_LTE;
  } else if (strcasecmp(str, "eq") == 0) {
    return FILTER_EQ;
  } else if (strcasecmp(str, "ne") == 0) {
    return FILTER_NE;
  } else if (strcasecmp(str, "exists") == 0) {
    return FILTER_EXISTS;
  } else if (strcasecmp(str, "isempty") == 0) {
    return FILTER_ISEMPTY;
  } else if (strcasecmp(str, "some") == 0) {
    return FILTER_SOME;
  } else if (strcasecmp(str, "any") == 0) {
    return FILTER_ANY;
  }
  return -1;
}

int filter_create_multi_filter(FilterBlock_t * fb,const char * fieldName, json_t *doc) {
  json_t *mfield;
  json_t *matchJson = json_object_get(doc, "match");

  Filter_t *filter = filter_filter_create();

  filter->field = strdup(fieldName);
  filter->type = FILTERTYPE_MULTI;
  filter->filterFunc = FILTER_SUBFILTER;
  filter->value = strdup(json_string_value(matchJson));
  
  json_t *fieldsKey = json_object_get(doc, "fields");
  int index = 0;

  json_array_foreach(fieldsKey, index, mfield) {
    const char * mfieldStr = json_string_value(mfield);
    filter_block_add_field(fb, mfieldStr);

    Filter_t *mFilter = filter_filter_create();

    mFilter->field = strdup(mfieldStr);
    mFilter->value = strdup(json_string_value(matchJson)); 
    mFilter->filterFunc = FILTER_MATCH;
    mFilter->type = FILTERTYPE_SCALAR;

    filter_block_insert(filter->subFilters, mFilter);
  }

  filter_block_insert(fb, filter);
  return 0;
}

Filter_t *filter_create_scalar_filter(const char* fieldName, const char * filterFunction , json_t *doc){
  Filter_t *subFilter = filter_filter_create();
  subFilter->type = FILTERTYPE_SCALAR;
  subFilter->field = strdup(fieldName);
  filter_block_free(subFilter->subFilters);
  subFilter->subFilters = NULL;

  if (json_typeof(doc) == JSON_REAL) {
    double v = json_real_value(doc);
    subFilter->value = malloc(100);
    sprintf((char *)subFilter->value, "%g", v);

  } else if (json_typeof(doc) == JSON_INTEGER){
    long long v = json_integer_value(doc);
    subFilter->value = malloc(sizeof(long long) + 1);
    sprintf((char *)subFilter->value, "%lli", v);
  } else {
    subFilter->value = strdup(json_string_value(doc));
  }
  
  subFilter->filterFunc = filter_function_type(filterFunction);
  return subFilter;
}

int filter_filter_from_object(Filter_t *filter, const char * fieldName, json_t *doc) {
  const char *jKey;
  json_t *jValue;
  filter->field = strdup(fieldName);

  json_object_foreach(doc, jKey, jValue) {  
    if (json_typeof(jValue) == JSON_OBJECT) {
      const char * akey;
      json_t * adoc;
      filter->filterFunc = filter_function_type(jKey);

      json_object_foreach(jValue, akey, adoc) {
        if (json_typeof(adoc) == JSON_OBJECT) {
          Filter_t *tempFilter = filter_filter_create();
          tempFilter->filterFunc = filter->filterFunc;
          tempFilter->type = filter->type;
          
          int res = filter_filter_from_object(tempFilter, fieldName, adoc );
          
          if (res < 0) {
            filter->err = strdup(tempFilter->err);
            filter_filter_free(tempFilter);
            return res;
          }

          filter_block_insert(filter->subFilters, tempFilter);
        } else {
          Filter_t *subfilter = filter_create_scalar_filter(fieldName, jKey, adoc);
          
          if (subfilter->filterFunc == FILTER_UNK) {
            char * errText = malloc(sizeof(char*)* 512);
            filter->err = errText;

            filter_filter_free(subfilter);
            return -1;
          }

          filter_block_insert(filter->subFilters, subfilter);
        }
      }
    } else {
      Filter_t *subfilter = filter_create_scalar_filter(fieldName, jKey, jValue);

      if (subfilter->filterFunc == FILTER_UNK) {
        char * errText = malloc(sizeof(char*)* 512);
        sprintf(errText, "unknown func: %s", jKey );
        filter->err = errText;
        filter_filter_free(subfilter);
        
        return -1;
      }

      filter_block_insert(filter->subFilters, subfilter);
    } 
  }
  return 0;
}

int filter_create_general_filter(FilterBlock_t *fb, const char * fieldName, json_t *doc, int root) {
  Filter_t *filter; 
  
  if (root)
    filter_block_add_field(fb, fieldName);
  
  int fieldType = json_typeof(doc);
  switch (fieldType) {
    default:{
      filter = filter_create_scalar_filter(fieldName,"match", doc);
    
      if (filter->filterFunc == FILTER_UNK) {
        char * errText = malloc(sizeof(char*)* 512);
        fb->err = errText;
        filter_filter_free(filter);
        return -1;
      }
    }
    break;
    case JSON_OBJECT: {
      //filter from object
      filter = filter_filter_create();
      filter->type = FILTERTYPE_OBJECT;
      filter->filterFunc = FILTER_SUBFILTER;
   
      int res = filter_filter_from_object(filter,fieldName, doc);
      if (res < 0) {
        fb->err = strdup(filter->err);
        filter_filter_free(filter);
        return -1;
      }
    };
    break;
  }
  filter_block_insert(fb, filter);

  return 0;
}

FilterBlock_t *filter_create_filters(json_t *filter) {
  
  FilterBlock_t *filtersArray = filter_block_create();
  
  const char *fieldName;
  json_t *value;
  json_object_foreach(filter, fieldName, value) {
    if (strcasecmp(fieldName, "#multi") == 0) {
      filter_create_multi_filter(filtersArray, fieldName,value);
    } else {
      filter_create_general_filter(filtersArray, fieldName,value,1);
    }
  }

  return filtersArray; 
}
