#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>

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

#define MAX_IMAGE_BYTES 2048U
#define TEST_NAME_SIZE 128U
#define RANDOM_PIXEL_MASK 63U
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef struct test_image {
  unsigned char data[MAX_IMAGE_BYTES];
  image_view_t view;
  size_t size;
} test_image_t;

typedef struct kernel_sample {
  const char *name;
  const double *values;
  size_t width;
  size_t height;
  double factor;
  double bias;
} kernel_sample_t;

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

  memset(image->data, 0, sizeof(image->data));
}

static unsigned char *pixel(test_image_t *image, size_t x, size_t y) {
  return image->data + y * image->view.stride + x * image->view.channels;
}

static const unsigned char *
const_pixel(const test_image_t *image, size_t x, size_t y) {
  return image->data + y * image->view.stride + x * image->view.channels;
}

static void fill_random(test_image_t *image) {
  for (size_t y = 0; y < image->view.height; ++y) {
    for (size_t x = 0; x < image->view.width; ++x) {
      unsigned char *current_pixel = pixel(image, x, y);

      for (size_t channel = 0; channel < image->view.channels; ++channel) {
        current_pixel[channel] = random_byte() & RANDOM_PIXEL_MASK;
      }
    }

    for (size_t i = image->view.width * image->view.channels;
         i < image->view.stride;
         ++i) {
      image->data[y * image->view.stride + i] = random_byte();
    }
  }
}

static void copy_image(test_image_t *to, const test_image_t *from) {
  memcpy(to->data, from->data, from->size);
  to->size = from->size;
  to->view = from->view;
  to->view.data = to->data;
}

static unsigned char clamp_to_byte(double value) {
  if (value < 0.0) {
    return 0U;
  }
  if (value > 255.0) {
    return 255U;
  }
  return (unsigned char)(value + 0.5);
}

static int cv_border(filter_border_mode_t border) {
  switch (border) {
  case FILTER_BORDER_WRAP:
    return IPL_BORDER_WRAP;
  case FILTER_BORDER_REFLECT:
    return IPL_BORDER_REFLECT_101;
  case FILTER_BORDER_CLAMP:
  default:
    return IPL_BORDER_REPLICATE;
  }
}

static const char *border_name(filter_border_mode_t border) {
  switch (border) {
  case FILTER_BORDER_WRAP:
    return "wrap";
  case FILTER_BORDER_REFLECT:
    return "reflect";
  case FILTER_BORDER_CLAMP:
  default:
    return "clamp";
  }
}

static filter_t make_test_filter(const kernel_sample_t *kernel,
                                 filter_border_mode_t border) {
  return make_convolution_filter(FILTER_KIND_BLUR,
                                 FILTER_CATEGORY_SMOOTHING,
                                 FILTER_DIRECTION_NONE,
                                 border,
                                 kernel->values,
                                 kernel->factor,
                                 kernel->bias,
                                 kernel->width,
                                 kernel->height);
}

static CvMat *make_cv_kernel(const kernel_sample_t *kernel) {
  CvMat *cv_kernel =
    cvCreateMat((int)kernel->height, (int)kernel->width, CV_64FC1);

  if (cv_kernel == NULL) {
    return NULL;
  }

  for (size_t y = 0; y < kernel->height; ++y) {
    for (size_t x = 0; x < kernel->width; ++x) {
      const double value = kernel->values[y * kernel->width + x];
      cvmSet(cv_kernel, (int)y, (int)x, value * kernel->factor);
    }
  }

  return cv_kernel;
}

static CvMat *run_opencv_filter(const test_image_t *source,
                                const kernel_sample_t *kernel,
                                filter_border_mode_t border) {
  const int channels = (int)source->view.channels;
  const int input_type = CV_MAKETYPE(CV_8U, channels);
  const int result_type = CV_MAKETYPE(CV_64F, channels);
  const int radius_x = (int)(kernel->width / 2U);
  const int radius_y = (int)(kernel->height / 2U);

  CvMat source_header;
  CvMat *padded = NULL;
  CvMat *result = NULL;
  CvMat *cv_kernel = NULL;

  cvInitMatHeader(&source_header,
                  (int)source->view.height,
                  (int)source->view.width,
                  input_type,
                  (void *)source->data,
                  (int)source->view.stride);

  padded = cvCreateMat((int)source->view.height + 2 * radius_y,
                       (int)source->view.width + 2 * radius_x,
                       input_type);
  result = cvCreateMat((int)source->view.height + 2 * radius_y,
                       (int)source->view.width + 2 * radius_x,
                       result_type);
  cv_kernel = make_cv_kernel(kernel);

  if (padded == NULL || result == NULL || cv_kernel == NULL) {
    cvReleaseMat(&padded);
    cvReleaseMat(&result);
    cvReleaseMat(&cv_kernel);
    return NULL;
  }

  /* cvFilter2D from OpenCV 2 C API always uses its own border mode.
     To compare the same border handling as our code, pad the image first. */
  cvCopyMakeBorder(&source_header,
                   padded,
                   cvPoint(radius_x, radius_y),
                   cv_border(border),
                   cvScalarAll(0.0));
  cvFilter2D(padded, result, cv_kernel, cvPoint(radius_x, radius_y));

  cvReleaseMat(&padded);
  cvReleaseMat(&cv_kernel);
  return result;
}

