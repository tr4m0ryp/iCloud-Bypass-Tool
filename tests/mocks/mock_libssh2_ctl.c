/*
 * mock_libssh2_ctl.c -- Staging / reset / state for the libssh2 mock.
 * The mocked libssh2 entry points live in mock_libssh2_sym.c.
 */

#include "mock_libssh2_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mock_ssh_exec_stage_t  g_ssh_exec_stages[SSH_EXEC_SLOTS];
char                 **g_ssh_scp_path_out;
size_t                *g_ssh_scp_bytes_out;
int                    g_ssh_scp_expect_active;
int                    g_ssh_auth_result;
int                    g_ssh_scp_force_error;

void mock_ssh_reset(void)
{
    int i;
    for (i = 0; i < SSH_EXEC_SLOTS; i++) {
        free(g_ssh_exec_stages[i].stdout_data);
        memset(&g_ssh_exec_stages[i], 0, sizeof(g_ssh_exec_stages[i]));
    }
    g_ssh_scp_path_out      = NULL;
    g_ssh_scp_bytes_out     = NULL;
    g_ssh_scp_expect_active = 0;
    g_ssh_auth_result       = 0;
    g_ssh_scp_force_error   = 0;
}

void mock_ssh_set_exec_response(const char *command_substring,
                                int exit_code,
                                const void *stdout_data, size_t stdout_len)
{
    int i;
    if (!command_substring)
        return;
    for (i = 0; i < SSH_EXEC_SLOTS; i++) {
        if (!g_ssh_exec_stages[i].in_use) {
            mock_ssh_exec_stage_t *s = &g_ssh_exec_stages[i];
            snprintf(s->substr, sizeof(s->substr), "%s", command_substring);
            s->exit_code   = exit_code;
            s->stdout_data = mock_memdup(stdout_data, stdout_len);
            s->stdout_len  = stdout_len;
            s->in_use      = 1;
            s->consumed    = 0;
            return;
        }
    }
}

void mock_ssh_expect_scp(char **path_out, size_t *bytes_out)
{
    g_ssh_scp_path_out      = path_out;
    g_ssh_scp_bytes_out     = bytes_out;
    g_ssh_scp_expect_active = 1;
}

void mock_ssh_set_auth_result(int error_code)
{
    g_ssh_auth_result = error_code;
}

/*
 * mock_ssh_force_scp_error -- on the next SCP send call, the mock returns
 * a NULL channel simulating a remote-side refusal.  The SUT's
 * ssh_scp_upload helper translates that into a -1 return.  The flag is
 * cleared after the first SCP call; tests can re-arm for multiple fails.
 */
void mock_ssh_force_scp_error(int err)
{
    g_ssh_scp_force_error = (err != 0) ? 1 : 0;
}
