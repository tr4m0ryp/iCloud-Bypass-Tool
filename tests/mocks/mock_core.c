/*
 * mock_core.c -- Cross-cutting mock machinery: call log, reset, helpers.
 */

#include "mock_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mock_log_t g_mock_log;

/* ------------------------------------------------------------------ */
/* Call log                                                           */
/* ------------------------------------------------------------------ */

void mock_log_reset(void)
{
    g_mock_log.count = 0;
    /* No need to clear individual entries; count gates access. */
}

void mock_log_append(const char *fmt, ...)
{
    va_list ap;

    if (g_mock_log.count >= MOCK_LOG_MAX)
        return;

    va_start(ap, fmt);
    vsnprintf(g_mock_log.entries[g_mock_log.count],
              MOCK_LOG_ENTRY_LEN, fmt, ap);
    va_end(ap);
    g_mock_log.count++;
}

size_t mock_get_call_count(void)
{
    return g_mock_log.count;
}

const char *mock_get_call_log(size_t index)
{
    if (index >= g_mock_log.count)
        return NULL;
    return g_mock_log.entries[index];
}

int mock_call_log_contains(const char *needle)
{
    size_t i;

    if (!needle)
        return 0;
    for (i = 0; i < g_mock_log.count; i++) {
        if (strstr(g_mock_log.entries[i], needle))
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Global reset                                                       */
/* ------------------------------------------------------------------ */

void mock_reset_all(void)
{
    mock_log_reset();
    mock_lockdownd_reset();
    mock_irecv_reset();
    mock_libusb_reset();
    mock_ssh_reset();
    mock_curl_reset();
    mock_checkm8_reset();
    mock_afc_mobileactivation_reset();
}

/* ------------------------------------------------------------------ */
/* Small helpers                                                      */
/* ------------------------------------------------------------------ */

void *mock_memdup(const void *src, size_t len)
{
    void *copy;

    if (!src || len == 0)
        return NULL;
    copy = malloc(len);
    if (!copy)
        return NULL;
    memcpy(copy, src, len);
    return copy;
}

char *mock_strdup_safe(const char *src)
{
    if (!src)
        return NULL;
    return strdup(src);
}
