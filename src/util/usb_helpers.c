#include <stdio.h>
#include "util/usb_helpers.h"

int usb_ctrl_transfer(libusb_device_handle *dev,
                      uint8_t bmRequestType,
                      uint8_t bRequest,
                      uint16_t wValue,
                      uint16_t wIndex,
                      unsigned char *data,
                      uint16_t wLength,
                      unsigned int timeout)
{
    int ret;

    if (!dev)
        return LIBUSB_ERROR_INVALID_PARAM;

    ret = libusb_control_transfer(dev, bmRequestType, bRequest,
                                  wValue, wIndex, data, wLength, timeout);
    return ret;
}

int usb_ctrl_transfer_no_data(libusb_device_handle *dev,
                              uint8_t bmRequestType,
                              uint8_t bRequest,
                              uint16_t wValue,
                              uint16_t wIndex,
                              unsigned int timeout)
{
    int ret;

    if (!dev)
        return LIBUSB_ERROR_INVALID_PARAM;

    ret = libusb_control_transfer(dev, bmRequestType, bRequest,
                                  wValue, wIndex, NULL, 0, timeout);
    /* libusb returns 0 for transfers with no data stage on success */
    return (ret < 0) ? ret : 0;
}

void usb_print_error(int libusb_error)
{
    fprintf(stderr, "[usb] error %d: %s\n",
            libusb_error, libusb_strerror((enum libusb_error)libusb_error));
}
