#include "emboss_filter.h"

// clang-format off
static const double emboss_kernel_3x3[] = {
  -1, -1,  0,
  -1,  0,  1,
   0,  1,  1,
};

static const double emboss_factor_3x3 = 1.0;
static const double emboss_bias_3x3 = 128.0;

static const double emboss_kernel_5x5[] = {
  -1, -1, -1, -1,  0,
  -1, -1, -1,  0,  1,
  -1, -1,  0,  1,  1,
  -1,  0,  1,  1,  1,
   0,  1,  1,  1,  1,
};

static const double emboss_factor_5x5 = 1.0;
static const double emboss_bias_5x5 = 128.0;
// clang-format on

filter_status_t init_emboss_filter(filter_t *filter,
                                   size_t width,
                                   size_t height,
                                   filter_direction_t direction) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }

  if (direction == FILTER_DIRECTION_NONE) {
    direction = FILTER_DIRECTION_DIAGONAL_45;
  }

  if (direction != FILTER_DIRECTION_DIAGONAL_45) {
    return FILTER_STATUS_UNSUPPORTED_DIRECTION;
  }

  if (width != height) {
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }

  switch (width) {
  case 3:
    *filter = make_convolution_filter(FILTER_KIND_EMBOSS,
                                      FILTER_CATEGORY_EDGE_ENHANCEMENT,
                                      FILTER_DIRECTION_DIAGONAL_45,
                                      FILTER_BORDER_WRAP,
                                      emboss_kernel_3x3,
                                      emboss_factor_3x3,
                                      emboss_bias_3x3,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case 5:
    *filter = make_convolution_filter(FILTER_KIND_EMBOSS,
                                      FILTER_CATEGORY_EDGE_ENHANCEMENT,
                                      FILTER_DIRECTION_DIAGONAL_45,
                                      FILTER_BORDER_WRAP,
                                      emboss_kernel_5x5,
                                      emboss_factor_5x5,
                                      emboss_bias_5x5,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  default:
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }
}
