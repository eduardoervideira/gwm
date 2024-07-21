#include "utils.h"
#include <stdarg.h>
#include <time.h>

FILE *log_file = NULL;

void get_timestamp(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
}

void init_log() {
    log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        fprintf(stderr, "unable to open log file: %s\n", LOG_FILE);
    }
}

void close_log() {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(const char *level, const char *format, ...) {
    if (log_file) {
        va_list args;
        va_start(args, format);

        char timestamp[20];
        get_timestamp(timestamp, sizeof(timestamp));

        fprintf(log_file, "(%s) %s: ", timestamp, level);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);

        va_end(args);
        va_start(args, format);

        printf("(%s) %s: ", timestamp, level);
        vprintf(format, args);
        printf("\n");

        va_end(args);
    } else {
        fprintf(stderr, "log file is not open\n");
    }
}