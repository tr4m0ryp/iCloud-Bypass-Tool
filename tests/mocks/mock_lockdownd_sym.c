/*
 * mock_lockdownd_sym.c -- Interposed idevice_* and lockdownd_* entry points.
 */

#include "mock_lockdownd_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* idevice_*                                                          */
/* ------------------------------------------------------------------ */

idevice_error_t idevice_get_device_list(char ***devices, int *count)
{
    int i;
    char **arr;

    mock_log_append("idevice_get_device_list(count=%d)",
                    g_idevice_list_count);

    if (!devices || !count)
        return IDEVICE_E_INVALID_ARG;

    *count = g_idevice_list_count;
    if (g_idevice_list_count <= 0) {
        *devices = NULL;
        return IDEVICE_E_NO_DEVICE;
    }

    arr = calloc((size_t)g_idevice_list_count + 1, sizeof(char *));
    if (!arr)
        return IDEVICE_E_UNKNOWN_ERROR;

    for (i = 0; i < g_idevice_list_count; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "MOCKUDID%d", i);
        arr[i] = strdup(buf);
    }
    arr[g_idevice_list_count] = NULL;
    *devices = arr;
    return IDEVICE_E_SUCCESS;
}

idevice_error_t idevice_device_list_free(char **devices)
{
    int i;
    mock_log_append("idevice_device_list_free");
    if (!devices)
        return IDEVICE_E_SUCCESS;
    for (i = 0; devices[i]; i++)
        free(devices[i]);
    free(devices);
    return IDEVICE_E_SUCCESS;
}

idevice_error_t idevice_new(idevice_t *device, const char *udid)
{
    struct idevice_private *dev;
    mock_log_append("idevice_new(udid=%s)", udid ? udid : "(null)");
    if (!device)
        return IDEVICE_E_INVALID_ARG;
    dev = calloc(1, sizeof(*dev));
    if (!dev)
        return IDEVICE_E_UNKNOWN_ERROR;
    dev->magic = IDEV_MAGIC;
    if (udid)
        snprintf(dev->udid, sizeof(dev->udid), "%s", udid);
    *device = (idevice_t)dev;
    return IDEVICE_E_SUCCESS;
}

idevice_error_t idevice_new_with_options(idevice_t *device,
                                         const char *udid,
                                         enum idevice_options options)
{
    (void)options;
    return idevice_new(device, udid);
}

