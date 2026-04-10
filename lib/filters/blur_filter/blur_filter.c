#include "blur_filter.h"

// clang-format off
static const double blur_data_3x3[] = {
  0.0, 0.2, 0.0,       
  0.2, 0.2, 0.2,       
  0.0, 0.2, 0.0 
};

static const double blur_data_5x5[] = {
  0.0, 0.0, 1.0, 0.0, 0.0,
  0.0, 1.0, 1.0, 1.0, 0.0,
  1.0, 1.0, 1.0, 1.0, 1.0,
  0.0, 1.0, 1.0, 1.0, 0.0,
  0.0, 0.0, 1.0, 0.0, 0.0,
};
// clang-format on

static const double blur_factor_3x3 = 1.0;
static const double blur_factor_5x5 = 1.0 / 13.0;
static const double blur_bias = 0.0;

filter_status_t init_blur_filter(filter_t *filter, filter_size_t size) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }
  switch (size) {
  case FILTER_SIZE_3:
    *filter = make_filter(blur_data_3x3, blur_factor_3x3, blur_bias, size);
    return FILTER_STATUS_OK;
  case FILTER_SIZE_5:
    *filter = make_filter(blur_data_5x5, blur_factor_5x5, blur_bias, size);
    return FILTER_STATUS_OK;
  case FILTER_SIZE_7:
  case FILTER_SIZE_9:
  default:
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
}