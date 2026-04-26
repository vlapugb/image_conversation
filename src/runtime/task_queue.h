#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum task_type { READER_TASK = 1, WRITER_TASK } task_type_t;

typedef void (*task_fn_t)(void *args);

typedef struct task {
  task_fn_t fn;
  task_type_t fn_type;
  void *args;
} task_t;

typedef struct _task_queue {

  task_t *ring_buffer;
  size_t capacity;

  size_t head;
  size_t tail;

  size_t count;

} task_queue_t;

bool task_queue_init(task_queue_t *queue, size_t capacity);

void task_queue_destroy(task_queue_t *queue);

bool task_queue_try_pop(task_queue_t *queue, task_t *out_task);

bool task_queue_try_push(task_queue_t *queue, const task_t *task);
