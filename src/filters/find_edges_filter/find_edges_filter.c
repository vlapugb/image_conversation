#include "find_edges_filter.h"

// clang-format off
static const double find_edge_kernel_horizontal[] = {
   0,  0, -1,  0,  0,
   0,  0, -1,  0,  0,
   0,  0,  2,  0,  0,
   0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,
};

static const double find_edge_horizontal_factor = 1.0;
static const double find_edge_horizontal_bias = 0.0;

static const double find_edge_kernel_vertical[] = {
   0,  0, -1,  0,  0,
   0,  0, -1,  0,  0,
   0,  0,  4,  0,  0,
   0,  0, -1,  0,  0,
   0,  0, -1,  0,  0,
};

static const double find_edge_vertical_factor = 1.0;
static const double find_edge_vertical_bias = 0.0;

static const double find_edge_kernel_diagonal45deg[] = {
  -1,  0,  0,  0,  0,
   0, -2,  0,  0,  0,
   0,  0,  6,  0,  0,
   0,  0,  0, -2,  0,
   0,  0,  0,  0, -1,
};

static const double find_edge_diagonal45deg_factor = 1.0;
static const double find_edge_diagonal45deg_bias = 0.0;


static const double find_edge_kernel_any_direction[] = {
  -1, -1, -1,
  -1,  8, -1,
  -1, -1, -1
};

static const double find_edge_any_direction_factor = 1.0;
static const double find_edge_any_direction_bias = 0.0;

// clang-format on

filter_status_t init_find_edges_filter(filter_t *filter,
                                       size_t width,
                                       size_t height,
                                       filter_direction_t direction) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }

  if (direction == FILTER_DIRECTION_NONE) {
    direction = FILTER_DIRECTION_OMNIDIRECTIONAL;
  }

  switch (direction) {
  case FILTER_DIRECTION_HORIZONTAL:
    if (width != 5 || height != 5) {
      return FILTER_STATUS_UNSUPPORTED_SIZE;
    }
    *filter = make_convolution_filter(FILTER_KIND_EDGE_DETECT,
                                      FILTER_CATEGORY_EDGE_DETECTION,
                                      FILTER_DIRECTION_HORIZONTAL,
                                      FILTER_BORDER_WRAP,
                                      find_edge_kernel_horizontal,
                                      find_edge_horizontal_factor,
                                      find_edge_horizontal_bias,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case FILTER_DIRECTION_VERTICAL:
    if (width != 5 || height != 5) {
      return FILTER_STATUS_UNSUPPORTED_SIZE;
    }
    *filter = make_convolution_filter(FILTER_KIND_EDGE_DETECT,
                                      FILTER_CATEGORY_EDGE_DETECTION,
                                      FILTER_DIRECTION_VERTICAL,
                                      FILTER_BORDER_WRAP,
                                      find_edge_kernel_vertical,
                                      find_edge_vertical_factor,
                                      find_edge_vertical_bias,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case FILTER_DIRECTION_DIAGONAL_45:
    if (width != 5 || height != 5) {
      return FILTER_STATUS_UNSUPPORTED_SIZE;
    }
    *filter = make_convolution_filter(FILTER_KIND_EDGE_DETECT,
                                      FILTER_CATEGORY_EDGE_DETECTION,
                                      FILTER_DIRECTION_DIAGONAL_45,
                                      FILTER_BORDER_WRAP,
                                      find_edge_kernel_diagonal45deg,
                                      find_edge_diagonal45deg_factor,
                                      find_edge_diagonal45deg_bias,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  case FILTER_DIRECTION_OMNIDIRECTIONAL:
    if (width != 3 || height != 3) {
      return FILTER_STATUS_UNSUPPORTED_SIZE;
    }
    *filter = make_convolution_filter(FILTER_KIND_EDGE_DETECT,
                                      FILTER_CATEGORY_EDGE_DETECTION,
                                      FILTER_DIRECTION_OMNIDIRECTIONAL,
                                      FILTER_BORDER_WRAP,
                                      find_edge_kernel_any_direction,
                                      find_edge_any_direction_factor,
                                      find_edge_any_direction_bias,
                                      width,
                                      height);
    return FILTER_STATUS_OK;
  default:
    return FILTER_STATUS_UNSUPPORTED_DIRECTION;
  }
}
