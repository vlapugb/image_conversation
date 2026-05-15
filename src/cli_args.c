#include "cli_args.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_IMG_CAP 4
#define MIN_ARGS 12

static void cli_set_invalid(char *err, size_t size) {
    if (err && size > 0) snprintf(err, size, "invalid");
}

static cli_parse_status_t cli_invalid(char *err, size_t size) {
    cli_set_invalid(err, size);
    return CLI_PARSE_ERROR;
}

static bool parse_size(const char *txt, size_t *val) {
    if (!txt || !val || *txt == '\0') return false;
    char *end = NULL;
    unsigned long p = strtoul(txt, &end, 10);
    if (*end != '\0' || p == 0) return false;
    *val = (size_t)p;
    return true;
}

static bool parse_filter_kind(const char *txt, filter_kind_t *k) {
    if (strcmp(txt, "blur") == 0) *k = FILTER_KIND_BLUR;
    else if (strcmp(txt, "mean") == 0) *k = FILTER_KIND_MEAN;
    else if (strcmp(txt, "gauss") == 0) *k = FILTER_KIND_GAUSSIAN_BLUR;
    else if (strcmp(txt, "motion") == 0) *k = FILTER_KIND_MOTION_BLUR;
    else if (strcmp(txt, "edge") == 0) *k = FILTER_KIND_EDGE_DETECT;
    else if (strcmp(txt, "sharpen") == 0) *k = FILTER_KIND_SHARPEN;
    else if (strcmp(txt, "emboss") == 0) *k = FILTER_KIND_EMBOSS;
    else if (strcmp(txt, "median") == 0) *k = FILTER_KIND_MEDIAN;
    else return false;
    return true;
}

static bool parse_direction(const char *txt, filter_direction_t *d) {
    if (strcmp(txt, "horizontal") == 0) *d = FILTER_DIRECTION_HORIZONTAL;
    else if (strcmp(txt, "vertical") == 0) *d = FILTER_DIRECTION_VERTICAL;
    else if (strcmp(txt, "diagonal") == 0) *d = FILTER_DIRECTION_DIAGONAL_45;
    else if (strcmp(txt, "omni") == 0) *d = FILTER_DIRECTION_OMNIDIRECTIONAL;
    else return false;
    return true;
}

static bool parse_mode(const char *txt, execution_mode_t *m) {
    if (strcmp(txt, "cols") == 0) *m = EXECUTION_MODE_COLS;
    else if (strcmp(txt, "rows") == 0 || strcmp(txt, "raws") == 0) *m = EXECUTION_MODE_ROWS;
    else if (strcmp(txt, "pixels") == 0) *m = EXECUTION_MODE_PIXELS;
    else if (strcmp(txt, "grid") == 0 || strcmp(txt, "rectangle") == 0 || strcmp(txt, "random") == 0) *m = EXECUTION_MODE_GRID;
    else if (strcmp(txt, "gpu") == 0) *m = EXECUTION_MODE_GPU;
    else if (strcmp(txt, "hybrid") == 0) *m = EXECUTION_MODE_HYBRID;
    else return false;
    return true;
}

void cli_request_destroy(cli_request_t *req) {
    if (req) free(req->images);
}

static bool add_image(cli_request_t *req, const char *in, const char *out) {
    if (!req || !in || !out) return false;
    if (req->image_count == req->image_capacity) {
        size_t cap = req->image_capacity == 0 ? INIT_IMG_CAP : req->image_capacity * 2;
        cli_image_io_t *next = realloc(req->images, sizeof(*req->images) * cap);
        if (!next) return false;
        req->images = next;
        req->image_capacity = cap;
    }
    req->images[req->image_count++] = (cli_image_io_t){.input_path = in, .output_path = out};
    return true;
}

static bool parse_filter_spec(int argc, char **argv, int *idx, cli_filter_spec_t *f) {
    if (*idx + 5 >= argc) return false;
    if (strcmp(argv[*idx], "-f") != 0 || strcmp(argv[*idx + 2], "-h") != 0 || strcmp(argv[*idx + 4], "-w") != 0) return false;
    if (!parse_filter_kind(argv[*idx + 1], &f->kind) || !parse_size(argv[*idx + 3], &f->height) || !parse_size(argv[*idx + 5], &f->width)) return false;
    *idx += 6;
    if (*idx + 1 < argc && strcmp(argv[*idx], "-t") == 0) {
        if (!parse_direction(argv[*idx + 1], &f->direction)) return false;
        *idx += 2;
    }
    return true;
}

cli_parse_status_t cli_parse_args(int argc, char **argv, cli_request_t *req, char *err, size_t err_size) {
    if (!req || !argv) return cli_invalid(err, err_size);
    memset(req, 0, sizeof(*req));
    req->mode = EXECUTION_MODE_SEQ;
    for (size_t i = 0; i < CLI_MAX_FILTERS; ++i) {
        req->filters[i].direction = FILTER_DIRECTION_NONE;
        req->filters[i].border_mode = FILTER_BORDER_WRAP;
    }

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        cli_print_help(stdout, argv[0]);
        return CLI_PARSE_HELP;
    }

    if (argc < MIN_ARGS) return cli_invalid(err, err_size);

    int idx = 1;
    while (idx < argc && strcmp(argv[idx], "-i") == 0) {
        if (idx + 3 >= argc || strcmp(argv[idx + 2], "-o") != 0) break;
        if (!add_image(req, argv[idx + 1], argv[idx + 3])) break;
        idx += 4;
    }

    if (req->image_count == 0) {
        cli_request_destroy(req);
        return cli_invalid(err, err_size);
    }

    if (!parse_filter_spec(argc, argv, &idx, &req->filters[0])) {
        cli_request_destroy(req);
        return cli_invalid(err, err_size);
    }
    req->filter_count = 1;

    if (idx < argc && strcmp(argv[idx], "-f") == 0) {
        if (!parse_filter_spec(argc, argv, &idx, &req->filters[1])) {
            cli_request_destroy(req);
            return cli_invalid(err, err_size);
        }
        req->filter_count = 2;
    }

    if (idx >= argc) {
        cli_request_destroy(req);
        return cli_invalid(err, err_size);
    }

    if (strcmp(argv[idx], "-s") == 0 && idx + 1 == argc) {
        req->mode = EXECUTION_MODE_SEQ;
    } else if (strcmp(argv[idx], "-p") == 0 && idx + 2 == argc) {
        if (!parse_mode(argv[idx + 1], &req->mode)) {
            cli_request_destroy(req);
            return cli_invalid(err, err_size);
        }
    } else {
        cli_request_destroy(req);
        return cli_invalid(err, err_size);
    }

    return CLI_PARSE_OK;
}

void cli_print_help(FILE *s, const char *prog) {
    const char *n = prog ? prog : "main";
    fprintf(s, "Usage:\n"
               "  %s (-i <in> -o <out>)+ -f <filt> -h <h> -w <w> [-t <type>] -s\n"
               "  %s (-i <in> -o <out>)+ -f <filt> -h <h> -w <w> [-t <type>] -p <mode>\n"
               "Modes: cols, rows, pixels, grid, gpu, hybrid\n", n, n);
}
