#include "gauss_blur_filter.h"

// clang-format off
static const double gauss_blur_kernel_3x3[] = {
  1, 2, 1,
  2, 4, 2,
  1, 2, 1, 
};

static const double blur_factor_3x3 = 1.0 / 16.0;
static const double blur_bias_3x3 = 0.0;

static const double gauss_blur_kernel_5x5[] = {
  1,  4,  6,  4,  1,
  4, 16, 24, 16,  4,
  6, 24, 36, 24,  6,
  4, 16, 24, 16,  4,
  1,  4,  6,  4,  1,
};

static const double blur_factor_5x5 = 1.0 / 256.0;
static const double blur_bias_5x5 = 0.0;
// clang-format on

filter_status_t
init_gauss_blur_filter(filter_t *filter, size_t width, size_t height) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }
  if (width != height) {
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
  switch (width) {
  case 3:
    *filter = make_convolution_filter(FILTER_KIND_GAUSSIAN_BLUR,
                                      FILTER_CATEGORY_SMOOTHING,
                                      FILTER_DIRECTION_NONE,
                                      FILTER_BORDER_WRAP,
                                      gauss_blur_kernel_3x3,
                                      blur_factor_3x3,
                                      blur_bias_3x3,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case 5:
    *filter = make_convolution_filter(FILTER_KIND_GAUSSIAN_BLUR,
                                      FILTER_CATEGORY_SMOOTHING,
                                      FILTER_DIRECTION_NONE,
                                      FILTER_BORDER_WRAP,
                                      gauss_blur_kernel_5x5,
                                      blur_factor_5x5,
                                      blur_bias_5x5,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  default:
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
}
