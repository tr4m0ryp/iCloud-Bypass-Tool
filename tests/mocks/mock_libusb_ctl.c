/*
 * mock_libusb_ctl.c -- Staging + reset for the libusb mock.
 */

#include "mock_libusb_internal.h"

#include <stdlib.h>
#include <string.h>

mock_libusb_ctrl_stage_t g_libusb_ctrl_stages[MOCK_CTRL_SLOTS];
int       g_libusb_list_count = 1;
uint16_t  g_libusb_list_vid   = 0x05AC;
uint16_t  g_libusb_list_pid   = 0x1227;

void mock_libusb_reset(void)
{
    int i;
    for (i = 0; i < MOCK_CTRL_SLOTS; i++) {
        free(g_libusb_ctrl_stages[i].data);
        memset(&g_libusb_ctrl_stages[i], 0,
               sizeof(g_libusb_ctrl_stages[i]));
    }
    g_libusb_list_count = 1;
    g_libusb_list_vid   = 0x05AC;
    g_libusb_list_pid   = 0x1227;
}

void mock_libusb_stage_control_transfer(int return_code,
                                        const void *data, size_t data_len)
{
    int i;
    for (i = 0; i < MOCK_CTRL_SLOTS; i++) {
        if (!g_libusb_ctrl_stages[i].in_use) {
            mock_libusb_ctrl_stage_t *s = &g_libusb_ctrl_stages[i];
            s->return_code = return_code;
            s->data        = mock_memdup(data, data_len);
            s->data_len    = data_len;
            s->in_use      = 1;
            s->consumed    = 0;
            return;
        }
    }
}

void mock_libusb_set_device_list(int count, uint16_t vid, uint16_t pid)
{
    g_libusb_list_count = count;
    g_libusb_list_vid   = vid;
    g_libusb_list_pid   = pid;
}
