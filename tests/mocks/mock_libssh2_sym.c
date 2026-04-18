/*
 * mock_libssh2_sym.c -- Interposed libssh2 entry points.
 *
 * libssh2_channel_exec and libssh2_scp_send are preprocessor macros
 * that expand to libssh2_channel_process_startup / libssh2_scp_send_ex
 * respectively; those are what we actually define here.
 */

#include "mock_libssh2_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

/* ------------------------------------------------------------------ */
/* Session                                                            */
/* ------------------------------------------------------------------ */

int libssh2_init(int flags)
{
    (void)flags;
    mock_log_append("libssh2_init");
    return 0;
}

void libssh2_exit(void)
{
    mock_log_append("libssh2_exit");
}

LIBSSH2_SESSION *libssh2_session_init_ex(LIBSSH2_ALLOC_FUNC((*my_alloc)),
                                         LIBSSH2_FREE_FUNC((*my_free)),
                                         LIBSSH2_REALLOC_FUNC((*my_realloc)),
                                         void *abstract)
{
    LIBSSH2_SESSION *s;
    (void)my_alloc; (void)my_free; (void)my_realloc; (void)abstract;
    mock_log_append("libssh2_session_init");
    s = calloc(1, sizeof(*s));
    if (s)
        s->magic = SSH_SESSION_MAGIC;
    return s;
}

int libssh2_session_handshake(LIBSSH2_SESSION *session, libssh2_socket_t sock)
{
    (void)session; (void)sock;
    mock_log_append("libssh2_session_handshake");
    return 0;
}

void libssh2_session_set_blocking(LIBSSH2_SESSION *session, int blocking)
{
    (void)session; (void)blocking;
}

int libssh2_session_disconnect_ex(LIBSSH2_SESSION *session, int reason,
                                  const char *description, const char *lang)
{
    (void)session; (void)reason; (void)lang;
    mock_log_append("libssh2_session_disconnect(%s)",
                    description ? description : "");
    return 0;
}

int libssh2_session_free(LIBSSH2_SESSION *session)
{
    mock_log_append("libssh2_session_free");
    if (!session)
        return 0;
    session->magic = 0;
    free(session);
    return 0;
}

int libssh2_session_last_error(LIBSSH2_SESSION *session, char **errmsg,
                               int *errmsg_len, int want_buf)
{
    (void)want_buf;
    if (errmsg)
        *errmsg = (char *)"mock-last-error";
    if (errmsg_len)
        *errmsg_len = (int)strlen("mock-last-error");
    return session ? session->last_error : 0;
}

/* ------------------------------------------------------------------ */
/* Authentication                                                     */
/* ------------------------------------------------------------------ */

int libssh2_userauth_password_ex(LIBSSH2_SESSION *session,
                                 const char *username, unsigned int ulen,
                                 const char *password, unsigned int plen,
                                 LIBSSH2_PASSWD_CHANGEREQ_FUNC((*cb)))
{
    (void)session; (void)ulen; (void)plen; (void)cb;
    mock_log_append("libssh2_userauth_password(%s:%.4s...)",
                    username ? username : "",
                    password ? password : "");
    return g_ssh_auth_result;
}

int libssh2_userauth_publickey_fromfile_ex(LIBSSH2_SESSION *session,
                                           const char *username,
                                           unsigned int ulen,
                                           const char *publickey,
                                           const char *privatekey,
                                           const char *passphrase)
{
    (void)session; (void)ulen; (void)publickey; (void)privatekey;
    (void)passphrase;
    mock_log_append("libssh2_userauth_publickey_fromfile(%s)",
                    username ? username : "");
    return g_ssh_auth_result;
}

