#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include "general.h"
#include <pthread.h>
#include <stdbool.h>

typedef void *(*thread_func_t)(void *arg);

struct FSortWork {
  thread_func_t func;
  void *argv;
  struct FSortWork *next;
};

typedef struct FSortWork FSortWork_t;

typedef int (*tpool_finish_cb_t)(FSortWork_t *work);

typedef struct FSortPool {
    FSortWork_t    *work_first;
    FSortWork_t    *work_last;
    pthread_mutex_t  work_mutex;
    pthread_cond_t   working_cond;
    size_t           working_cnt;
    size_t           thread_cnt;
    tpool_finish_cb_t cb;
    bool             stop;
    pthread_cond_t   work_cond;
} FSortPool_t;

FSortPool_t *tpool_create(size_t num);
bool tpool_add_work(FSortPool_t *tm, thread_func_t func, void *arg);
void tpool_wait(FSortPool_t *tm);
void tpool_work_destroy(FSortWork_t *work);

#endif