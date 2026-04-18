/*
 * mock_lockdownd_ctl.c -- Staging + reset for lockdownd/idevice mocks.
 */

#include "mock_lockdownd_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mock_lockdownd_stage_t g_lockdownd_stage[MOCK_STAGE_SLOTS];
int g_idevice_list_count = 1;
int g_handshake_error    = 0;

void mock_lockdownd_reset(void)
{
    int i;
    for (i = 0; i < MOCK_STAGE_SLOTS; i++) {
        free(g_lockdownd_stage[i].data);
        memset(&g_lockdownd_stage[i], 0, sizeof(g_lockdownd_stage[i]));
    }
    g_idevice_list_count = 1;
    g_handshake_error    = 0;
}

void mock_lockdownd_set_value(const char *domain, const char *key,
                              const char *data_type,
                              const void *data, size_t len)
{
    int i;
    if (!key || !data_type)
        return;
    for (i = 0; i < MOCK_STAGE_SLOTS; i++) {
        if (!g_lockdownd_stage[i].in_use) {
            mock_lockdownd_stage_t *s = &g_lockdownd_stage[i];
            snprintf(s->domain, sizeof(s->domain), "%s",
                     domain ? domain : "");
            snprintf(s->key, sizeof(s->key), "%s", key);
            snprintf(s->type, sizeof(s->type), "%s", data_type);
            s->data = mock_memdup(data, len);
            s->len  = len;
            s->in_use   = 1;
            s->consumed = 0;
            return;
        }
    }
}

void mock_idevice_set_device_list(int count)
{
    g_idevice_list_count = count;
}

void mock_lockdownd_set_handshake_error(int error_code)
{
    g_handshake_error = error_code;
}

mock_lockdownd_stage_t *find_lockdownd_stage(const char *domain,
                                             const char *key)
{
    int i;
    for (i = 0; i < MOCK_STAGE_SLOTS; i++) {
        mock_lockdownd_stage_t *s = &g_lockdownd_stage[i];
        if (!s->in_use || s->consumed)
            continue;
        if (strcmp(s->key, key ? key : "") != 0)
            continue;
        /* domain "" in stage means wildcard */
        if (s->domain[0] != '\0' && domain &&
            strcmp(s->domain, domain) != 0)
            continue;
        return s;
    }
    return NULL;
}