int libssh2_userauth_authenticated(LIBSSH2_SESSION *session)
{
    (void)session;
    return g_ssh_auth_result == 0 ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* Channels                                                           */
/* ------------------------------------------------------------------ */

LIBSSH2_CHANNEL *libssh2_channel_open_ex(LIBSSH2_SESSION *session,
                                         const char *channel_type,
                                         unsigned int ct_len,
                                         unsigned int window_size,
                                         unsigned int packet_size,
                                         const char *message,
                                         unsigned int msg_len)
{
    LIBSSH2_CHANNEL *c;
    (void)ct_len; (void)window_size; (void)packet_size;
    (void)message; (void)msg_len;

    mock_log_append("libssh2_channel_open(%s)",
                    channel_type ? channel_type : "");
    c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->magic   = SSH_CHANNEL_MAGIC;
    c->session = session;
    return c;
}

int libssh2_channel_process_startup(LIBSSH2_CHANNEL *channel,
                                    const char *request,
                                    unsigned int rlen,
                                    const char *message,
                                    unsigned int mlen)
{
    int i;
    (void)rlen;

    mock_log_append("libssh2_channel_process_startup(%s,msg=%.*s)",
                    request ? request : "",
                    (int)(mlen < 128 ? mlen : 128),
                    message ? message : "");

    if (!channel || !request || strcmp(request, "exec") != 0 || !message)
        return 0;

    if (mlen >= sizeof(channel->command))
        mlen = sizeof(channel->command) - 1;
    memcpy(channel->command, message, mlen);
    channel->command[mlen] = '\0';
    channel->exec_pending  = 1;
    channel->exit_code     = 0;

    for (i = 0; i < SSH_EXEC_SLOTS; i++) {
        mock_ssh_exec_stage_t *s = &g_ssh_exec_stages[i];
        if (!s->in_use || s->consumed)
            continue;
        if (strstr(channel->command, s->substr)) {
            s->consumed          = 1;
            channel->exit_code   = s->exit_code;
            channel->stdout_data = s->stdout_data;
            channel->stdout_len  = s->stdout_len;
            channel->stdout_pos  = 0;
            s->stdout_data       = NULL; /* ownership transferred */
            s->stdout_len        = 0;
            break;
        }
    }
    return 0;
}

ssize_t libssh2_channel_read_ex(LIBSSH2_CHANNEL *channel, int stream_id,
                                char *buf, size_t buflen)
{
    size_t remaining;
    size_t copy;
    (void)stream_id;

    if (!channel || !buf)
        return -1;
    if (!channel->stdout_data || channel->stdout_pos >= channel->stdout_len)
        return 0;

    remaining = channel->stdout_len - channel->stdout_pos;
    copy = remaining < buflen ? remaining : buflen;
    memcpy(buf, (uint8_t *)channel->stdout_data + channel->stdout_pos, copy);
    channel->stdout_pos += copy;
    return (ssize_t)copy;
}

ssize_t libssh2_channel_write_ex(LIBSSH2_CHANNEL *channel, int stream_id,
                                 const char *buf, size_t buflen)
{
    (void)stream_id; (void)buf;
    if (!channel)
        return -1;
    if (channel->scp_mode)
        channel->scp_bytes_written += buflen;
    return (ssize_t)buflen;
}

int libssh2_channel_send_eof(LIBSSH2_CHANNEL *c)    { (void)c; return 0; }
int libssh2_channel_wait_eof(LIBSSH2_CHANNEL *c)    { (void)c; return 0; }
int libssh2_channel_close(LIBSSH2_CHANNEL *c)       { (void)c; return 0; }
int libssh2_channel_wait_closed(LIBSSH2_CHANNEL *c) { (void)c; return 0; }

int libssh2_channel_get_exit_status(LIBSSH2_CHANNEL *c)
{
    return c ? c->exit_code : -1;
}

int libssh2_channel_free(LIBSSH2_CHANNEL *c)
{
    mock_log_append("libssh2_channel_free");
    if (!c)
        return 0;

    if (c->scp_mode && g_ssh_scp_expect_active) {
        g_ssh_scp_expect_active = 0;
        if (g_ssh_scp_path_out)
            *g_ssh_scp_path_out = strdup(c->scp_path);
        if (g_ssh_scp_bytes_out)
            *g_ssh_scp_bytes_out = c->scp_bytes_written;
    }
    free(c->stdout_data);
    c->magic = 0;
    free(c);
    return 0;
}

int libssh2_channel_eof(LIBSSH2_CHANNEL *c)
{
    if (!c) return 1;
    if (!c->stdout_data) return 1;
    return c->stdout_pos >= c->stdout_len ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* SCP upload                                                         */
/* ------------------------------------------------------------------ */

LIBSSH2_CHANNEL *libssh2_scp_send_ex(LIBSSH2_SESSION *session,
                                     const char *path, int mode,
                                     size_t size, long mtime, long atime)
{
    LIBSSH2_CHANNEL *c;
    (void)mode; (void)mtime; (void)atime;

    mock_log_append("libssh2_scp_send(path=%s,size=%zu)",
                    path ? path : "", size);

    /* Phase 3: tests can force SCP to fail at channel open. */
    if (g_ssh_scp_force_error) {
        g_ssh_scp_force_error = 0;
        mock_log_append("libssh2_scp_send: forced error (NULL channel)");
        return NULL;
    }

    c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->magic    = SSH_CHANNEL_MAGIC;
    c->session  = session;
    c->scp_mode = 1;
    if (path)
        snprintf(c->scp_path, sizeof(c->scp_path), "%s", path);
    return c;
}

LIBSSH2_CHANNEL *libssh2_scp_send64(LIBSSH2_SESSION *session, const char *path,
                                    int mode, libssh2_int64_t size,
                                    time_t mtime, time_t atime)
{
    return libssh2_scp_send_ex(session, path, mode, (size_t)size,
                               (long)mtime, (long)atime);
}
