/*
 * mock_libssh2_internal.h -- Private types shared between mock_libssh2_*.c
 * files.  Split out to keep each source file under 300 lines.
 */

#ifndef TR4MPASS_MOCK_LIBSSH2_INTERNAL_H
#define TR4MPASS_MOCK_LIBSSH2_INTERNAL_H

#include "mock_internal.h"

#include <stdint.h>
#include <stddef.h>

#include <libssh2.h>

#define SSH_SESSION_MAGIC 0x53534853u /* "SSHS" */
#define SSH_CHANNEL_MAGIC 0x53534843u /* "SSHC" */

struct _LIBSSH2_SESSION {
    uint32_t magic;
    int      last_error;
    int      scp_mode;
    char     scp_path[512];
    size_t   scp_expected;
};

struct _LIBSSH2_CHANNEL {
    uint32_t          magic;
    LIBSSH2_SESSION  *session;

    /* Canned exec response */
    int               exec_pending;
    char              command[512];
    int               exit_code;
    void             *stdout_data;
    size_t            stdout_len;
    size_t            stdout_pos;

    /* SCP state */
    int               scp_mode;
    char              scp_path[512];
    size_t            scp_bytes_written;
};

#define SSH_EXEC_SLOTS 16

typedef struct {
    int    in_use;
    int    consumed;
    char   substr[256];
    int    exit_code;
    void  *stdout_data;
    size_t stdout_len;
} mock_ssh_exec_stage_t;

/* Shared state: defined in mock_libssh2_ctl.c, referenced from _sym.c. */
extern mock_ssh_exec_stage_t g_ssh_exec_stages[SSH_EXEC_SLOTS];
extern char                 **g_ssh_scp_path_out;
extern size_t                *g_ssh_scp_bytes_out;
extern int                    g_ssh_scp_expect_active;
extern int                    g_ssh_auth_result;

/* Phase 3: when non-zero, the next libssh2_scp_send*() call returns NULL
 * (simulating a mid-stream SCP failure).  Consumed by the first call so
 * subsequent SCP attempts succeed unless re-armed. */
extern int                    g_ssh_scp_force_error;

#endif /* TR4MPASS_MOCK_LIBSSH2_INTERNAL_H */
