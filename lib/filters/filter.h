#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum filter_status {
  FILTER_STATUS_OK = 0,
  FILTER_STATUS_NULL_POINTER,
  FILTER_STATUS_INVALID_ARGUMENT,
  FILTER_STATUS_UNSUPPORTED_SIZE,
  FILTER_STATUS_UNSUPPORTED_DIRECTION,
  FILTER_STATUS_UNSUPPORTED_KIND
} filter_status_t;

typedef enum filter_category {
  FILTER_CATEGORY_SMOOTHING = 0,
  FILTER_CATEGORY_EDGE_DETECTION,
  FILTER_CATEGORY_EDGE_ENHANCEMENT,
} filter_category_t;

typedef enum filter_operator {
  FILTER_OPERATOR_CONVOLUTION = 0,
  FILTER_OPERATOR_RANK_SELECTION,
} filter_operator_t;

typedef enum filter_kind {
  FILTER_KIND_BLUR = 0,
  FILTER_KIND_MEAN,
  FILTER_KIND_GAUSSIAN_BLUR,
  FILTER_KIND_MOTION_BLUR,
  FILTER_KIND_EDGE_DETECT,
  FILTER_KIND_SHARPEN,
  FILTER_KIND_EMBOSS,
  FILTER_KIND_MEDIAN,
} filter_kind_t;

typedef enum filter_direction {
  FILTER_DIRECTION_NONE = 0,
  FILTER_DIRECTION_HORIZONTAL,
  FILTER_DIRECTION_VERTICAL,
  FILTER_DIRECTION_DIAGONAL_45,
  FILTER_DIRECTION_OMNIDIRECTIONAL,
} filter_direction_t;

typedef enum filter_border_mode {
  FILTER_BORDER_WRAP = 0,
  FILTER_BORDER_CLAMP,
  FILTER_BORDER_REFLECT,
} filter_border_mode_t;

typedef struct filter_request {
  filter_kind_t kind;
  size_t width;
  size_t height;
  filter_direction_t direction;
  filter_border_mode_t border_mode;
} filter_request_t;

typedef struct filter {
  filter_category_t category;
  filter_operator_t operator;
  filter_kind_t kind;
  filter_direction_t direction;
  filter_border_mode_t border_mode;
  const double *kernel;
  double factor;
  double bias;
  size_t width;
  size_t height;
  size_t rank_index;
} filter_t;

static inline bool filter_size_is_odd(size_t value) {
  return value != 0U && (value % 2U) == 1U;
}

static inline size_t filter_median_rank(size_t width, size_t height) {
  return (width * height) / 2U;
}

static inline filter_request_t make_filter_request(filter_kind_t kind,
                                                   size_t width,
                                                   size_t height) {
  return (filter_request_t){
    .kind = kind,
    .width = width,
    .height = height,
    .direction = FILTER_DIRECTION_NONE,
    .border_mode = FILTER_BORDER_WRAP,
  };
}

static inline filter_t make_convolution_filter(filter_kind_t kind,
                                               filter_category_t category,
                                               filter_direction_t direction,
                                               filter_border_mode_t border_mode,
                                               const double *kernel,
                                               double factor,
                                               double bias,
                                               size_t width,
                                               size_t height) {
  return (filter_t){
    .category = category,
    .operator = FILTER_OPERATOR_CONVOLUTION,
    .kind = kind,
    .direction = direction,
    .border_mode = border_mode,
    .kernel = kernel,
    .factor = factor,
    .bias = bias,
    .width = width,
    .height = height,
    .rank_index = 0U,
  };
}

static inline filter_t make_rank_filter(filter_kind_t kind,
                                        filter_category_t category,
                                        filter_direction_t direction,
                                        filter_border_mode_t border_mode,
                                        size_t width,
                                        size_t height,
                                        size_t rank_index) {
  return (filter_t){
    .category = category,
    .operator = FILTER_OPERATOR_RANK_SELECTION,
    .kind = kind,
    .direction = direction,
    .border_mode = border_mode,
    .kernel = NULL,
    .factor = 0.0,
    .bias = 0.0,
    .width = width,
    .height = height,
    .rank_index = rank_index,
  };
}

static inline bool filter_is_convolution(const filter_t *filter) {
  return filter != NULL && filter->operator == FILTER_OPERATOR_CONVOLUTION;
}

static inline bool filter_is_rank_selection(const filter_t *filter) {
  return filter != NULL &&
         filter->operator == FILTER_OPERATOR_RANK_SELECTION;
}

static inline bool filter_has_explicit_kernel(const filter_t *filter) {
  return filter_is_convolution(filter) && filter->kernel != NULL;
}

filter_status_t filter_init_builtin(filter_t *filter,
                                    const filter_request_t *request);
