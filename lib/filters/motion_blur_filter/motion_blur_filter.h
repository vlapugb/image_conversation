#pragma once

#include <filters/filter.h>

filter_status_t init_motion_blur_filter(filter_t *filter,
                                        size_t width,
                                        size_t height);
