#pragma once

#include <pthread.h>
#include "task_queue.h"

typedef struct _thread_pool {

  pthread_t *workers;

  task_queue_t readers_queue;
  task_queue_t writers_queue;

  pthread_cond_t has_work;
  pthread_cond_t has_space;
  pthread_cond_t all_done;

  pthread_mutex_t pool_mutex;

  size_t worker_count;
  size_t active_tasks;
  bool stopping;

} thread_pool_t;

bool thread_pool_enqueue(thread_pool_t *pool, const task_t *task);

bool thread_pool_try_enqueue(thread_pool_t *pool, const task_t *task);
