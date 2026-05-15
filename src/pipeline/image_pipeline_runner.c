#include "image_pipeline_runner.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>

#include <runtime/thread_pool.h>

#define PIPELINE_WORKER_COUNT 4U
#define PIPELINE_MAX_ACTIVE_READERS 1U
#define PIPELINE_MAX_ACTIVE_COMPUTERS 2U
#define PIPELINE_MAX_ACTIVE_WRITERS 1U

typedef enum image_job_status {
  IMAGE_JOB_OK = 0,
  IMAGE_JOB_READ_FAILED,
  IMAGE_JOB_COMPUTE_FAILED,
  IMAGE_JOB_WRITE_FAILED,
  IMAGE_JOB_ENQUEUE_FAILED,
} image_job_status_t;

typedef struct image_pipeline {
  thread_pool_t pool;
  atomic_bool failed;
} image_pipeline_t;

typedef struct image_job {
  image_pipeline_t *pipeline;
  image_pipeline_request_t request;

  IplImage *image;
  image_view_t image_view;

  image_job_status_t status;
} image_job_t;

static void read_image_task(void *args);
static void compute_image_task(void *args);
static void write_image_task(void *args);

static void pipeline_mark_failed(image_pipeline_t *pipeline) {
  if (pipeline != NULL) {
    atomic_store(&pipeline->failed, true);
  }
}

static void image_job_destroy(image_job_t *job) {
  if (job == NULL) {
    return;
  }

  if (job->image != NULL) {
    cvReleaseImage(&job->image);
  }

  free(job);
}

static bool pipeline_request_is_valid(const image_pipeline_request_t *request) {
  return request != NULL && request->input_path != NULL &&
         request->output_path != NULL && request->filter_count != 0U &&
         request->filter_count <= IMAGE_PIPELINE_MAX_FILTERS &&
         request->run_convolution != NULL;
}

static image_job_t *image_job_create(image_pipeline_t *pipeline,
                                     const image_pipeline_request_t *request) {
  if (pipeline == NULL || !pipeline_request_is_valid(request)) {
    return NULL;
  }

  image_job_t *job = (image_job_t *)calloc(1U, sizeof(*job));
  if (job == NULL) {
    return NULL;
  }

  job->pipeline = pipeline;
  job->request = *request;
  job->status = IMAGE_JOB_OK;
  return job;
}

static bool
enqueue_job_task(image_job_t *job, task_type_t task_type, task_fn_t task_fn) {
  if (job == NULL || job->pipeline == NULL || task_fn == NULL) {
    return false;
  }

  task_t task = {
    .fn = task_fn,
    .fn_type = task_type,
    .args = job,
  };

  return thread_pool_enqueue(&job->pipeline->pool, &task);
}

static void cleanup_after_enqueue_failure(image_job_t *job,
                                          const char *stage_name) {
  if (job == NULL) {
    return;
  }

  if (job->status == IMAGE_JOB_OK) {
    job->status = IMAGE_JOB_ENQUEUE_FAILED;
  }
  pipeline_mark_failed(job->pipeline);

  if (stage_name != NULL) {
    fprintf(stderr,
            "failed to enqueue %s task: %s\n",
            stage_name,
            job->request.input_path != NULL ? job->request.input_path : "");
  }

  image_job_destroy(job);
}

static void enqueue_compute_or_cleanup(image_job_t *job) {
  if (!enqueue_job_task(job, COMPUTE_TASK, compute_image_task)) {
    cleanup_after_enqueue_failure(job, "compute");
  }
}

static void enqueue_writer_or_cleanup(image_job_t *job) {
  if (!enqueue_job_task(job, WRITER_TASK, write_image_task)) {
    cleanup_after_enqueue_failure(job, "writer");
  }
}

static int apply_filters(const image_pipeline_request_t *request,
                         image_view_t *image_view) {
  if (request == NULL || image_view == NULL ||
      request->run_convolution == NULL) {
    return -1;
  }

  for (size_t i = 0; i < request->filter_count; ++i) {
    filter_t filter;

    if (filter_init_builtin(&filter, &request->filters[i]) !=
        FILTER_STATUS_OK) {
      return -1;
    }

    if (!filter_is_convolution(&filter) ||
        request->run_convolution(&filter, image_view) != 0) {
      return -1;
    }
  }

  return 0;
}

static void read_image_task(void *args) {
  image_job_t *job = args;
  if (job == NULL) {
    return;
  }

  job->image = cvLoadImage(job->request.input_path, CV_LOAD_IMAGE_UNCHANGED);
  if (job->image == NULL) {
    job->status = IMAGE_JOB_READ_FAILED;
    pipeline_mark_failed(job->pipeline);
    fprintf(stderr, "failed to load image: %s\n", job->request.input_path);
    enqueue_writer_or_cleanup(job);
    return;
  }

  job->image_view = (image_view_t){
    .data = (unsigned char *)job->image->imageData,
    .height = (size_t)job->image->height,
    .width = (size_t)job->image->width,
    .stride = (size_t)job->image->widthStep,
    .channels = (size_t)job->image->nChannels,
  };

  enqueue_compute_or_cleanup(job);
}

static void compute_image_task(void *args) {
  image_job_t *job = args;
  if (job == NULL) {
    return;
  }

  if (job->status == IMAGE_JOB_OK &&
      apply_filters(&job->request, &job->image_view) != 0) {
    job->status = IMAGE_JOB_COMPUTE_FAILED;
    pipeline_mark_failed(job->pipeline);
    fprintf(stderr, "failed to apply filters: %s\n", job->request.input_path);
  }

  enqueue_writer_or_cleanup(job);
}

static void write_image_task(void *args) {
  image_job_t *job = args;
  if (job == NULL) {
    return;
  }

  if (job->status == IMAGE_JOB_OK) {
    if (!cvSaveImage(job->request.output_path, job->image, NULL)) {
      job->status = IMAGE_JOB_WRITE_FAILED;
      pipeline_mark_failed(job->pipeline);
      fprintf(stderr, "failed to save image: %s\n", job->request.output_path);
    }
  }

  image_job_destroy(job);
}

int image_pipeline_run(const image_pipeline_request_t *requests,
                       size_t request_count) {
  if (requests == NULL || request_count == 0U) {
    return -1;
  }

  image_pipeline_t pipeline;
  atomic_init(&pipeline.failed, false);

  const thread_pool_config_t config = {
    .worker_count = PIPELINE_WORKER_COUNT,
    .reader_queue_capacity = request_count,
    .writer_queue_capacity = request_count,
    .computer_queue_capacity = request_count,
    .max_active_readers = PIPELINE_MAX_ACTIVE_READERS,
    .max_active_writers = PIPELINE_MAX_ACTIVE_WRITERS,
    .max_active_computers = PIPELINE_MAX_ACTIVE_COMPUTERS,
  };

  if (!thread_pool_init(&pipeline.pool, &config)) {
    return -1;
  }

  for (size_t i = 0; i < request_count; ++i) {
    image_job_t *job = image_job_create(&pipeline, &requests[i]);
    if (job == NULL) {
      pipeline_mark_failed(&pipeline);
      continue;
    }

    if (!enqueue_job_task(job, READER_TASK, read_image_task)) {
      cleanup_after_enqueue_failure(job, "reader");
    }
  }

  thread_pool_wait(&pipeline.pool);
  thread_pool_destroy(&pipeline.pool);

  return !atomic_load(&pipeline.failed) ? 0 : -1;
}
