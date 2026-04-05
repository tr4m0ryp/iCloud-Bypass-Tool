/*
 * path_a_internal.h -- Internal declarations shared between path_a.c
 * and path_a_jailbreak.c.
 *
 * Not part of the public API. Include only within the bypass module.
 */

#ifndef BYPASS_PATH_A_INTERNAL_H
#define BYPASS_PATH_A_INTERNAL_H

#include "device/device.h"

int path_a_load_ramdisk(device_info_t *dev);
int path_a_jailbreak(device_info_t *dev);
int path_a_replace_mobileactivationd(device_info_t *dev);

#endif /* BYPASS_PATH_A_INTERNAL_H */
