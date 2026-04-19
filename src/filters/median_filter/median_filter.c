#include "median_filter.h"

filter_status_t
init_median_filter(filter_t *filter, size_t width, size_t height) {
  if (!filter) {
    return FILTER_STATUS_NULL_POINTER;
  }

  if (!filter_size_is_odd(width) || !filter_size_is_odd(height)) {
    return FILTER_STATUS_UNSUPPORTED_SIZE;
  }

  *filter = make_rank_filter(FILTER_KIND_MEDIAN,
                             FILTER_CATEGORY_SMOOTHING,
                             FILTER_DIRECTION_NONE,
                             FILTER_BORDER_WRAP,
                             width,
                             height,
                             filter_median_rank(width, height));

  return FILTER_STATUS_OK;
}
