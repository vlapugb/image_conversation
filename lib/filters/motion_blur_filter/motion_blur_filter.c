#include "motion_blur_filter.h"

// clang-format off

static const double blur_kernel_9x9[] = {
  1, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 1,
};
// clang-format on

static const double blur_factor_9x9 = 1.0 / 9.0;
static const double blur_bias = 0.0;

filter_status_t init_motion_blur_filter(filter_t *filter,
                                        size_t width,
                                        size_t height) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }
  if (width != height) {
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
  switch (width) {
  case 9:
    *filter =
      make_filter(blur_kernel_9x9, blur_factor_9x9, blur_bias, width, height);
    return FILTER_STATUS_OK;
  case 3:
  case 5:
  case 7:
  default:
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
}
