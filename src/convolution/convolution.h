#pragma once

#include <stddef.h>

#include <filters/filter.h>
#include <image_helpers/image_helpers.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct convolution_context {
  const filter_t *filter;
  const unsigned char *source;
  unsigned char *destination;
  size_t height;
  size_t width;
  size_t stride;
  size_t channels;
} convolution_context_t;

typedef int (*convolution_executor_t)(const convolution_context_t *context);

int convolution_run(const filter_t *filter,
                    image_view_t *image_view,
                    convolution_executor_t executor);

int convolution_apply_pixel(const convolution_context_t *context,
                            size_t y,
                            size_t x);

int convolution_apply_rect(const convolution_context_t *context,
                           size_t y_begin,
                           size_t y_end,
                           size_t x_begin,
                           size_t x_end);

int convolution_apply_rows(const convolution_context_t *context,
                           size_t y_begin,
                           size_t y_end);

int convolution_apply_cols(const convolution_context_t *context,
                           size_t x_begin,
                           size_t x_end);

#ifdef __cplusplus
}
#endif