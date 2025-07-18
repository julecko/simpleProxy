#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG_ULTRA,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

typedef enum {
    LOG_FLAG_NONE         = 0,
    LOG_FLAG_NO_TIMESTAMP = 1 << 0,
    LOG_FLAG_NO_LEVEL     = 1 << 1
} LoggerFlags;

void logger_init(FILE *output_normal, FILE *output_error, LogLevel level, LoggerFlags flags);
void logger_set_level(LogLevel level);
void logger_log_normal(LogLevel level, const char *fmt, ...);
void logger_log_error(LogLevel level, const char *fmt, ...);
void logger_close();

#ifdef DEBUG_MODE
#define log_debug(...) logger_log_normal(LOG_DEBUG, __VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif

#ifdef DEBUG_MODE
#define log_debug_ultra(...) logger_log_normal(LOG_DEBUG, __VA_ARGS__)
#else
#define log_debug_ultra(...) ((void)0)
#endif

#define log_info(...)  logger_log_normal(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  logger_log_normal(LOG_WARN,  __VA_ARGS__)
#define log_error(...) logger_log_normal(LOG_ERROR, __VA_ARGS__)

#endif
