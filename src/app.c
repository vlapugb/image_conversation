#include "cli_args.h"

#include <filters/filter.h>
#include <sequentially_convolution/sequentially_convolution.h>

#include <time.h>
#include <stdio.h>

#include <opencv2/highgui/highgui_c.h>
#include <opencv2/core/core_c.h>

#define MAX_ERROR_MESSAGE_LENGTH 32

static double elapsed_ms(const struct timespec *start,
                         const struct timespec *end) {
  const double seconds = (double)(end->tv_sec - start->tv_sec) * 1000.0;
  const double nanoseconds =
    (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;

  return seconds + nanoseconds;
}

static int apply_filters(const cli_request_t *request,
                         image_view_t *image_view) {
  for (size_t i = 0; i < request->filter_count; ++i) {
    filter_t filter;
    filter_request_t filter_request = {
      .kind = request->filters[i].kind,
      .width = request->filters[i].width,
      .height = request->filters[i].height,
      .direction = request->filters[i].direction,
      .border_mode = request->filters[i].border_mode,
    };

    if (filter_init_builtin(&filter, &filter_request) != FILTER_STATUS_OK) {
      return -1;
    }

    if (!filter_is_convolution(&filter) ||
        sequential_convolution(&filter, image_view) != 0) {
      return -1;
    }
  }

  return 0;
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

  IplImage *image = cvLoadImage(request.input_path, CV_LOAD_IMAGE_UNCHANGED);
  if (image == NULL) {
    fprintf(stderr, "failed to load image: %s\n", request.input_path);
    return -1;
  }

  image_view_t image_view = {
    .data = (unsigned char *)image->imageData,
    .height = (size_t)image->height,
    .width = (size_t)image->width,
    .stride = (size_t)image->widthStep,
    .channels = (size_t)image->nChannels,
  };
  struct timespec start_time;
  struct timespec end_time;

  timespec_get(&start_time, TIME_UTC);

  if (apply_filters(&request, &image_view) != 0) {
    cvReleaseImage(&image);
    fputs("failed to apply filters\n", stderr);
    return -1;
  }

  timespec_get(&end_time, TIME_UTC);
  printf("processing time: %.3f ms\n", elapsed_ms(&start_time, &end_time));

  if (!cvSaveImage(request.output_path, image, NULL)) {
    cvReleaseImage(&image);
    fprintf(stderr, "failed to save image: %s\n", request.output_path);
    return -1;
  }

  cvReleaseImage(&image);
  return 0;
}
