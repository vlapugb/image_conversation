#include "cli_args.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void cli_set_invalid(char *error_message, size_t error_message_size) {
  if (error_message != NULL && error_message_size != 0U) {
    snprintf(error_message, error_message_size, "invalid");
  }
}

static cli_parse_status_t cli_invalid(char *error_message,
                                      size_t error_message_size) {
  cli_set_invalid(error_message, error_message_size);
  return CLI_PARSE_ERROR;
}

static bool cli_parse_size(const char *text, size_t *value) {
  char *end = NULL;
  unsigned long parsed = 0;

  if (text == NULL || value == NULL || *text == '\0') {
    return false;
  }

  parsed = strtoul(text, &end, 10);
  if (*end != '\0' || parsed == 0UL) {
    return false;
  }

  *value = (size_t)parsed;
  return true;
}

static bool cli_parse_filter(const char *text, filter_kind_t *kind) {
  if (strcmp(text, "blur") == 0) {
    *kind = FILTER_KIND_BLUR;
  } else if (strcmp(text, "mean") == 0) {
    *kind = FILTER_KIND_MEAN;
  } else if (strcmp(text, "gauss") == 0) {
    *kind = FILTER_KIND_GAUSSIAN_BLUR;
  } else if (strcmp(text, "motion") == 0) {
    *kind = FILTER_KIND_MOTION_BLUR;
  } else if (strcmp(text, "edge") == 0) {
    *kind = FILTER_KIND_EDGE_DETECT;
  } else if (strcmp(text, "sharpen") == 0) {
    *kind = FILTER_KIND_SHARPEN;
  } else if (strcmp(text, "emboss") == 0) {
    *kind = FILTER_KIND_EMBOSS;
  } else if (strcmp(text, "median") == 0) {
    *kind = FILTER_KIND_MEDIAN;
  } else {
    return false;
  }

  return true;
}

static bool cli_parse_type(const char *text, filter_direction_t *direction) {
  if (strcmp(text, "horizontal") == 0) {
    *direction = FILTER_DIRECTION_HORIZONTAL;
  } else if (strcmp(text, "vertical") == 0) {
    *direction = FILTER_DIRECTION_VERTICAL;
  } else if (strcmp(text, "diagonal") == 0) {
    *direction = FILTER_DIRECTION_DIAGONAL_45;
  } else if (strcmp(text, "omni") == 0) {
    *direction = FILTER_DIRECTION_OMNIDIRECTIONAL;
  } else {
    return false;
  }

  return true;
}

static bool cli_parse_execution_mode(const char *text, execution_mode_t *mode) {
  if (strcmp(text, "cols") == 0) {
    *mode = EXECUTION_MODE_COLS;
  } else if (strcmp(text, "rows") == 0 || strcmp(text, "raws") == 0) {
    *mode = EXECUTION_MODE_ROWS;
  } else if (strcmp(text, "pixels") == 0) {
    *mode = EXECUTION_MODE_PIXELS;
  } else if (strcmp(text, "grid") == 0 || strcmp(text, "rectangle") == 0 ||
             strcmp(text, "random") == 0) {
    *mode = EXECUTION_MODE_GRID;
  } else {
    return false;
  }

  return true;
}

static bool
cli_parse_io_paths(int argc, char **argv, int *index, cli_request_t *request) {
  if (*index + 3 >= argc) {
    return false;
  }

  if (strcmp(argv[*index], "-i") != 0 || strcmp(argv[*index + 2], "-o") != 0) {
    return false;
  }

  request->input_path = argv[*index + 1];
  request->output_path = argv[*index + 3];
  *index += 4;
  return true;
}

static void cli_init_request(cli_request_t *request) {
  memset(request, 0, sizeof(*request));
  request->mode = EXECUTION_MODE_SEQ;
  for (size_t i = 0; i < 2U; ++i) {
    request->filters[i].direction = FILTER_DIRECTION_NONE;
    request->filters[i].border_mode = FILTER_BORDER_WRAP;
  }
}

static bool cli_parse_filter_spec(int argc,
                                  char **argv,
                                  int *index,
                                  cli_filter_spec_t *filter) {
  if (*index + 5 >= argc) {
    return false;
  }

  if (strcmp(argv[*index], "-f") != 0 || strcmp(argv[*index + 2], "-h") != 0 ||
      strcmp(argv[*index + 4], "-w") != 0) {
    return false;
  }

  if (!cli_parse_filter(argv[*index + 1], &filter->kind) ||
      !cli_parse_size(argv[*index + 3], &filter->height) ||
      !cli_parse_size(argv[*index + 5], &filter->width)) {
    return false;
  }

  *index += 6;

  if (*index + 1 < argc && strcmp(argv[*index], "-t") == 0) {
    if (!cli_parse_type(argv[*index + 1], &filter->direction)) {
      return false;
    }
    *index += 2;
  }

  return true;
}

cli_parse_status_t cli_parse_args(int argc,
                                  char **argv,
                                  cli_request_t *request,
                                  char *error_message,
                                  size_t error_message_size) {
  int index = 1;

  if (request == NULL || argv == NULL) {
    return cli_invalid(error_message, error_message_size);
  }

  cli_init_request(request);

  if (error_message != NULL && error_message_size != 0U) {
    error_message[0] = '\0';
  }

  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    cli_print_help(stdout, argv[0]);
    return CLI_PARSE_HELP;
  }

  if (argc < 12) {
    return cli_invalid(error_message, error_message_size);
  }

  if (!cli_parse_io_paths(argc, argv, &index, request)) {
    return cli_invalid(error_message, error_message_size);
  }

  if (!cli_parse_filter_spec(argc, argv, &index, &request->filters[0])) {
    return cli_invalid(error_message, error_message_size);
  }

  request->filter_count = 1U;

  if (index < argc && strcmp(argv[index], "-f") == 0) {
    if (!cli_parse_filter_spec(argc, argv, &index, &request->filters[1])) {
      return cli_invalid(error_message, error_message_size);
    }
    request->filter_count = 2U;
  }

  if (index >= argc) {
    return cli_invalid(error_message, error_message_size);
  }

  if (strcmp(argv[index], "-s") == 0 && index + 1 == argc) {
    request->mode = EXECUTION_MODE_SEQ;
  } else if (strcmp(argv[index], "-p") == 0 && index + 2 == argc &&
             cli_parse_execution_mode(argv[index + 1], &request->mode)) {
  } else {
    return cli_invalid(error_message, error_message_size);
  }

  return CLI_PARSE_OK;
}

void cli_print_help(FILE *stream, const char *program_name) {
  const char *name = program_name != NULL ? program_name : "main";

  fprintf(stream,
          "Usage:\n"
          "  %s -i <input> -o <output> -f <filter> -h <height> -w <width> "
          "[-t <type>] -s\n"
          "  %s -i <input> -o <output> -f <filter> -h <height> -w <width> "
          "[-t <type>] -p "
          "<cols|rows|raws|pixels|grid|rectangle|random>\n"
          "  %s -i <input> -o <output> -f <filter1> -h <height1> -w <width1> "
          "[-t <type1>] "
          "-f <filter2> -h <height2> -w <width2> [-t <type2>] "
          "(-s | -p <cols|rows|raws|pixels|grid|rectangle|random>)\n",
          name,
          name,
          name);
}
