#include "./core/logger.h"
#include <stdarg.h>
#include <time.h>

static FILE *log_output = NULL;
static LogLevel current_level = LOG_INFO;

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void logger_init(FILE *output, LogLevel level) {
    log_output = output ? output : stderr;
    current_level = level;
}

void logger_set_level(LogLevel level) {
    current_level = level;
}

void logger_log(LogLevel level, const char *fmt, ...) {
    if (level < current_level || !log_output) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log_output, "[%s] [%s] ", timestamp, level_names[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_output, fmt, args);
    va_end(args);

    fprintf(log_output, "\n");
    fflush(log_output);
}

void logger_close() {
    log_output = NULL;
}
