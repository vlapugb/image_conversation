#pragma once

#include <stddef.h>
#include <filters/filter.h>
#include <image_helpers/image_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

int parallel_convolution_rows(const filter_t *filter, image_view_t *image_view);

int parallel_convolution_cols(const filter_t *filter, image_view_t *image_view);

int parallel_convolution_pixels(const filter_t *filter,
                                image_view_t *image_view);

int parallel_convolution_rectangle(const filter_t *filter,
                                   image_view_t *image_view);

#ifdef __cplusplus
}
#endif