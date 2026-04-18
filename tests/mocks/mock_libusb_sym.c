/*
 * mock_libusb_sym.c -- Interposed libusb-1.0 entry points.
 */

#include "mock_libusb_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int libusb_init(libusb_context **ctx)
{
    mock_log_append("libusb_init");
    if (ctx) {
        struct libusb_context *c = calloc(1, sizeof(*c));
        if (!c) return LIBUSB_ERROR_NO_MEM;
        c->magic = CTX_MAGIC;
        *ctx = c;
    }
    return 0;
}

int libusb_init_context(libusb_context **ctx,
                        const struct libusb_init_option options[],
                        int num_options)
{
    (void)options; (void)num_options;
    return libusb_init(ctx);
}

void libusb_exit(libusb_context *ctx)
{
    mock_log_append("libusb_exit");
    if (ctx) {
        ctx->magic = 0;
        free(ctx);
    }
}

void libusb_set_debug(libusb_context *ctx, int level)
{
    (void)ctx; (void)level;
}

int libusb_set_option(libusb_context *ctx, enum libusb_option option, ...)
{
    (void)ctx; (void)option;
    return 0;
}

const char *libusb_error_name(int error_code)
{
    switch (error_code) {
    case 0:                         return "LIBUSB_SUCCESS";
    case LIBUSB_ERROR_IO:           return "LIBUSB_ERROR_IO";
    case LIBUSB_ERROR_NO_DEVICE:    return "LIBUSB_ERROR_NO_DEVICE";
    case LIBUSB_ERROR_NOT_FOUND:    return "LIBUSB_ERROR_NOT_FOUND";
    case LIBUSB_ERROR_PIPE:         return "LIBUSB_ERROR_PIPE";
    case LIBUSB_ERROR_TIMEOUT:      return "LIBUSB_ERROR_TIMEOUT";
    default:                        return "LIBUSB_ERROR_OTHER";
    }
}

const char *libusb_strerror(int errcode)
{
    return libusb_error_name(errcode);
}

/* ------------------------------------------------------------------ */
/* Device enumeration                                                 */
/* ------------------------------------------------------------------ */

ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list)
{
    struct libusb_device **arr;
    int i;

    (void)ctx;
    mock_log_append("libusb_get_device_list(count=%d)", g_libusb_list_count);

    if (!list)
        return LIBUSB_ERROR_INVALID_PARAM;
    if (g_libusb_list_count <= 0) {
        *list = NULL;
        return 0;
    }
    arr = calloc((size_t)g_libusb_list_count + 1,
                 sizeof(struct libusb_device *));
    if (!arr) return LIBUSB_ERROR_NO_MEM;
    for (i = 0; i < g_libusb_list_count; i++) {
        arr[i] = calloc(1, sizeof(struct libusb_device));
        if (arr[i]) {
            arr[i]->vid  = g_libusb_list_vid;
            arr[i]->pid  = g_libusb_list_pid;
            arr[i]->bus  = 1;
            arr[i]->addr = (uint8_t)(i + 1);
        }
    }
    arr[g_libusb_list_count] = NULL;
    *list = arr;
    return g_libusb_list_count;
}

void libusb_free_device_list(libusb_device **list, int unref)
{
    int i;
    (void)unref;
    if (!list) return;
    for (i = 0; list[i]; i++)
        free(list[i]);
    free(list);
}

int libusb_get_device_descriptor(libusb_device *dev,
                                 struct libusb_device_descriptor *desc)
{
    if (!dev || !desc) return LIBUSB_ERROR_INVALID_PARAM;
    memset(desc, 0, sizeof(*desc));
    desc->bLength         = 18;
    desc->bDescriptorType = 0x01;
    desc->idVendor        = dev->vid;
    desc->idProduct       = dev->pid;
    desc->iSerialNumber   = 3;
    return 0;
}

uint8_t libusb_get_bus_number(libusb_device *dev)
{
    return dev ? dev->bus : 0;
}

uint8_t libusb_get_device_address(libusb_device *dev)
{
    return dev ? dev->addr : 0;
}

int libusb_open(libusb_device *dev, libusb_device_handle **handle)
{
    struct libusb_device_handle *h;
    mock_log_append("libusb_open");
    if (!dev || !handle) return LIBUSB_ERROR_INVALID_PARAM;
    h = calloc(1, sizeof(*h));
    if (!h) return LIBUSB_ERROR_NO_MEM;
    h->dev = dev;
    *handle = h;
    return 0;
}

void libusb_close(libusb_device_handle *handle)
{
    mock_log_append("libusb_close");
    free(handle);
}

libusb_device *libusb_get_device(libusb_device_handle *h)
{
    return h ? h->dev : NULL;
}

int libusb_claim_interface(libusb_device_handle *h, int iface)
{
    (void)h;
    mock_log_append("libusb_claim_interface(%d)", iface);
    return 0;
}

int libusb_release_interface(libusb_device_handle *h, int iface)
{
    (void)h;
    mock_log_append("libusb_release_interface(%d)", iface);
    return 0;
}

int libusb_detach_kernel_driver(libusb_device_handle *h, int iface)
{
    (void)h; (void)iface;
    return 0;
}

int libusb_reset_device(libusb_device_handle *h)
{
    (void)h;
    mock_log_append("libusb_reset_device");
    return 0;
}

int libusb_set_configuration(libusb_device_handle *h, int configuration)
{
    (void)h; (void)configuration;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Transfers                                                          */
/* ------------------------------------------------------------------ */

int libusb_control_transfer(libusb_device_handle *h,
                            uint8_t bmRequestType, uint8_t bRequest,
                            uint16_t wValue, uint16_t wIndex,
                            unsigned char *data, uint16_t wLength,
                            unsigned int timeout)
{
    int i;
    (void)h; (void)timeout;

    mock_log_append("libusb_control_transfer(rt=0x%02x,req=0x%02x,val=0x%04x,idx=0x%04x,len=%u)",
                    bmRequestType, bRequest, wValue, wIndex, wLength);

    for (i = 0; i < MOCK_CTRL_SLOTS; i++) {
        mock_libusb_ctrl_stage_t *s = &g_libusb_ctrl_stages[i];
        if (!s->in_use || s->consumed)
            continue;
        s->consumed = 1;
        if (data && s->data && s->data_len > 0) {
            size_t copy = s->data_len;
            if (copy > (size_t)wLength)
                copy = (size_t)wLength;
            memcpy(data, s->data, copy);
        }
        return s->return_code;
    }
    /* No stage set: return wLength (success, all bytes transferred). */
    return (int)wLength;
}

int libusb_bulk_transfer(libusb_device_handle *h, unsigned char endpoint,
                         unsigned char *data, int length,
                         int *transferred, unsigned int timeout)
{
    (void)h; (void)data; (void)timeout;
    mock_log_append("libusb_bulk_transfer(ep=0x%02x,len=%d)",
                    endpoint, length);
    if (transferred)
        *transferred = length;
    return 0;
}
