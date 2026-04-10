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
  const double *data;
  double factor;
  double bias;
  filter_size_t size;
} filter_t;

static inline filter_t make_filter(const double *data,
                                   double factor,
                                   double bias,
                                   filter_size_t size) {
  return (filter_t){
    .data = data,
    .factor = factor,
    .bias = bias,
    .size = size,
  };
}

// Может не понадобится
// static inline double filter_at(const filter_t *filter, size_t row, size_t col) {
//   return filter->data[row * filter->size + col];
// }