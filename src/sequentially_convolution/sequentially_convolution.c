#include "sequentially_convolution.h"

#include <sequentially_convolution/helper_functions.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int sequential_convolution(const filter_t *filter, image_view_t *image_view) {
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

  const size_t height = image_view->height;
  const size_t width = image_view->width;
  const size_t stride = image_view->stride;
  const size_t channels = image_view->channels;
  unsigned char *pixels = image_view->data;
  const size_t buffer_size = height * stride;

  unsigned char *source_copy = (unsigned char *)malloc(buffer_size);
  if (source_copy == NULL) {
    return -1;
  }

  memcpy(source_copy, pixels, buffer_size);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      unsigned char *dst_pixel = pixels + y * stride + x * channels;

      for (size_t channel = 0; channel < channels; ++channel) {
        if (channels == 4U && channel == 3U) {
          dst_pixel[channel] = source_copy[y * stride + x * channels + channel];
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
              resolve_index(src_x, width, filter->border_mode);
            const size_t image_y =
              resolve_index(src_y, height, filter->border_mode);

            const unsigned char *src_pixel =
              source_copy + image_y * stride + image_x * channels;
            const double kernel_value =
              filter->kernel[filter_y * filter->width + filter_x];

            sum += (double)src_pixel[channel] * kernel_value;
          }
        }

        dst_pixel[channel] = clamp_to_u8(filter->factor * sum + filter->bias);
      }
    }
  }

  free(source_copy);
  return 0;
}
