#include "hashtable.h"
#include "general.h"

HashTable *ht_create(unsigned int size) {
  HashTable *ht;

  if (size < 1) {
    return NULL;
  }

  ht = malloc(sizeof(HashTable));
  if (ht == NULL) {
    return (NULL);
  }

  ht->array = (List**)malloc(size * sizeof(List*));
  if (ht->array == NULL) {
    return NULL;
  }

  memset(ht->array, 0, size * sizeof(List*));

  ht->size = size;

  return ht;
}

void ht_free(HashTable *hashtable) {
  List *tmp;
  unsigned int i;

  if (hashtable == NULL) {
    return;
  }

  for (i = 0; i < hashtable->size; ++i) {
    if (hashtable->array[i] != NULL) {
      /* Traverse the list and free the nodes. */
      while(hashtable->array[i] != NULL) {
        tmp = hashtable->array[i]->next;
        free((char*)hashtable->array[i]->key);
        free((char*)hashtable->array[i]->value);
        free(hashtable->array[i]);
        hashtable->array[i] = tmp;
      }
      free(hashtable->array[i]);
    }
  }
  free(hashtable->array);
  free(hashtable);
}

const char *ht_get(HashTable *hashtable, const char *key) {
  char *key_cp;
  unsigned int i;
  List *tmp;

  if (hashtable == NULL) {
    return NULL;
  }
  key_cp = strdup(key);
  i = hash(key, hashtable->size);
  tmp = hashtable->array[i];

  while (tmp != NULL) {
    if (strcmp(tmp->key, key_cp) == 0) {
      break;
    }
    tmp = tmp->next;
  }
  free(key_cp);

  if (tmp == NULL) {
    return NULL;
  }
  return tmp->value;
}

int ht_put(HashTable *hashtable, const char *key, const char *value)
{
  List *node;

  if (hashtable == NULL) {
    return 1;
  }

  node = malloc(sizeof(List));
  if (node == NULL) {
    return (1);
  }

  node->key = strdup(key);
  node->value = strdup(value);

  node_handler(hashtable, node);

  return 0;
}

void node_handler(HashTable *hashtable, List *node){
  unsigned int i = hash(node->key, hashtable->size);
  List *tmp = hashtable->array[i];

  if (hashtable->array[i] != NULL) {
    tmp = hashtable->array[i];
    while (tmp != NULL) {
      if (strcmp(tmp->key, node->key) == 0) {
        break;
      }
      tmp = tmp->next;
    }
    if (tmp == NULL) {
      node->next = hashtable->array[i];
      hashtable->array[i] = node;
    } else {
      free((void*)tmp->value);
      tmp->value = node->value;
      free((void*)node->value);
      free((void*)node->key);
      free(node);
    }
  } else {
    node->next = NULL;
    hashtable->array[i] = node;
  }
}


unsigned int hash(const char *key, unsigned int size){
  unsigned int hash;
  unsigned int i;

  hash = 0;
  i = 0;
  while (key && key[i])
  {
    hash = (hash + key[i]) % size;
    ++i;
  }
  return (hash);
}

