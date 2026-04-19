#include "filter.h"

#include "blur_filter/blur_filter.h"
#include "emboss_filter/emboss_filter.h"
#include "find_edges_filter/find_edges_filter.h"
#include "gauss_blur_filter/gauss_blur_filter.h"
#include "mean_filter/mean_filter.h"
#include "median_filter/median_filter.h"
#include "motion_blur_filter/motion_blur_filter.h"
#include "sharpen_filter/sharpen_filter.h"

filter_status_t filter_init_builtin(filter_t *filter,
                                    const filter_request_t *request) {
  filter_status_t status = FILTER_STATUS_UNSUPPORTED_KIND;

  if (!filter || !request) {
    return FILTER_STATUS_NULL_POINTER;
  }

  switch (request->kind) {
  case FILTER_KIND_BLUR:
    if (request->direction != FILTER_DIRECTION_NONE) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_blur_filter(filter, request->width, request->height);
    break;
  case FILTER_KIND_MEAN:
    if (request->direction != FILTER_DIRECTION_NONE) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_mean_filter(filter, request->width, request->height);
    break;
  case FILTER_KIND_GAUSSIAN_BLUR:
    if (request->direction != FILTER_DIRECTION_NONE) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_gauss_blur_filter(filter, request->width, request->height);
    break;
  case FILTER_KIND_MOTION_BLUR:
    if (request->direction != FILTER_DIRECTION_NONE &&
        request->direction != FILTER_DIRECTION_DIAGONAL_45) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_motion_blur_filter(filter, request->width, request->height);
    break;
  case FILTER_KIND_EDGE_DETECT:
    status = init_find_edges_filter(
      filter, request->width, request->height, request->direction);
    break;
  case FILTER_KIND_SHARPEN:
    if (request->direction != FILTER_DIRECTION_NONE) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_sharpen_filter(filter, request->width, request->height);
    break;
  case FILTER_KIND_EMBOSS:
    status = init_emboss_filter(
      filter, request->width, request->height, request->direction);
    break;
  case FILTER_KIND_MEDIAN:
    if (request->direction != FILTER_DIRECTION_NONE) {
      return FILTER_STATUS_UNSUPPORTED_DIRECTION;
    }
    status = init_median_filter(filter, request->width, request->height);
    break;
  default:
    return FILTER_STATUS_UNSUPPORTED_KIND;
  }

  if (status == FILTER_STATUS_OK) {
    filter->border_mode = request->border_mode;
  }

  return status;
}
