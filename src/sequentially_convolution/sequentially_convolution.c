#include "sequentially_convolution.h"

#include <convolution/convolution.h>

static int sequential_executor(const convolution_context_t *context) {
  return convolution_apply_rect(
    context, 0U, context->height, 0U, context->width);
}

int sequential_convolution(const filter_t *filter, image_view_t *image_view) {
  return convolution_run(filter, image_view, sequential_executor);
}
