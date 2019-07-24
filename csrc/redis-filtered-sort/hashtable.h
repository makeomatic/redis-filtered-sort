#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct List{
  const char *key;
  const char *value;
  struct List *next;
} List;

typedef struct HashTable{
  unsigned int size;
  List **array;
} HashTable;


unsigned int hash(const char *key, unsigned int size);


HashTable *ht_create(unsigned int);
int ht_put(HashTable *, const char *, const char *);
int add_begin_list(List **, List *);
int str_cmp(char *, char *);
void node_handler(HashTable *, List *);
const char * ht_get(HashTable *, const char *);
void ht_free(HashTable *);
void print_str(char *str);
int print_char(char c);
void print_num(int n);

#endif