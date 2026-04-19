#include <filters/filter.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct builtin_case {
  const char *name;
  filter_request_t request;
  filter_kind_t expected_kind;
  filter_direction_t expected_direction;
  filter_operator_t expected_operator;
  size_t expected_rank_index;
} builtin_case_t;

static int report_failure(const char *context, const char *message) {
  fprintf(stderr, "%s: %s\n", context, message);
  return 1;
}

static int expect_status(const char *context,
                         filter_status_t actual,
                         filter_status_t expected) {
  if (actual != expected) {
    fprintf(stderr,
            "%s: unexpected status %d, expected %d\n",
            context,
            (int)actual,
            (int)expected);
    return 1;
  }

  return 0;
}

static int test_supported_cases(void) {
  const builtin_case_t cases[] = {
    {
      .name = "blur",
      .request = {
        .kind = FILTER_KIND_BLUR,
        .width = 3U,
        .height = 3U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_CLAMP,
      },
      .expected_kind = FILTER_KIND_BLUR,
      .expected_direction = FILTER_DIRECTION_NONE,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "mean",
      .request = {
        .kind = FILTER_KIND_MEAN,
        .width = 5U,
        .height = 5U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_REFLECT,
      },
      .expected_kind = FILTER_KIND_MEAN,
      .expected_direction = FILTER_DIRECTION_NONE,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "gaussian_blur",
      .request = {
        .kind = FILTER_KIND_GAUSSIAN_BLUR,
        .width = 3U,
        .height = 3U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_WRAP,
      },
      .expected_kind = FILTER_KIND_GAUSSIAN_BLUR,
      .expected_direction = FILTER_DIRECTION_NONE,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "motion_blur",
      .request = {
        .kind = FILTER_KIND_MOTION_BLUR,
        .width = 9U,
        .height = 9U,
        .direction = FILTER_DIRECTION_DIAGONAL_45,
        .border_mode = FILTER_BORDER_CLAMP,
      },
      .expected_kind = FILTER_KIND_MOTION_BLUR,
      .expected_direction = FILTER_DIRECTION_DIAGONAL_45,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "edge_detect",
      .request = {
        .kind = FILTER_KIND_EDGE_DETECT,
        .width = 3U,
        .height = 3U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_REFLECT,
      },
      .expected_kind = FILTER_KIND_EDGE_DETECT,
      .expected_direction = FILTER_DIRECTION_OMNIDIRECTIONAL,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "sharpen",
      .request = {
        .kind = FILTER_KIND_SHARPEN,
        .width = 5U,
        .height = 5U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_WRAP,
      },
      .expected_kind = FILTER_KIND_SHARPEN,
      .expected_direction = FILTER_DIRECTION_NONE,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "emboss",
      .request = {
        .kind = FILTER_KIND_EMBOSS,
        .width = 3U,
        .height = 3U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_CLAMP,
      },
      .expected_kind = FILTER_KIND_EMBOSS,
      .expected_direction = FILTER_DIRECTION_DIAGONAL_45,
      .expected_operator = FILTER_OPERATOR_CONVOLUTION,
      .expected_rank_index = 0U,
    },
    {
      .name = "median",
      .request = {
        .kind = FILTER_KIND_MEDIAN,
        .width = 3U,
        .height = 5U,
        .direction = FILTER_DIRECTION_NONE,
        .border_mode = FILTER_BORDER_REFLECT,
      },
      .expected_kind = FILTER_KIND_MEDIAN,
      .expected_direction = FILTER_DIRECTION_NONE,
      .expected_operator = FILTER_OPERATOR_RANK_SELECTION,
      .expected_rank_index = 7U,
    },
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    filter_t filter;
    memset(&filter, 0, sizeof(filter));

    if (expect_status(cases[i].name,
                      filter_init_builtin(&filter, &cases[i].request),
                      FILTER_STATUS_OK) != 0) {
      return 1;
    }

    if (filter.kind != cases[i].expected_kind) {
      return report_failure(cases[i].name, "unexpected kind");
    }
    if (filter.direction != cases[i].expected_direction) {
      return report_failure(cases[i].name, "unexpected direction");
    }
    if (filter.operator != cases[i].expected_operator) {
      return report_failure(cases[i].name, "unexpected operator");
    }
    if (filter.width != cases[i].request.width ||
        filter.height != cases[i].request.height) {
      return report_failure(cases[i].name, "unexpected dimensions");
    }
    if (filter.border_mode != cases[i].request.border_mode) {
      return report_failure(cases[i].name, "border mode was not propagated");
    }
    if (filter.rank_index != cases[i].expected_rank_index) {
      return report_failure(cases[i].name, "unexpected rank index");
    }

    if (cases[i].expected_operator == FILTER_OPERATOR_CONVOLUTION &&
        filter.kernel == NULL) {
      return report_failure(cases[i].name, "convolution kernel is missing");
    }
    if (cases[i].expected_operator == FILTER_OPERATOR_RANK_SELECTION &&
        filter.kernel != NULL) {
      return report_failure(cases[i].name, "rank filter should not expose a kernel");
    }
  }

  return 0;
}

static int test_error_paths(void) {
  filter_t filter;
  filter_request_t request = make_filter_request(FILTER_KIND_BLUR, 3U, 3U);

  if (expect_status("null_filter",
                    filter_init_builtin(NULL, &request),
                    FILTER_STATUS_NULL_POINTER) != 0) {
    return 1;
  }

  if (expect_status("null_request",
                    filter_init_builtin(&filter, NULL),
                    FILTER_STATUS_NULL_POINTER) != 0) {
    return 1;
  }

  request.direction = FILTER_DIRECTION_VERTICAL;
  if (expect_status("blur_unsupported_direction",
                    filter_init_builtin(&filter, &request),
                    FILTER_STATUS_UNSUPPORTED_DIRECTION) != 0) {
    return 1;
  }

  request.kind = FILTER_KIND_EMBOSS;
  request.direction = FILTER_DIRECTION_HORIZONTAL;
  if (expect_status("emboss_unsupported_direction",
                    filter_init_builtin(&filter, &request),
                    FILTER_STATUS_UNSUPPORTED_DIRECTION) != 0) {
    return 1;
  }

  request = make_filter_request((filter_kind_t)999, 3U, 3U);
  if (expect_status("unsupported_kind",
                    filter_init_builtin(&filter, &request),
                    FILTER_STATUS_UNSUPPORTED_KIND) != 0) {
    return 1;
  }

  return 0;
}

int main(void) {
  if (test_supported_cases() != 0) {
    return 1;
  }

  if (test_error_paths() != 0) {
    return 1;
  }

  return 0;
}
