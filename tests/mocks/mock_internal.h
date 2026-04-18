/*
 * mock_internal.h -- Private state and helpers shared between mock_*.c files.
 *
 * Not part of the public staging API.  Only mock_*.c should include this.
 */

#ifndef TR4MPASS_MOCK_INTERNAL_H
#define TR4MPASS_MOCK_INTERNAL_H

#include "mock_control.h"

#include <stddef.h>
#include <stdint.h>

/* Number of entries in each staging table.  Tests typically stage only a
 * handful of values; 32 is a comfortable upper bound. */
#define MOCK_STAGE_SLOTS 32

/* Maximum length of a single call-log entry after formatting. */
#define MOCK_LOG_ENTRY_LEN 256

/* ------------------------------------------------------------------ */
/* Shared call log                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    char entries[MOCK_LOG_MAX][MOCK_LOG_ENTRY_LEN];
    size_t count;
} mock_log_t;

extern mock_log_t g_mock_log;

void mock_log_reset(void);

/* ------------------------------------------------------------------ */
/* Per-subsystem reset hooks.  Each mock_*.c implements one of these. */
/* ------------------------------------------------------------------ */

void mock_lockdownd_reset(void);
void mock_irecv_reset(void);
void mock_libusb_reset(void);
void mock_ssh_reset(void);
void mock_curl_reset(void);
void mock_checkm8_reset(void);
void mock_afc_mobileactivation_reset(void);

/* ------------------------------------------------------------------ */
/* Tiny helpers                                                       */
/* ------------------------------------------------------------------ */

/* Duplicate a blob; caller frees. Returns NULL on OOM or if src==NULL. */
void *mock_memdup(const void *src, size_t len);

/* Safe strdup wrapper returning NULL if src is NULL. */
char *mock_strdup_safe(const char *src);

#endif /* TR4MPASS_MOCK_INTERNAL_H */
