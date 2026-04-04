#ifndef USB_HELPERS_H
#define USB_HELPERS_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>

/*
 * Perform a USB control transfer with a data stage.
 * Returns number of bytes transferred on success, negative libusb error code
 * on failure.
 */
int usb_ctrl_transfer(libusb_device_handle *dev,
                      uint8_t bmRequestType,
                      uint8_t bRequest,
                      uint16_t wValue,
                      uint16_t wIndex,
                      unsigned char *data,
                      uint16_t wLength,
                      unsigned int timeout);

/*
 * Perform a USB control transfer with no data stage (wLength=0).
 * Returns 0 on success, negative libusb error code on failure.
 */
int usb_ctrl_transfer_no_data(libusb_device_handle *dev,
                              uint8_t bmRequestType,
                              uint8_t bRequest,
                              uint16_t wValue,
                              uint16_t wIndex,
                              unsigned int timeout);

/*
 * Print a human-readable description of a libusb error code to stderr.
 */
void usb_print_error(int libusb_error);

#endif /* USB_HELPERS_H */
