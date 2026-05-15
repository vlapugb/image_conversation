#pragma once

#include <filters/filter.h>
#include <image_helpers/image_helpers.h>

int gpu_convolution(const filter_t *filter, image_view_t *image_view);
