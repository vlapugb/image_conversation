#pragma once

#include <stddef.h>
#include <filters/filter.h>
#include <image_helpers/image_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

int sequential_convolution(const filter_t *filter, image_view_t *image_view);

#ifdef __cplusplus
}
#endif