static int padding_is_same(const test_image_t *before,
                           const test_image_t *after) {
  const size_t row_pixels = after->view.width * after->view.channels;

  for (size_t y = 0; y < after->view.height; ++y) {
    const unsigned char *before_padding =
      before->data + y * before->view.stride + row_pixels;
    const unsigned char *after_padding =
      after->data + y * after->view.stride + row_pixels;
    const size_t padding_size = after->view.stride - row_pixels;

    if (memcmp(before_padding, after_padding, padding_size) != 0) {
      return 0;
    }
  }

  return 1;
}

static int compare_with_opencv(const test_image_t *source,
                               const test_image_t *actual,
                               const kernel_sample_t *kernel,
                               const CvMat *opencv_result,
                               const char *test_name) {
  const int radius_x = (int)(kernel->width / 2U);
  const int radius_y = (int)(kernel->height / 2U);

  for (size_t y = 0; y < actual->view.height; ++y) {
    const double *opencv_row =
      (const double *)(const void *)(opencv_result->data.ptr +
                                     ((int)y + radius_y) * opencv_result->step);

    for (size_t x = 0; x < actual->view.width; ++x) {
      const unsigned char *source_pixel = const_pixel(source, x, y);
      const unsigned char *actual_pixel = const_pixel(actual, x, y);

      for (size_t channel = 0; channel < actual->view.channels; ++channel) {
        const double cv_value =
          opencv_row[((int)x + radius_x) * actual->view.channels + channel] +
          kernel->bias;
        const unsigned char expected =
          (actual->view.channels == 4U && channel == 3U)
            ? source_pixel[channel]
            : clamp_to_byte(cv_value);

        if (actual_pixel[channel] != expected) {
          fail_msg("%s: pixel (%zu, %zu), channel %zu: got %u, expected %u",
                   test_name,
                   x,
                   y,
                   channel,
                   (unsigned)actual_pixel[channel],
                   (unsigned)expected);
          return 0;
        }
      }
    }
  }

  if (!padding_is_same(source, actual)) {
    fail_msg("%s: image row padding changed", test_name);
    return 0;
  }

  return 1;
}

static void check_case(size_t width,
                       size_t height,
                       size_t channels,
                       const kernel_sample_t *kernel,
                       filter_border_mode_t border) {
  const size_t padding = (width + height + channels) % 4U;
  char test_name[TEST_NAME_SIZE];
  test_image_t source;
  test_image_t actual;
  CvMat *opencv_result = NULL;

  snprintf(test_name,
           sizeof(test_name),
           "%zux%zux%zu, %s, %s border",
           width,
           height,
           channels,
           kernel->name,
           border_name(border));

  init_image(&source, width, height, channels, padding);
  fill_random(&source);
  copy_image(&actual, &source);

  filter_t filter = make_test_filter(kernel, border);
  assert_int_equal(sequential_convolution(&filter, &actual.view), 0);

  opencv_result = run_opencv_filter(&source, kernel, border);
  assert_non_null(opencv_result);

  assert_true(compare_with_opencv(&source,
                                  &actual,
                                  kernel,
                                  opencv_result,
                                  test_name));
  cvReleaseMat(&opencv_result);
}

static void sequential_convolution_matches_opencv(void **state) {
  (void)state;

  static const double identity[] = {1.0};
  static const double gaussian_3x3[] = {
    1.0, 2.0, 1.0,
    2.0, 4.0, 2.0,
    1.0, 2.0, 1.0,
  };
  static const double sharpen_3x3[] = {
     0.0, -1.0,  0.0,
    -1.0,  5.0, -1.0,
     0.0, -1.0,  0.0,
  };
  static const double tall_3x5[] = {
     0.0, 1.0,  0.0,
    -1.0, 2.0, -1.0,
     0.0, 3.0,  0.0,
    -1.0, 2.0, -1.0,
     0.0, 1.0,  0.0,
  };
  static const double wide_5x3[] = {
     0.0, -1.0, 0.0,  1.0, 0.0,
     1.0,  2.0, 3.0,  2.0, 1.0,
     0.0,  1.0, 0.0, -1.0, 0.0,
  };

  const size_t widths[] = {1U, 2U, 3U, 5U, 8U, 13U};
  const size_t heights[] = {1U, 2U, 4U, 7U};
  const size_t channels[] = {1U, 3U, 4U};
  const filter_border_mode_t borders[] = {
    FILTER_BORDER_WRAP,
    FILTER_BORDER_CLAMP,
    FILTER_BORDER_REFLECT,
  };
  const kernel_sample_t kernels[] = {
    {"identity", identity, 1U, 1U, 1.0, 0.0},
    {"gaussian 3x3", gaussian_3x3, 3U, 3U, 1.0 / 16.0, 0.0},
    {"sharpen 3x3", sharpen_3x3, 3U, 3U, 1.0, 0.0},
    {"3x5", tall_3x5, 3U, 5U, 1.0, 3.0},
    {"5x3", wide_5x3, 5U, 3U, 1.0, 7.0},
  };

  for (size_t width_index = 0; width_index < ARRAY_SIZE(widths); ++width_index) {
    for (size_t height_index = 0; height_index < ARRAY_SIZE(heights);
         ++height_index) {
      for (size_t channel_index = 0; channel_index < ARRAY_SIZE(channels);
           ++channel_index) {
        for (size_t kernel_index = 0; kernel_index < ARRAY_SIZE(kernels);
             ++kernel_index) {
          for (size_t border_index = 0; border_index < ARRAY_SIZE(borders);
               ++border_index) {
            check_case(widths[width_index],
                       heights[height_index],
                       channels[channel_index],
                       &kernels[kernel_index],
                       borders[border_index]);
          }
        }
      }
    }
  }
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(sequential_convolution_matches_opencv),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
