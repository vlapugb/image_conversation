#pragma once

#include <stddef.h>
#include <stdio.h>

#include <app/execution_config.h>
#include <filters/filter.h>

typedef enum cli_parse_status {
  CLI_PARSE_OK = 0,
  CLI_PARSE_HELP,
  CLI_PARSE_ERROR,
} cli_parse_status_t;

typedef struct cli_filter_spec {
  filter_kind_t kind;
  size_t width;
  size_t height;
  filter_direction_t direction;
  filter_border_mode_t border_mode;
} cli_filter_spec_t;

typedef struct cli_request {
  const char *input_path;
  const char *output_path;
  execution_mode_t mode;
  cli_filter_spec_t filters[2];
  size_t filter_count;
} cli_request_t;

cli_parse_status_t cli_parse_args(int argc,
                                  char **argv,
                                  cli_request_t *request,
                                  char *error_message,
                                  size_t error_message_size);

void cli_print_help(FILE *stream, const char *program_name);
