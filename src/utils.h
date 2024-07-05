#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define LOG_INFO "[info]"
#define LOG_DEBUG "[debug]"
#define LOG_WARNING "[warning]"
#define LOG_ERROR "[error]"
#define LOG_FATAL "[fatal]"

#define log_message(level, format, ...) \
    do { \
        printf("%s: ", level); \
        printf(format, ##__VA_ARGS__); \
        printf("\n"); \
    } while (0)


#define LAYOUT_MODE_TO_STRING(layout_mode) \
    (layout_mode == FLOATING_MODE) ? "floating" : \
    (layout_mode == STACKING_MODE) ? "stacking" : \
    (layout_mode == COLUMNS_MODE) ? "columns" : \
    (layout_mode == ROWS_MODE) ? "rows" : \
    (layout_mode == N_COLUMNS_MODE) ? "n-columns" : \
    (layout_mode == N_ROWS_MODE) ? "n-rows" : \
    (layout_mode == MONOCLE_MODE) ? "monocle" : "unknown"

#define STRING_TO_LAYOUT_MODE(str) \
    ( \
        !strcmp(str, "FLOATING_MODE") ? FLOATING_MODE : \
        !strcmp(str, "STACKING_MODE") ? STACKING_MODE : \
        !strcmp(str, "COLUMNS_MODE") ? COLUMNS_MODE : \
        !strcmp(str, "ROWS_MODE") ? ROWS_MODE : \
        !strcmp(str, "N_COLUMNS_MODE") ? N_COLUMNS_MODE : \
        !strcmp(str, "N_ROWS_MODE") ? N_ROWS_MODE : \
        !strcmp(str, "MONOCLE_MODE") ? MONOCLE_MODE : \
        -1 \
    )

#endif