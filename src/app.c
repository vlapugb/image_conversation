#include "cli_args.h"

#include <filters/filter.h>
#include <parallel_convolution/parallel_convolution.h>
#include <pipeline/image_pipeline_runner.h>
#include <sequentially_convolution/sequentially_convolution.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ERROR_MESSAGE_LENGTH 32

static double elapsed_ms(const struct timespec *start,
                         const struct timespec *end) {
  const double seconds = (double)(end->tv_sec - start->tv_sec) * 1000.0;
  const double nanoseconds =
    (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;

  return seconds + nanoseconds;
}

static image_convolution_runner_t
select_convolution_runner(execution_mode_t mode) {
  switch (mode) {
  case EXECUTION_MODE_SEQ:
    return sequential_convolution;
  case EXECUTION_MODE_ROWS:
    return parallel_convolution_rows;
  case EXECUTION_MODE_PIXELS:
    return parallel_convolution_pixels;
  case EXECUTION_MODE_COLS:
    return parallel_convolution_cols;
  case EXECUTION_MODE_GRID:
    return parallel_convolution_rectangle;
  default:
    return NULL;
  }
}

static void fill_pipeline_request(image_pipeline_request_t *pipeline_request,
                                  const cli_request_t *cli_request,
                                  size_t image_index,
                                  image_convolution_runner_t runner) {
  pipeline_request->input_path = cli_request->images[image_index].input_path;
  pipeline_request->output_path = cli_request->images[image_index].output_path;
  pipeline_request->filter_count = cli_request->filter_count;
  pipeline_request->run_convolution = runner;

  for (size_t i = 0; i < cli_request->filter_count; ++i) {
    pipeline_request->filters[i] = (filter_request_t){
      .kind = cli_request->filters[i].kind,
      .width = cli_request->filters[i].width,
      .height = cli_request->filters[i].height,
      .direction = cli_request->filters[i].direction,
      .border_mode = cli_request->filters[i].border_mode,
    };
  }
}

int main(int argc, char **argv) {
  cli_request_t request;
  char error_message[MAX_ERROR_MESSAGE_LENGTH];
  cli_parse_status_t parse_status =
    cli_parse_args(argc, argv, &request, error_message, sizeof(error_message));

  if (parse_status == CLI_PARSE_HELP) {
    return 0;
  }

  if (parse_status != CLI_PARSE_OK) {
    puts(error_message);
    return -1;
  }

  image_convolution_runner_t runner = select_convolution_runner(request.mode);
  if (runner == NULL) {
    cli_request_destroy(&request);
    fputs("invalid execution mode\n", stderr);
    return -1;
  }

  image_pipeline_request_t *pipeline_requests =
    (image_pipeline_request_t *)malloc(sizeof(*pipeline_requests) *
                                       request.image_count);
  if (pipeline_requests == NULL) {
    cli_request_destroy(&request);
    fputs("failed to allocate pipeline requests\n", stderr);
    return -1;
  }

  for (size_t i = 0; i < request.image_count; ++i) {
    fill_pipeline_request(&pipeline_requests[i], &request, i, runner);
  }

  struct timespec start_time;
  struct timespec end_time;

  timespec_get(&start_time, TIME_UTC);

  const int pipeline_result =
    image_pipeline_run(pipeline_requests, request.image_count);

  timespec_get(&end_time, TIME_UTC);
  printf("processing time: %.3f ms\n", elapsed_ms(&start_time, &end_time));

  free(pipeline_requests);
  cli_request_destroy(&request);
  return pipeline_result == 0 ? 0 : -1;
}
