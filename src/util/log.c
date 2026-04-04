#include <stdio.h>
#include <stdarg.h>
#include "util/log.h"

#define ANSI_RESET  "\033[0m"
#define ANSI_WHITE  "\033[37m"
#define ANSI_GREEN  "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RED    "\033[31m"

static log_level_t g_level = LOG_INFO;

void log_set_level(log_level_t level)
{
    g_level = level;
}

static void log_vprint(log_level_t level, const char *color,
                       const char *tag, const char *fmt, va_list ap)
{
    if (level < g_level)
        return;
    fprintf(stderr, "%s%s ", color, tag);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "%s\n", ANSI_RESET);
}

void log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprint(LOG_DEBUG, ANSI_WHITE, "[DEBUG]", fmt, ap);
    va_end(ap);
}

void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprint(LOG_INFO, ANSI_GREEN, "[INFO]", fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprint(LOG_WARN, ANSI_YELLOW, "[WARN]", fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_vprint(LOG_ERROR, ANSI_RED, "[ERROR]", fmt, ap);
    va_end(ap);
}
