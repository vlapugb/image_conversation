#include "mean_filter.h"

// clang-format off
static const double mean_kernel_3x3[] = {
  1, 1, 1,
  1, 1, 1,
  1, 1, 1,
};

static const double mean_factor_3x3 = 1.0 / 9.0;
static const double mean_bias_3x3 = 0.0;

static const double mean_kernel_5x5[] = {
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
};

static const double mean_factor_5x5 = 1.0 / 25.0;
static const double mean_bias_5x5 = 0.0;
// clang-format on

filter_status_t init_mean_filter(filter_t *filter, size_t width, size_t height) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }

  if (width != height) {
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }

  switch (width) {
  case 3:
    *filter = make_convolution_filter(FILTER_KIND_MEAN,
                                      FILTER_CATEGORY_SMOOTHING,
                                      FILTER_DIRECTION_NONE,
                                      FILTER_BORDER_WRAP,
                                      mean_kernel_3x3,
                                      mean_factor_3x3,
                                      mean_bias_3x3,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case 5:
    *filter = make_convolution_filter(FILTER_KIND_MEAN,
                                      FILTER_CATEGORY_SMOOTHING,
                                      FILTER_DIRECTION_NONE,
                                      FILTER_BORDER_WRAP,
                                      mean_kernel_5x5,
                                      mean_factor_5x5,
                                      mean_bias_5x5,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  default:
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
}
