#pragma once

#include <pthread.h>
#include "task_queue.h"

typedef struct thread_pool_config {
  size_t worker_count;

  size_t reader_queue_capacity;
  size_t writer_queue_capacity;
  size_t computer_queue_capacity;

  size_t max_active_readers;
  size_t max_active_writers;
  size_t max_active_computers;

} thread_pool_config_t;
typedef struct _thread_pool {

  pthread_t *workers;

  task_queue_t readers_queue;
  task_queue_t writers_queue;
  task_queue_t computers_queue;

  pthread_cond_t has_work;
  pthread_cond_t has_space;
  pthread_cond_t all_done;

  pthread_mutex_t pool_mutex;

  size_t active_tasks;

  size_t active_readers;
  size_t active_writers;
  size_t active_computers;

  size_t max_active_readers;
  size_t max_active_writers;
  size_t max_active_computers;

  size_t worker_count;
  bool stopping;

} thread_pool_t;

bool thread_pool_enqueue(thread_pool_t *pool, const task_t *task);

bool thread_pool_has_pending_tasks(thread_pool_t *pool);

bool thread_pool_try_enqueue(thread_pool_t *pool, const task_t *task);

bool thread_pool_queue_for_task(const task_t *task,
                                task_queue_t **q,
                                thread_pool_t *pool);

bool thread_pool_init(thread_pool_t *pool, const thread_pool_config_t *config);

bool pool_thread_pop_task(thread_pool_t *pool, task_t *out_task);

void thread_pool_wait(thread_pool_t *pool);

void thread_pool_destroy(thread_pool_t *pool);

void thread_pool_shutdown(thread_pool_t *pool);
