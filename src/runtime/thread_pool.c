#include "thread_pool.h"

#include <stdlib.h>

static bool has_runnable_tasks(const thread_pool_t *pool) {
  return (pool->writers_queue.count != 0U &&
          pool->active_writers < pool->max_active_writers) ||
         (pool->computers_queue.count != 0U &&
          pool->active_computers < pool->max_active_computers) ||
         (pool->readers_queue.count != 0U &&
          pool->active_readers < pool->max_active_readers);
}

static bool has_queued_tasks(const thread_pool_t *pool) {
  return pool->readers_queue.count != 0U || pool->writers_queue.count != 0U ||
         pool->computers_queue.count != 0U;
}

static void destroy_sync_objects(thread_pool_t *pool) {
  pthread_cond_destroy(&pool->all_done);
  pthread_cond_destroy(&pool->has_space);
  pthread_cond_destroy(&pool->has_work);
  pthread_mutex_destroy(&pool->pool_mutex);
}

static void *worker_main(void *args);

bool thread_pool_init(thread_pool_t *pool, const thread_pool_config_t *config) {
  if (pool == NULL || config == NULL || config->worker_count == 0U ||
      config->reader_queue_capacity == 0U ||
      config->writer_queue_capacity == 0U ||
      config->computer_queue_capacity == 0U ||
      config->max_active_readers == 0U || config->max_active_writers == 0U ||
      config->max_active_computers == 0U) {
    return false;
  }

  pool->workers = NULL;
  pool->worker_count = 0U;
  pool->active_tasks = 0U;
  pool->active_readers = 0U;
  pool->active_writers = 0U;
  pool->active_computers = 0U;
  pool->max_active_readers = config->max_active_readers;
  pool->max_active_writers = config->max_active_writers;
  pool->max_active_computers = config->max_active_computers;
  pool->stopping = false;

  if (!task_queue_init(&pool->readers_queue, config->reader_queue_capacity)) {
    return false;
  }
  if (!task_queue_init(&pool->writers_queue, config->writer_queue_capacity)) {
    task_queue_destroy(&pool->readers_queue);
    return false;
  }
  if (!task_queue_init(&pool->computers_queue,
                       config->computer_queue_capacity)) {
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }
  if (pthread_mutex_init(&pool->pool_mutex, NULL) != 0) {
    task_queue_destroy(&pool->computers_queue);
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }
  if (pthread_cond_init(&pool->has_work, NULL) != 0) {
    pthread_mutex_destroy(&pool->pool_mutex);
    task_queue_destroy(&pool->computers_queue);
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }
  if (pthread_cond_init(&pool->has_space, NULL) != 0) {
    pthread_cond_destroy(&pool->has_work);
    pthread_mutex_destroy(&pool->pool_mutex);
    task_queue_destroy(&pool->computers_queue);
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }
  if (pthread_cond_init(&pool->all_done, NULL) != 0) {
    pthread_cond_destroy(&pool->has_space);
    pthread_cond_destroy(&pool->has_work);
    pthread_mutex_destroy(&pool->pool_mutex);
    task_queue_destroy(&pool->computers_queue);
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }

  pool->workers = malloc(sizeof(*pool->workers) * config->worker_count);
  if (pool->workers == NULL) {
    destroy_sync_objects(pool);
    task_queue_destroy(&pool->computers_queue);
    task_queue_destroy(&pool->writers_queue);
    task_queue_destroy(&pool->readers_queue);
    return false;
  }

  for (size_t i = 0; i < config->worker_count; ++i) {
    if (pthread_create(&pool->workers[i], NULL, worker_main, pool) != 0) {
      thread_pool_destroy(pool);
      return false;
    }
    ++pool->worker_count;
  }

  return true;
}

bool thread_pool_queue_for_task(const task_t *task,
                                task_queue_t **q,
                                thread_pool_t *pool) {
  if (task == NULL || q == NULL || pool == NULL || task->fn == NULL) {
    return false;
  }
  if (task->fn_type == WRITER_TASK) {
    *q = &pool->writers_queue;
    return true;
  }
  if (task->fn_type == COMPUTE_TASK) {
    *q = &pool->computers_queue;
    return true;
  }
  if (task->fn_type == READER_TASK) {
    *q = &pool->readers_queue;
    return true;
  }
  return false;
}

