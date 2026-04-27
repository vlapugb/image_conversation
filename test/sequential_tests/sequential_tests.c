#include <filters/filter.h>
#include <image_helpers/image_helpers.h>
#include <sequentially_convolution/sequentially_convolution.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cmocka.h>

#define MAX_IMAGE_BYTES 4096U
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef struct test_image {
  unsigned char data[MAX_IMAGE_BYTES];
  image_view_t view;
  size_t size;
} test_image_t;

static uint32_t random_state = 0x12345678U;

static unsigned char random_byte(void) {
  random_state = random_state * 1664525U + 1013904223U;
  return (unsigned char)(random_state >> 24U);
}

static void init_image(test_image_t *image,
                       size_t width,
                       size_t height,
                       size_t channels,
                       size_t padding) {
  const size_t stride = width * channels + padding;

  image->size = height * stride;
  image->view = (image_view_t){
    .data = image->data,
    .height = height,
    .width = width,
    .stride = stride,
    .channels = channels,
  };

  memset(image->data, 0xCD, sizeof(image->data));
}

static void fill_random(test_image_t *image) {
  for (size_t i = 0; i < image->size; ++i) {
    image->data[i] = random_byte();
  }
}

static void copy_image(test_image_t *to, const test_image_t *from) {
  memcpy(to->data, from->data, from->size);
}

static int same_image(const test_image_t *left, const test_image_t *right) {
  if (left->size != right->size) {
    return 0;
  }
  return memcmp(left->data, right->data, left->size) == 0;
}

static filter_t make_filter(const double *kernel,
                            size_t width,
                            size_t height,
                            double factor,
                            filter_border_mode_t border_mode) {
  return make_convolution_filter(FILTER_KIND_BLUR,
                                 FILTER_CATEGORY_SMOOTHING,
                                 FILTER_DIRECTION_NONE,
                                 border_mode,
                                 kernel,
                                 factor,
                                 0.0,
                                 width,
                                 height);
}

static void apply_filter(test_image_t *image,
                        const double *kernel,
                        size_t width,
                        size_t height,
                        double factor,
                        filter_border_mode_t border_mode) {
  filter_t filter = make_filter(kernel, width, height, factor, border_mode);
  assert_int_equal(sequential_convolution(&filter, &image->view), 0);
}

static void make_identity_kernel(double *kernel, size_t size) {
  memset(kernel, 0, size * size * sizeof(*kernel));
  kernel[(size / 2U) * size + size / 2U] = 1.0;
}

static void make_shift_kernel(double *kernel, int dx, int dy) {
  const int center = 1;

  memset(kernel, 0, 9U * sizeof(*kernel));
  kernel[(size_t)(center + dy) * 3U + (size_t)(center + dx)] = 1.0;
}

static unsigned char *pixel(test_image_t *image, size_t x, size_t y) {
  return image->data + y * image->view.stride + x * image->view.channels;
}

static const unsigned char *
const_pixel(const test_image_t *image, size_t x, size_t y) {
  return image->data + y * image->view.stride + x * image->view.channels;
}

static void set_one_channel_image(test_image_t *image,
                                  const unsigned char *values) {
  for (size_t y = 0; y < image->view.height; ++y) {
    for (size_t x = 0; x < image->view.width; ++x) {
      *pixel(image, x, y) = values[y * image->view.width + x];
    }
  }
}

static int one_channel_image_has_values(const test_image_t *image,
                                        const unsigned char *values) {
  for (size_t y = 0; y < image->view.height; ++y) {
    for (size_t x = 0; x < image->view.width; ++x) {
      if (*const_pixel(image, x, y) != values[y * image->view.width + x]) {
        return 0;
      }
    }
  }
  return 1;
}

static void identity_filter_keeps_image(void **state) {
  (void)state;
  const size_t kernel_sizes[] = {1U, 3U, 5U, 7U};
  const filter_border_mode_t borders[] = {
    FILTER_BORDER_WRAP,
    FILTER_BORDER_CLAMP,
    FILTER_BORDER_REFLECT,
  };

  for (size_t channels = 1U; channels <= 4U; ++channels) {
    if (channels == 2U) {
      continue;
    }

    for (size_t i = 0; i < ARRAY_SIZE(kernel_sizes);
         ++i) {
      double kernel[49];
      test_image_t original;
      test_image_t actual;

      init_image(&original, 8U, 5U, channels, 3U);
      init_image(&actual, 8U, 5U, channels, 3U);

      fill_random(&original);
      make_identity_kernel(kernel, kernel_sizes[i]);

      for (size_t b = 0; b < ARRAY_SIZE(borders); ++b) {
        copy_image(&actual, &original);
        apply_filter(
          &actual, kernel, kernel_sizes[i], kernel_sizes[i], 1.0, borders[b]);
        assert_true(same_image(&original, &actual));
      }
    }
  }

}

static void zero_filter_makes_rgb_black_and_keeps_alpha(void **state) {
  (void)state;
  double zero_kernel[9] = {0.0};
  test_image_t original;
  test_image_t actual;

  init_image(&original, 7U, 4U, 4U, 2U);
  init_image(&actual, 7U, 4U, 4U, 2U);

  fill_random(&original);
  copy_image(&actual, &original);

  apply_filter(&actual, zero_kernel, 3U, 3U, 1.0, FILTER_BORDER_REFLECT);

  for (size_t y = 0; y < actual.view.height; ++y) {
    for (size_t x = 0; x < actual.view.width; ++x) {
      const unsigned char *before = const_pixel(&original, x, y);
      const unsigned char *after = const_pixel(&actual, x, y);

      assert_int_equal(after[0], 0U);
      assert_int_equal(after[1], 0U);
      assert_int_equal(after[2], 0U);
      assert_int_equal(after[3], before[3]);
    }
  }

}

