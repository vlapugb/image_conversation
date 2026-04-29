#pragma once

#include <image_helpers/image_helpers.h>
#include <filters/filter.h>

#define IMAGE_PIPELINE_MAX_FILTERS 2U

typedef int (*image_convolution_runner_t)(const filter_t *filter,
                                          image_view_t *image_view);

typedef struct image_pipeline_request {
  const char *input_path;
  const char *output_path;

  filter_request_t filters[IMAGE_PIPELINE_MAX_FILTERS];
  size_t filter_count;

  image_convolution_runner_t run_convolution;
} image_pipeline_request_t;