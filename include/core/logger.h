#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

void logger_init(FILE *output, LogLevel level);
void logger_set_level(LogLevel level);
void logger_log(LogLevel level, const char *fmt, ...);
void logger_close();

#define log_debug(...) logger_log(LOG_DEBUG, __VA_ARGS__)
#define log_info(...)  logger_log(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  logger_log(LOG_WARN,  __VA_ARGS__)
#define log_error(...) logger_log(LOG_ERROR, __VA_ARGS__)

#endif
