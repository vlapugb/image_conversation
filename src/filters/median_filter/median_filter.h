#pragma once

#include <filters/filter.h>

filter_status_t init_median_filter(filter_t *filter,
                                   size_t width,
                                   size_t height);
