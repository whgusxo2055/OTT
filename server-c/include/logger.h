#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

// Initialize logger
void logger_init(log_level_t level);

// Log functions
void log_debug(const char *format, ...);
void log_info(const char *format, ...);
void log_warn(const char *format, ...);
void log_error(const char *format, ...);

// Macros for convenience
#define LOG_DEBUG(...) log_debug(__VA_ARGS__)
#define LOG_INFO(...) log_info(__VA_ARGS__)
#define LOG_WARN(...) log_warn(__VA_ARGS__)
#define LOG_ERROR(...) log_error(__VA_ARGS__)

#endif // LOGGER_H
