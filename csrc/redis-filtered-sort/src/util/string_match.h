#ifndef __STRINGMATCH_H
#define __STRINGMATCH_H
//emulates lua pattern search
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
// #include <sys/types.h>

//ptrdiff_t
#include <stddef.h>
#include <err.h>

#define MAXCAPTURES 32
#define CAP_POSITION 2
#define CAP_UNFINISHED -1
#define ESC_CHAR '%'
#define SPECIALS        "^$*+?.([%-"
/* macro to 'unsign' a character */
#define uchar(c)        ((unsigned char)(c))

#if !defined(MAXCCALLS)
#define MAXCCALLS       200
#endif

typedef enum MatchType {
  MATCH_STRING = 101,
  MATCH_RANGE
} MatchType_t;

typedef struct Match {
  MatchType_t type;
  void * value;
} Match_t;

typedef struct MatchRange {
  int start;
  int end;
} MatchRange_t;

typedef struct MatchString {
  const char * value;
  int size;
} MatchString_t;

typedef struct MatchResult {
  size_t size;
  Match_t **data;
  int error_count;
  const char **error;

} MatchResult_t;

typedef struct MatchState {
  int matchdepth;  // control for recursive depth (to avoid C stack overflow)
  const char *src_init; // init of source string
  const char *src_end;  // end ('\0') of source string
  const char *p_end;  // end ('\0') of pattern

  const char **error;
  int error_count;
  MatchResult_t *result;

  int level;  // total number of captures (finished or unfinished)
  struct {
    const char *init;
    ptrdiff_t len;
  } capture[MAXCAPTURES];
} MatchState;


int free_match_result(MatchResult_t *mr);
MatchString_t *match_string_value(Match_t *m);
MatchRange_t *match_range_value(Match_t * m);

MatchResult_t *str_match(const char *s, const char *p);

#endif