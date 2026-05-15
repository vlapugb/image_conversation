#pragma once

#include <stddef.h>

typedef enum execution_mode {
    EXECUTION_MODE_SEQ = 0,
    EXECUTION_MODE_ROWS,
    EXECUTION_MODE_PIXELS,
    EXECUTION_MODE_COLS,
    EXECUTION_MODE_GRID,
    EXECUTION_MODE_GPU,
    EXECUTION_MODE_HYBRID,
} execution_mode_t;

typedef struct execution_config {
    execution_mode_t mode;
    size_t threads;
    size_t grid_rows;
    size_t grid_cols;
} execution_config_t;

static inline execution_config_t make_execution_config(execution_mode_t mode, size_t threads, size_t grid_rows, size_t grid_cols) {
    return (execution_config_t){
        .mode = mode,
        .threads = threads,
        .grid_rows = grid_rows,
        .grid_cols = grid_cols,
    };
}
