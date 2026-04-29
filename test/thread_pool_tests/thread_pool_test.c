#include <runtime/thread_pool.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

#include <cmocka.h>

#define THREAD_POOL_WORKERS 2U
#define TASK_COUNT 32U

typedef struct counter_task_args {
  size_t *counter;
  pthread_mutex_t *mutex;
} counter_task_args_t;

static void increment_counter(void *args) {
  counter_task_args_t *task_args = args;

  pthread_mutex_lock(task_args->mutex);
  ++(*task_args->counter);
  pthread_mutex_unlock(task_args->mutex);
}

static task_t make_counter_task(counter_task_args_t *args, size_t index) {
  const task_type_t task_types[] = {
    READER_TASK,
    COMPUTE_TASK,
    WRITER_TASK,
  };

  return (task_t){
    .fn = increment_counter,
    .fn_type = task_types[index % (sizeof(task_types) / sizeof(task_types[0]))],
    .args = args,
  };
}

static void thread_pool_runs_all_tasks(void **state) {
  (void)state;

  thread_pool_t pool;
  pthread_mutex_t counter_mutex;
  counter_task_args_t args[TASK_COUNT];
  size_t counter = 0U;

  const thread_pool_config_t config = {
    .worker_count = THREAD_POOL_WORKERS,
    .reader_queue_capacity = TASK_COUNT,
    .computer_queue_capacity = TASK_COUNT,
    .writer_queue_capacity = TASK_COUNT,
    .max_active_readers = 1U,
    .max_active_computers = 1U,
    .max_active_writers = 1U,
  };

  assert_int_equal(pthread_mutex_init(&counter_mutex, NULL), 0);
  assert_true(thread_pool_init(&pool, &config));

  for (size_t i = 0; i < TASK_COUNT; ++i) {
    args[i].counter = &counter;
    args[i].mutex = &counter_mutex;

    task_t task = make_counter_task(&args[i], i);
    assert_true(thread_pool_enqueue(&pool, &task));
  }

  thread_pool_wait(&pool);
  thread_pool_destroy(&pool);
  pthread_mutex_destroy(&counter_mutex);

  assert_int_equal(counter, TASK_COUNT);
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(thread_pool_runs_all_tasks),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
