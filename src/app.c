#include <convolution/convolution.h>
#include <gpu_convolution/gpu_convolution.h>
#include <parallel_convolution/parallel_convolution.h>
#include <pipeline/image_pipeline_runner.h>
#include <sequentially_convolution/sequentially_convolution.h>
#include "cli_args.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_ERR_LEN 32

static double elapsed_ms(const struct timespec *start, const struct timespec *end) {
    double s = (double)(end->tv_sec - start->tv_sec) * 1000.0;
    double ns = (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;
    return s + ns;
}

static image_convolution_runner_t select_runner(execution_mode_t mode) {
    switch (mode) {
        case EXECUTION_MODE_SEQ:    return sequential_convolution;
        case EXECUTION_MODE_ROWS:   return parallel_convolution_rows;
        case EXECUTION_MODE_PIXELS: return parallel_convolution_pixels;
        case EXECUTION_MODE_COLS:   return parallel_convolution_cols;
        case EXECUTION_MODE_GRID:   return parallel_convolution_rectangle;
        case EXECUTION_MODE_GPU:    return gpu_convolution;
        case EXECUTION_MODE_HYBRID: return sequential_convolution;
        default:                    return NULL;
    }
}

static void fill_request(image_pipeline_request_t *pipe_req, const cli_request_t *cli_req, size_t idx, image_convolution_runner_t runner) {
    pipe_req->input_path = cli_req->images[idx].input_path;
    pipe_req->output_path = cli_req->images[idx].output_path;
    pipe_req->filter_count = cli_req->filter_count;
    pipe_req->run_convolution = runner;

    for (size_t i = 0; i < cli_req->filter_count; ++i) {
        pipe_req->filters[i] = (filter_request_t){
            .kind = cli_req->filters[i].kind,
            .width = cli_req->filters[i].width,
            .height = cli_req->filters[i].height,
            .direction = cli_req->filters[i].direction,
            .border_mode = cli_req->filters[i].border_mode,
        };
    }
}

int main(int argc, char **argv) {
    cli_request_t req;
    char err_msg[MAX_ERR_LEN];
    cli_parse_status_t status = cli_parse_args(argc, argv, &req, err_msg, sizeof(err_msg));

    if (status == CLI_PARSE_HELP) return 0;
    if (status != CLI_PARSE_OK) {
        if (err_msg[0] != '\0') puts(err_msg);
        return -1;
    }

    image_convolution_runner_t base_runner = select_runner(req.mode);
    if (!base_runner) {
        cli_request_destroy(&req);
        return -1;
    }

    image_pipeline_request_t *pipe_reqs = malloc(sizeof(*pipe_reqs) * req.image_count);
    for (size_t i = 0; i < req.image_count; ++i) {
        image_convolution_runner_t r = base_runner;
        if (req.mode == EXECUTION_MODE_HYBRID) {
            r = (i % 2 == 0) ? parallel_convolution_rows : gpu_convolution;
        }
        fill_request(&pipe_reqs[i], &req, i, r);
    }

    struct timespec start, end;
    timespec_get(&start, TIME_UTC);
    int res = image_pipeline_run(pipe_reqs, req.image_count);
    timespec_get(&end, TIME_UTC);

    printf("processing time: %.3f ms\n", elapsed_ms(&start, &end));

    free(pipe_reqs);
    cli_request_destroy(&req);
    return res == 0 ? 0 : -1;
}
