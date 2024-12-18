#include "logger.h"

LogLevel current_log_level = DEBUG;  // Define the global variable

// Color codes
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static const char* color_codes[] = {
    ANSI_COLOR_RED,
    ANSI_COLOR_GREEN,
    ANSI_COLOR_YELLOW,
    ANSI_COLOR_BLUE,
    ANSI_COLOR_MAGENTA,
    ANSI_COLOR_CYAN
};

void logr(LogLevel level, const char* format, ...) {
    if (level < current_log_level) return;
    
    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Remove newline
    
    const char* level_str;
    switch (level) {
        case VERBOSE: level_str = "VERBOSE"; break;
        case DEBUG:   level_str = "DEBUG"; break;
        case INFO:    level_str = "INFO"; break;
        case WARN:    level_str = "WARN"; break;
        case ERROR:   level_str = "ERROR"; break;
        default:      level_str = "UNKNOWN";
    }
    
    const char* color = color_codes[level];
    
    fprintf(stderr, "%s[%s] %s: %s", color, timestamp, level_str, ANSI_COLOR_RESET);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void setLogLevel(LogLevel level) {
    current_log_level = level;
}
