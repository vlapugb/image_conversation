#pragma once

#include <stddef.h>

typedef struct image_view {
  unsigned char *data;
  size_t height;
  size_t width;
  size_t stride;
  size_t channels;
} image_view_t;
