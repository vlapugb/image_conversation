#include "convolution.h"
#include <sequentially_convolution/helper_functions.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int validate_convolution_input(const filter_t *filter,
                                      const image_view_t *image_view) {
  if (filter == NULL || image_view == NULL || image_view->data == NULL) {
    return -1;
  }
  if (!filter_is_convolution(filter) || !filter_has_explicit_kernel(filter)) {
    return -1;
  }
  if (filter->width == 0U || filter->height == 0U) {
    return -1;
  }
  if (image_view->width == 0U || image_view->height == 0U ||
      image_view->channels == 0U) {
    return -1;
  }
  if (image_view->width > (SIZE_MAX / image_view->channels)) {
    return -1;
  }
  if (image_view->stride < image_view->width * image_view->channels) {
    return -1;
  }
  if (image_view->height > (SIZE_MAX / image_view->stride)) {
    return -1;
  }

  return 0;
}

static int convolution_context_is_valid(const convolution_context_t *context) {
  return context != NULL && context->filter != NULL &&
         context->source != NULL && context->destination != NULL &&
         context->width != 0U && context->height != 0U &&
         context->stride != 0U && context->channels != 0U;
}

static void convolution_apply_pixel_unchecked(
  const convolution_context_t *context, size_t y, size_t x) {
  const filter_t *filter = context->filter;
  unsigned char *dst_pixel =
    context->destination + y * context->stride + x * context->channels;

  for (size_t channel = 0; channel < context->channels; ++channel) {
    if (context->channels == 4U && channel == 3U) {
      dst_pixel[channel] =
        context->source[y * context->stride + x * context->channels + channel];
      continue;
    }

    double sum = 0.0;

    for (size_t filter_y = 0; filter_y < filter->height; ++filter_y) {
      for (size_t filter_x = 0; filter_x < filter->width; ++filter_x) {
        const long src_x =
          (long)x - (long)(filter->width / 2U) + (long)filter_x;
        const long src_y =
          (long)y - (long)(filter->height / 2U) + (long)filter_y;

        const size_t image_x =
          resolve_index(src_x, context->width, filter->border_mode);
        const size_t image_y =
          resolve_index(src_y, context->height, filter->border_mode);

        const unsigned char *src_pixel = context->source +
                                         image_y * context->stride +
                                         image_x * context->channels;
        const double kernel_value =
          filter->kernel[filter_y * filter->width + filter_x];

        sum += (double)src_pixel[channel] * kernel_value;
      }
    }

    dst_pixel[channel] = clamp_to_u8(filter->factor * sum + filter->bias);
  }
}

int convolution_run(const filter_t *filter,
                    image_view_t *image_view,
                    convolution_executor_t executor) {
  if (executor == NULL || validate_convolution_input(filter, image_view) != 0) {
    return -1;
  }

  const size_t buffer_size = image_view->height * image_view->stride;
  unsigned char *source_copy = (unsigned char *)malloc(buffer_size);
  if (source_copy == NULL) {
    return -1;
  }

  memcpy(source_copy, image_view->data, buffer_size);

  const convolution_context_t context = {
    .filter = filter,
    .source = source_copy,
    .destination = image_view->data,
    .height = image_view->height,
    .width = image_view->width,
    .stride = image_view->stride,
    .channels = image_view->channels,
  };

  const int result = executor(&context);
  free(source_copy);
  return result;
}

int convolution_apply_pixel(const convolution_context_t *context,
                            size_t y,
                            size_t x) {
  if (!convolution_context_is_valid(context) || y >= context->height ||
      x >= context->width) {
    return -1;
  }

  convolution_apply_pixel_unchecked(context, y, x);
  return 0;
}

int convolution_apply_rect(const convolution_context_t *context,
                           size_t y_begin,
                           size_t y_end,
                           size_t x_begin,
                           size_t x_end) {
  if (!convolution_context_is_valid(context) || y_begin > y_end ||
      x_begin > x_end || y_end > context->height || x_end > context->width) {
    return -1;
  }

  for (size_t y = y_begin; y < y_end; ++y) {
    for (size_t x = x_begin; x < x_end; ++x) {
      convolution_apply_pixel_unchecked(context, y, x);
    }
  }

  return 0;
}

int convolution_apply_rows(const convolution_context_t *context,
                           size_t y_begin,
                           size_t y_end) {
  if (!convolution_context_is_valid(context)) {
    return -1;
  }

  return convolution_apply_rect(context, y_begin, y_end, 0U, context->width);
}

int convolution_apply_cols(const convolution_context_t *context,
                           size_t x_begin,
                           size_t x_end) {
  if (!convolution_context_is_valid(context)) {
    return -1;
  }

  return convolution_apply_rect(context, 0U, context->height, x_begin, x_end);
}
