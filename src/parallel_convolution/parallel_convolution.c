#include "parallel_convolution.h"

#include <convolution/convolution.h>
#include <omp.h>

#define TILE_HEIGHT 32U
#define TILE_WIDTH 128U

static size_t min_size(size_t lhs, size_t rhs) {
  return lhs < rhs ? lhs : rhs;
}

static int rows_executor(const convolution_context_t *context) {
  int failed = 0;

#pragma omp parallel reduction(| : failed)
  {
    const size_t thread_index = (size_t)omp_get_thread_num();
    const size_t thread_count = (size_t)omp_get_num_threads();

    const size_t first_row = thread_index * context->height / thread_count;
    const size_t last_row =
      (thread_index + 1U) * context->height / thread_count;

    if (convolution_apply_rows(context, first_row, last_row) != 0) {
      failed = 1;
    }
  }

  return failed ? -1 : 0;
}

static int cols_executor(const convolution_context_t *context) {
  int failed = 0;

#pragma omp parallel reduction(| : failed)
  {
    const size_t thread_index = (size_t)omp_get_thread_num();
    const size_t thread_count = (size_t)omp_get_num_threads();

    const size_t first_col = thread_index * context->width / thread_count;
    const size_t last_col = (thread_index + 1U) * context->width / thread_count;

    if (convolution_apply_cols(context, first_col, last_col) != 0) {
      failed = 1;
    }
  }

  return failed ? -1 : 0;
}

static int pixels_executor(const convolution_context_t *context) {
  int failed = 0;
  const size_t pixel_count = context->height * context->width;

#pragma omp parallel for schedule(static) reduction(| : failed)
  for (size_t pixel_index = 0; pixel_index < pixel_count; ++pixel_index) {
    const size_t y = pixel_index / context->width;
    const size_t x = pixel_index % context->width;

    if (convolution_apply_pixel(context, y, x) != 0) {
      failed = 1;
    }
  }

  return failed ? -1 : 0;
}

static int tiles_executor(const convolution_context_t *context) {
  int failed = 0;

#pragma omp parallel for collapse(2) schedule(static) reduction(| : failed)
  for (size_t first_row = 0; first_row < context->height;
       first_row += TILE_HEIGHT) {
    for (size_t first_col = 0; first_col < context->width;
         first_col += TILE_WIDTH) {
      const size_t last_row =
        min_size(first_row + TILE_HEIGHT, context->height);
      const size_t last_col = min_size(first_col + TILE_WIDTH, context->width);

      if (convolution_apply_rect(
            context, first_row, last_row, first_col, last_col) != 0) {
        failed = 1;
      }
    }
  }

  return failed ? -1 : 0;
}

int parallel_convolution_rows(const filter_t *filter,
                              image_view_t *image_view) {
  return convolution_run(filter, image_view, rows_executor);
}

int parallel_convolution_cols(const filter_t *filter,
                              image_view_t *image_view) {
  return convolution_run(filter, image_view, cols_executor);
}

int parallel_convolution_pixels(const filter_t *filter,
                                image_view_t *image_view) {
  return convolution_run(filter, image_view, pixels_executor);
}

int parallel_convolution_rectangle(const filter_t *filter,
                                   image_view_t *image_view) {
  return convolution_run(filter, image_view, tiles_executor);
}
