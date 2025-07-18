#include "./core/logger.h"
#include <stdarg.h>
#include <time.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_CYAN    "\x1b[36m"

static const char *level_names[] = {
    COLOR_CYAN   "DEBUG ULTRA" COLOR_RESET,
    COLOR_CYAN   "DEBUG" COLOR_RESET,
    COLOR_GREEN  "INFO"  COLOR_RESET,
    COLOR_YELLOW "WARN"  COLOR_RESET,
    COLOR_RED    "ERROR" COLOR_RESET
};

static FILE *log_output_normal = NULL;
static FILE *log_output_error = NULL;
static LogLevel current_level = LOG_INFO;
static LoggerFlags logger_flags = LOG_FLAG_NONE;

void logger_init(FILE *output_normal, FILE *output_error, LogLevel level, LoggerFlags flags) {
    if (!output_normal || !output_error) {
        fprintf(stderr, "Initialization of logger failed, provide not null output file pointers\n");
        return;
    }

    log_output_normal = output_normal;
    log_output_error = output_error;
    current_level = level;
    logger_flags = flags;
}

void logger_set_level(LogLevel level) {
    current_level = level;
}

void logger_log(FILE *log_output, LogLevel level, const char *fmt, va_list args) {
    if (level < current_level || !log_output) return;

    if (!(logger_flags & LOG_FLAG_NO_TIMESTAMP)) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        fprintf(log_output, "[%s] ", timestamp);
    }

    if (!(logger_flags & LOG_FLAG_NO_LEVEL)) {
        fprintf(log_output, "[%s] ", level_names[level]);
    }

    vfprintf(log_output, fmt, args);
    fprintf(log_output, "\n");
    fflush(log_output);
}

void logger_log_normal(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(log_output_normal, level, fmt, args);
    va_end(args);
}

void logger_log_error(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    logger_log(log_output_error, level, fmt, args);
    va_end(args);
}

void logger_close() {
    log_output_normal = NULL;
    log_output_error = NULL;
}
