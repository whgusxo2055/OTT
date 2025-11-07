#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include "logger.h"

static log_level_t current_log_level = LOG_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char* level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void logger_init(log_level_t level) {
    current_log_level = level;
}

static void log_message(log_level_t level, const char *format, va_list args) {
    if (level < current_log_level) {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    // Get current time
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[26];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // Print timestamp and level
    fprintf(stderr, "[%s] [%s] ", time_buf, level_strings[level]);

    // Print message
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fflush(stderr);

    pthread_mutex_unlock(&log_mutex);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, format, args);
    va_end(args);
}

void log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARN, format, args);
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_ERROR, format, args);
    va_end(args);
}
