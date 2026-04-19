#pragma once

#include <filters/filter.h>

filter_status_t init_find_edges_filter(filter_t *filter,
                                       size_t width,
                                       size_t height,
                                       filter_direction_t direction);
