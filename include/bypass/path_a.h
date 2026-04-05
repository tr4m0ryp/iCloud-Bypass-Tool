/*
 * path_a.h -- Path A bypass module for A5-A11 devices.
 *
 * Chains: checkm8 BootROM exploit -> ramdisk -> jailbreak ->
 * activation record submission -> deletescript cleanup -> verify.
 */

#ifndef PATH_A_H
#define PATH_A_H

#include "bypass/bypass.h"

/* The Path A bypass module for registration with bypass_register(). */
extern const bypass_module_t path_a_module;

#endif /* PATH_A_H */
