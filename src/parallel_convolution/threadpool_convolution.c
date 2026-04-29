#include <runtime/thread_pool.h>
#include <opencv2/core/core_c.h>
#include <parallel_convolution/parallel_convolution.h>

typedef struct image_job {
  IplImage *src;
  IplImage *dst;

  task_t *task;
} image_job_t;
