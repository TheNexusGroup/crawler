#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <time.h>
#include <stdio.h>

typedef enum {
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR
} LogLevel;

extern LogLevel current_log_level;  // Declare as extern

// Function declarations
void logr(LogLevel level, const char* format, ...);
void set_log_level(LogLevel level);  // Just declare the function

#endif // LOGGER_H