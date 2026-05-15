#pragma once

#include <stddef.h>

#include "image_pipeline.h"

int image_pipeline_run(const image_pipeline_request_t *requests,
                       size_t request_count);