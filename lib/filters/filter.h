#pragma once

#include <stddef.h>

typedef enum filter_status {
  FILTER_STATUS_OK = 0,
  FILTER_STATUS_NULL_POINTER,
  FILTER_STATUS_UNSUPPORTED_SIZE,
} filter_status_t;

typedef enum filter_size {
  FILTER_SIZE_3 = 3,
  FILTER_SIZE_5 = 5,
  FILTER_SIZE_7 = 7,
  FILTER_SIZE_9 = 9,
} filter_size_t;

typedef struct filter {
  const double *kernel;
  double factor;
  double bias;
  size_t width;
  size_t height;
} filter_t;

static inline filter_t make_filter(const double *kernel,
                                   double factor,
                                   double bias,
                                   size_t width,
                                   size_t height) {
  return (filter_t){
    .kernel = kernel,
    .factor = factor,
    .bias = bias,
    .width = width,
    .height = height,
  };
}

// Может не понадобится
// static inline double filter_at(const filter_t *filter, size_t row,
//                                size_t col) {
//   return filter->kernel[row * filter->width + col];
// }
