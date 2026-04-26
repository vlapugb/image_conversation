#include "thread_pool.h"

bool thread_pool_enqueue(thread_pool_t *pool, const task_t *task) {
  if (!task || !pool || !task->fn) {
    return false;
  }
  task_queue_t *q = NULL;
  if (task->fn_type == READER_TASK) {
    q = &pool->readers_queue;
  } else if (task->fn_type == WRITER_TASK) {
    q = &pool->writers_queue;
  } else {
    return false;
  }

  pthread_mutex_lock(&pool->pool_mutex);
  while (q->count == q->capacity && !pool->stopping) {
    pthread_cond_wait(&pool->has_space, &pool->pool_mutex);
  }

  if (pool->stopping) {
    pthread_mutex_unlock(&pool->pool_mutex);
    return false;
  }

  const bool pushed = task_queue_try_push(q, task);
  if (pushed) {
    pthread_cond_signal(&pool->has_work);
  }

  pthread_mutex_unlock(&pool->pool_mutex);

  return pushed;
}

bool thread_pool_try_enqueue(thread_pool_t *pool, const task_t *task) {
  if (!task || !pool || !task->fn) {
    return false;
  }
  task_queue_t *q = NULL;
  if (task->fn_type == READER_TASK) {
    q = &pool->readers_queue;
  } else if (task->fn_type == WRITER_TASK) {
    q = &pool->writers_queue;
  } else {
    return false;
  }

  pthread_mutex_lock(&pool->pool_mutex);
  if (pool->stopping || q->count == q->capacity) {
    pthread_mutex_unlock(&pool->pool_mutex);
    return false;
  }

  const bool pushed = task_queue_try_push(q, task);
  if (pushed) {
    pthread_cond_signal(&pool->has_work);
  }

  pthread_mutex_unlock(&pool->pool_mutex);
  return pushed;
}