idevice_error_t idevice_free(idevice_t device)
{
    struct idevice_private *dev = (struct idevice_private *)device;
    mock_log_append("idevice_free");
    if (!dev)
        return IDEVICE_E_INVALID_ARG;
    if (dev->magic != IDEV_MAGIC)
        return IDEVICE_E_INVALID_ARG;
    dev->magic = 0;
    free(dev);
    return IDEVICE_E_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* lockdownd_*                                                        */
/* ------------------------------------------------------------------ */

lockdownd_error_t
lockdownd_client_new_with_handshake(idevice_t device,
                                    lockdownd_client_t *client,
                                    const char *label)
{
    struct lockdownd_client_private *c;
    mock_log_append("lockdownd_client_new_with_handshake(label=%s)",
                    label ? label : "(null)");
    if (!client)
        return LOCKDOWN_E_INVALID_ARG;
    if (g_handshake_error != 0)
        return (lockdownd_error_t)g_handshake_error;
    (void)device;
    c = calloc(1, sizeof(*c));
    if (!c)
        return LOCKDOWN_E_UNKNOWN_ERROR;
    c->magic = LCKD_MAGIC;
    if (label)
        snprintf(c->label, sizeof(c->label), "%s", label);
    *client = (lockdownd_client_t)c;
    return LOCKDOWN_E_SUCCESS;
}

lockdownd_error_t lockdownd_client_new(idevice_t device,
                                       lockdownd_client_t *client,
                                       const char *label)
{
    return lockdownd_client_new_with_handshake(device, client, label);
}

lockdownd_error_t lockdownd_client_free(lockdownd_client_t client)
{
    struct lockdownd_client_private *c = (struct lockdownd_client_private *)client;
    mock_log_append("lockdownd_client_free");
    if (!c)
        return LOCKDOWN_E_INVALID_ARG;
    c->magic = 0;
    free(c);
    return LOCKDOWN_E_SUCCESS;
}

lockdownd_error_t lockdownd_goodbye(lockdownd_client_t client)
{
    (void)client;
    mock_log_append("lockdownd_goodbye");
    return LOCKDOWN_E_SUCCESS;
}

lockdownd_error_t lockdownd_get_value(lockdownd_client_t client,
                                      const char *domain, const char *key,
                                      plist_t *value)
{
    mock_lockdownd_stage_t *stage;
    (void)client;

    mock_log_append("lockdownd_get_value(domain=%s,key=%s)",
                    domain ? domain : "(null)",
                    key    ? key    : "(null)");

    if (!value)
        return LOCKDOWN_E_INVALID_ARG;
    *value = NULL;

    stage = find_lockdownd_stage(domain, key);
    if (!stage)
        return LOCKDOWN_E_UNKNOWN_ERROR;

    if (strcmp(stage->type, "string") == 0) {
        *value = plist_new_string((const char *)stage->data);
    } else if (strcmp(stage->type, "uint") == 0) {
        uint64_t v = 0;
        if (stage->len == sizeof(uint64_t))
            memcpy(&v, stage->data, sizeof(v));
        *value = plist_new_uint(v);
    } else if (strcmp(stage->type, "bool") == 0) {
        uint8_t b = stage->data ? *(uint8_t *)stage->data : 0;
        *value = plist_new_bool(b);
    } else if (strcmp(stage->type, "data") == 0) {
        *value = plist_new_data((const char *)stage->data,
                                (uint64_t)stage->len);
    } else {
        return LOCKDOWN_E_UNKNOWN_ERROR;
    }

    stage->consumed = 1;
    return *value ? LOCKDOWN_E_SUCCESS : LOCKDOWN_E_UNKNOWN_ERROR;
}

lockdownd_error_t lockdownd_set_value(lockdownd_client_t client,
                                      const char *domain, const char *key,
                                      plist_t value)
{
    (void)client; (void)value;
    mock_log_append("lockdownd_set_value(domain=%s,key=%s)",
                    domain ? domain : "(null)",
                    key    ? key    : "(null)");
    return LOCKDOWN_E_SUCCESS;
}

lockdownd_error_t lockdownd_start_service(lockdownd_client_t client,
                                          const char *identifier,
                                          lockdownd_service_descriptor_t *service)
{
    lockdownd_service_descriptor_t svc;
    (void)client;
    mock_log_append("lockdownd_start_service(identifier=%s)",
                    identifier ? identifier : "(null)");
    if (!service)
        return LOCKDOWN_E_INVALID_ARG;
    svc = calloc(1, sizeof(*svc));
    if (!svc)
        return LOCKDOWN_E_UNKNOWN_ERROR;
    svc->port = 0xB00B;
    svc->ssl_enabled = 0;
    if (identifier)
        svc->identifier = strdup(identifier);
    *service = svc;
    return LOCKDOWN_E_SUCCESS;
}

lockdownd_error_t
lockdownd_service_descriptor_free(lockdownd_service_descriptor_t service)
{
    mock_log_append("lockdownd_service_descriptor_free");
    if (!service)
        return LOCKDOWN_E_SUCCESS;
    free(service->identifier);
    free(service);
    return LOCKDOWN_E_SUCCESS;
}

const char *lockdownd_strerror(lockdownd_error_t err)
{
    switch ((int)err) {
    case 0:  return "Success";
    case -1: return "Invalid argument";
    default: return "Mock lockdownd error";
    }
}