static void shift_filter_respects_border_modes(void **state) {
  (void)state;
  double shift_right[9];

  // clang-format off
  const unsigned char source_values[] = {
    1U, 2U, 3U,
    4U, 5U, 6U,
  };
  const unsigned char wrap_expected[] = {
    2U, 3U, 1U,
    5U, 6U, 4U,
  };
  const unsigned char clamp_expected[] = {
    2U, 3U, 3U,
    5U, 6U, 6U,
  };
  const unsigned char reflect_expected[] = {
    2U, 3U, 2U,
    5U, 6U, 5U,
  };
  // clang-format on

  test_image_t image;

  init_image(&image, 3U, 2U, 1U, 1U);

  make_shift_kernel(shift_right, 1, 0);

  set_one_channel_image(&image, source_values);
  apply_filter(&image, shift_right, 3U, 3U, 1.0, FILTER_BORDER_WRAP);
  assert_true(one_channel_image_has_values(&image, wrap_expected));

  set_one_channel_image(&image, source_values);
  apply_filter(&image, shift_right, 3U, 3U, 1.0, FILTER_BORDER_CLAMP);
  assert_true(one_channel_image_has_values(&image, clamp_expected));

  set_one_channel_image(&image, source_values);
  apply_filter(&image, shift_right, 3U, 3U, 1.0, FILTER_BORDER_REFLECT);
  assert_true(one_channel_image_has_values(&image, reflect_expected));

}

static void opposite_shifts_compose_to_identity_with_wrap(void **state) {
  (void)state;
  double shift_right[9];
  double shift_left[9];
  double shift_down[9];
  double shift_up[9];
  test_image_t original;
  test_image_t actual;

  init_image(&original, 9U, 7U, 3U, 4U);
  init_image(&actual, 9U, 7U, 3U, 4U);

  make_shift_kernel(shift_right, 1, 0);
  make_shift_kernel(shift_left, -1, 0);
  make_shift_kernel(shift_down, 0, 1);
  make_shift_kernel(shift_up, 0, -1);

  fill_random(&original);

  copy_image(&actual, &original);
  apply_filter(&actual, shift_right, 3U, 3U, 1.0, FILTER_BORDER_WRAP);
  apply_filter(&actual, shift_left, 3U, 3U, 1.0, FILTER_BORDER_WRAP);
  assert_true(same_image(&original, &actual));

  copy_image(&actual, &original);
  apply_filter(&actual, shift_down, 3U, 3U, 1.0, FILTER_BORDER_WRAP);
  apply_filter(&actual, shift_up, 3U, 3U, 1.0, FILTER_BORDER_WRAP);
  assert_true(same_image(&original, &actual));

}

static void zero_padded_kernel_gives_same_result(void **state) {
  (void)state;

  // clang-format off
  const double kernel_3x3[9] = {
    0.0, 1.0, 0.0,
    1.0, 4.0, 1.0,
    0.0, 1.0, 0.0,
  };
  const double same_kernel_padded_to_5x5[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0,
    0.0, 1.0, 4.0, 1.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
  };
  // clang-format on

  const filter_border_mode_t borders[] = {
    FILTER_BORDER_WRAP,
    FILTER_BORDER_CLAMP,
    FILTER_BORDER_REFLECT,
  };
  test_image_t source;
  test_image_t small_kernel_result;
  test_image_t padded_kernel_result;

  init_image(&source, 10U, 6U, 3U, 2U);
  init_image(&small_kernel_result, 10U, 6U, 3U, 2U);
  init_image(&padded_kernel_result, 10U, 6U, 3U, 2U);

  fill_random(&source);

  for (size_t i = 0; i < ARRAY_SIZE(borders); ++i) {
    copy_image(&small_kernel_result, &source);
    copy_image(&padded_kernel_result, &source);

    apply_filter(&small_kernel_result, kernel_3x3, 3U, 3U, 1.0, borders[i]);
    apply_filter(&padded_kernel_result,
                 same_kernel_padded_to_5x5,
                 5U,
                 5U,
                 1.0,
                 borders[i]);
    assert_true(same_image(&small_kernel_result, &padded_kernel_result));
  }

}

static void known_wrap_mean_3x3(void **state) {
  (void)state;
  double mean_kernel[9];
  test_image_t image;

  init_image(&image, 3U, 3U, 1U, 0U);

  // clang-format off
  const unsigned char values[] = {
    1U, 2U, 3U,
    4U, 5U, 6U,
    7U, 8U, 9U,
  };
  const unsigned char expected[] = {
    5U, 5U, 5U,
    5U, 5U, 5U,
    5U, 5U, 5U,
  };
  // clang-format on

  for (size_t i = 0; i < 9U; ++i) {
    mean_kernel[i] = 1.0;
  }

  set_one_channel_image(&image, values);
  apply_filter(&image, mean_kernel, 3U, 3U, 1.0 / 9.0, FILTER_BORDER_WRAP);
  assert_true(one_channel_image_has_values(&image, expected));

}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(identity_filter_keeps_image),
    cmocka_unit_test(zero_filter_makes_rgb_black_and_keeps_alpha),
    cmocka_unit_test(shift_filter_respects_border_modes),
    cmocka_unit_test(opposite_shifts_compose_to_identity_with_wrap),
    cmocka_unit_test(zero_padded_kernel_gives_same_result),
    cmocka_unit_test(known_wrap_mean_3x3),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
