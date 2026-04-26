#include "task_queue.h"
#include <stdlib.h>

bool task_queue_init(task_queue_t *queue, size_t capacity) {
  if (capacity == 0 || !queue) {
    return false;
  }
  queue->ring_buffer = (task_t *)malloc(sizeof(task_t) * capacity);
  if (!queue->ring_buffer) {
    return false;
  }
  queue->capacity = capacity;
  queue->count = 0;
  queue->head = 0;
  queue->tail = 0;
  return true;
}

void task_queue_destroy(task_queue_t *queue) {
  if (!queue) {
    return;
  }
  free(queue->ring_buffer);
  queue->ring_buffer = NULL;
  queue->capacity = 0;
  queue->count = 0;
  queue->head = 0;
  queue->tail = 0;
}

bool task_queue_try_push(task_queue_t *queue, const task_t *task) {
  if (!queue->ring_buffer || !task || !queue) {
    return false;
  }

  if (queue->count == queue->capacity) {
    return false;
  }
  queue->ring_buffer[queue->head] = *task;
  queue->head = (queue->head + 1) % queue->capacity;
  ++queue->count;
  return true;
}

bool task_queue_try_pop(task_queue_t *queue, task_t *out_task) {
  if (!queue->ring_buffer || !out_task || !queue) {
    return false;
  }
  if (queue->count == 0) {
    return false;
  }
  *out_task = queue->ring_buffer[queue->tail];
  queue->tail = (queue->tail + 1) % queue->capacity;
  --queue->count;
  return true;
}