bool thread_pool_has_pending_tasks(thread_pool_t *pool) {
  return pool != NULL && (has_queued_tasks(pool) || pool->active_tasks != 0U);
}

bool thread_pool_enqueue(thread_pool_t *pool, const task_t *task) {
  task_queue_t *q = NULL;
  if (!thread_pool_queue_for_task(task, &q, pool)) {
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
  task_queue_t *q = NULL;
  if (!thread_pool_queue_for_task(task, &q, pool)) {
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

bool pool_thread_pop_task(thread_pool_t *pool, task_t *out_task) {
  if (pool == NULL || out_task == NULL) {
    return false;
  }
  if ((pool->active_writers < pool->max_active_writers) &&
      task_queue_try_pop(&pool->writers_queue, out_task)) {
    return true;
  }
  if ((pool->active_computers < pool->max_active_computers) &&
      task_queue_try_pop(&pool->computers_queue, out_task)) {
    return true;
  }
  return ((pool->active_readers < pool->max_active_readers) &&
          task_queue_try_pop(&pool->readers_queue, out_task));
}

static void *worker_main(void *args) {
  thread_pool_t *pool = args;

  for (;;) {
    task_t task;

    pthread_mutex_lock(&pool->pool_mutex);
    while (!has_runnable_tasks(pool) && !pool->stopping) {
      pthread_cond_wait(&pool->has_work, &pool->pool_mutex);
    }
    if (pool->stopping && !has_queued_tasks(pool)) {
      pthread_mutex_unlock(&pool->pool_mutex);
      return NULL;
    }

    if (!pool_thread_pop_task(pool, &task)) {
      pthread_mutex_unlock(&pool->pool_mutex);
      continue;
    }

    ++pool->active_tasks;
    if (task.fn_type == READER_TASK) {
      ++pool->active_readers;
    } else if (task.fn_type == WRITER_TASK) {
      ++pool->active_writers;
    } else {
      ++pool->active_computers;
    }
    pthread_cond_signal(&pool->has_space);
    pthread_mutex_unlock(&pool->pool_mutex);

    task.fn(task.args);

    pthread_mutex_lock(&pool->pool_mutex);
    --pool->active_tasks;
    if (task.fn_type == READER_TASK) {
      --pool->active_readers;
    } else if (task.fn_type == WRITER_TASK) {
      --pool->active_writers;
    } else {
      --pool->active_computers;
    }
    pthread_cond_broadcast(&pool->has_work);
    if (!thread_pool_has_pending_tasks(pool)) {
      pthread_cond_broadcast(&pool->all_done);
    }
    pthread_mutex_unlock(&pool->pool_mutex);
  }
}

void thread_pool_wait(thread_pool_t *pool) {
  if (pool == NULL) {
    return;
  }

  pthread_mutex_lock(&pool->pool_mutex);
  while (thread_pool_has_pending_tasks(pool)) {
    pthread_cond_wait(&pool->all_done, &pool->pool_mutex);
  }
  pthread_mutex_unlock(&pool->pool_mutex);
}

void thread_pool_shutdown(thread_pool_t *pool) {
  if (pool == NULL) {
    return;
  }

  pthread_mutex_lock(&pool->pool_mutex);
  pool->stopping = true;
  pthread_cond_broadcast(&pool->has_work);
  pthread_cond_broadcast(&pool->has_space);
  pthread_mutex_unlock(&pool->pool_mutex);
}

void thread_pool_destroy(thread_pool_t *pool) {
  if (pool == NULL) {
    return;
  }

  thread_pool_shutdown(pool);
  for (size_t i = 0; i < pool->worker_count; ++i) {
    pthread_join(pool->workers[i], NULL);
  }

  free(pool->workers);
  pool->workers = NULL;
  pool->worker_count = 0U;

  destroy_sync_objects(pool);
  task_queue_destroy(&pool->readers_queue);
  task_queue_destroy(&pool->writers_queue);
  task_queue_destroy(&pool->computers_queue);
}
