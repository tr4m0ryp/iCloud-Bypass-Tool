/*
 * path_a_jailbreak.c -- Ramdisk loading and jailbreak for A5-A11 devices.
 *
 * After checkm8 puts the device into pwned DFU, this module handles:
 *   1. Loading a ramdisk image via the pwned DFU interface
 *   2. Applying jailbreak kernel patches via ramdisk SSH
 *   3. Replacing mobileactivationd with a patched version
 *
 * For v1, function bodies contain logging and TODO markers. The actual
 * SSH commands depend on which ramdisk is used (palera1n vs jbinit).
 */

#include <string.h>

#include "device/device.h"
#include "util/log.h"

/* Expected ramdisk payload paths (relative to working directory) */
#define RAMDISK_ARCHIVE  "payloads/boot.tar.lzma"
#define RAMDISK_RAW      "payloads/boot.raw"

/* SSH tunnel port used by the ramdisk */
#define RAMDISK_SSH_PORT 2222

/* ------------------------------------------------------------------ */
/* Ramdisk loading                                                     */
/* ------------------------------------------------------------------ */

/*
 * path_a_load_ramdisk -- Send a ramdisk image to the device via pwned DFU.
 *
 * After checkm8, the BootROM accepts unsigned images. We send a patched
 * ramdisk that includes SSH and the jailbreak toolchain. The ramdisk
 * boots into a minimal environment where we can apply kernel patches
 * and modify the filesystem.
 *
 * Implementation plan:
 *   1. Read ramdisk payload from payloads/ directory
 *   2. Send iBSS (second-stage bootloader) via DFU
 *   3. Send iBEC (boot chain) via DFU
 *   4. Send ramdisk and devicetree
 *   5. Boot the ramdisk via irecovery
 *   6. Wait for SSH tunnel to come up
 *
 * Returns 0 on success, -1 on failure.
 */
int
path_a_load_ramdisk(device_info_t *dev)
{
    if (!dev) {
        log_error("path_a_jailbreak: load_ramdisk called with NULL device");
        return -1;
    }

    if (!dev->usb) {
        log_error("path_a_jailbreak: no USB handle -- not in pwned DFU?");
        return -1;
    }

    log_info("path_a_jailbreak: loading ramdisk for %s (CPID 0x%04X)",
             dev->product_type, dev->cpid);

    /*
     * TODO: Implement ramdisk loading sequence.
     *
     * The exact commands depend on the ramdisk provider:
     *
     * palera1n approach:
     *   - Send patched iBSS via usb_dfu_send_buffer()
     *   - Reconnect, send patched iBEC
     *   - irecv_send_command("bootx") or equivalent
     *   - Send ramdisk + devicetree
     *   - Wait for SSH on port 2222
     *
     * jbinit approach:
     *   - Send boot.tar.lzma via DFU
     *   - Ramdisk auto-extracts and boots jbinit
     *   - jbinit sets up SSH and mounts filesystems
     *
     * For now, log the expected flow and return success to allow
     * downstream integration testing.
     */
    log_info("path_a_jailbreak: [STUB] would send iBSS for CPID 0x%04X",
             dev->cpid);
    log_info("path_a_jailbreak: [STUB] would send iBEC and boot ramdisk");
    log_info("path_a_jailbreak: [STUB] would wait for SSH on port %d",
             RAMDISK_SSH_PORT);

    log_warn("path_a_jailbreak: ramdisk loading not yet implemented"
             " -- stub returning success");
    return 0;
}

/* ------------------------------------------------------------------ */
/* Jailbreak kernel patches                                            */
/* ------------------------------------------------------------------ */

/*
 * path_a_jailbreak -- Apply jailbreak kernel patches via ramdisk SSH.
 *
 * Once the ramdisk is booted and SSH is available, we connect and
 * apply kernel patches to disable code signing, sandbox restrictions,
 * and other security mechanisms needed for the activation bypass.
 *
 * Implementation plan:
 *   1. Connect to ramdisk SSH (localhost:2222, user root, alpine)
 *   2. Unlock NVRAM (allow writing activation-related keys)
 *   3. Mount /dev/disk0s1s1 (system partition) as read-write
 *   4. Patch dyld shared cache or amfi to disable code signing
 *   5. Verify patches applied successfully
 *
 * Returns 0 on success, -1 on failure.
 */
int
path_a_jailbreak(device_info_t *dev)
{
    if (!dev) {
        log_error("path_a_jailbreak: jailbreak called with NULL device");
        return -1;
    }

    log_info("path_a_jailbreak: applying kernel patches for %s (iOS %s)",
             dev->product_type, dev->ios_version);

    /*
     * TODO: Implement jailbreak kernel patches via SSH.
     *
     * palera1n approach:
     *   ssh root@localhost -p 2222:
     *     nvram auto-boot=false
     *     mount -uw /dev/disk0s1s1 /mnt1
     *     patch /mnt1/usr/lib/dyld (disable AMFI checks)
     *
     * jbinit approach:
     *   jbinit handles kernel patching automatically
     *   we just need to verify the patches took effect:
     *     cat /proc/version (should show patched kernel)
     *     test -f /jb/jbinit.log
     *
     * Common verification:
     *   nvram -p | grep auto-boot
     *   ls /mnt1/Applications/Setup.app (should exist pre-cleanup)
     */
    log_info("path_a_jailbreak: [STUB] would connect SSH to port %d",
             RAMDISK_SSH_PORT);
    log_info("path_a_jailbreak: [STUB] would unlock NVRAM");
    log_info("path_a_jailbreak: [STUB] would mount system partition rw");
    log_info("path_a_jailbreak: [STUB] would patch dyld/AMFI");

    log_warn("path_a_jailbreak: kernel patches not yet implemented"
             " -- stub returning success");
    return 0;
}

/* ------------------------------------------------------------------ */
/* mobileactivationd replacement                                       */
/* ------------------------------------------------------------------ */

/*
 * path_a_replace_mobileactivationd -- Replace mobileactivationd with a
 * patched version that accepts locally-built activation records.
 *
 * The stock mobileactivationd validates activation records against
 * Apple's servers. The patched version skips server validation and
 * accepts records signed with our local keys.
 *
 * Implementation plan:
 *   1. SSH to ramdisk
 *   2. Back up original: cp /mnt1/usr/libexec/mobileactivationd{,.bak}
 *   3. Copy patched binary: scp patched_mad /mnt1/usr/libexec/mobileactivationd
 *   4. Set permissions: chmod 755, chown root:wheel
 *   5. Clear mobileactivationd cache
 *   6. Verify replacement: md5sum comparison
 *
 * Returns 0 on success, -1 on failure.
 */
int
path_a_replace_mobileactivationd(device_info_t *dev)
{
    if (!dev) {
        log_error("path_a_jailbreak: replace_mad called with NULL device");
        return -1;
    }

    log_info("path_a_jailbreak: replacing mobileactivationd on %s",
             dev->product_type);

    /*
     * TODO: Implement mobileactivationd replacement via SSH.
     *
     * Steps:
     *   ssh root@localhost -p 2222:
     *     cp /mnt1/usr/libexec/mobileactivationd \
     *        /mnt1/usr/libexec/mobileactivationd.bak
     *     cat > /mnt1/usr/libexec/mobileactivationd < patched_binary
     *     chmod 755 /mnt1/usr/libexec/mobileactivationd
     *     chown root:wheel /mnt1/usr/libexec/mobileactivationd
     *
     * The patched binary source depends on iOS version:
     *   iOS 12-14: bypass via lockdownd activation path
     *   iOS 15-16: bypass via direct plist injection
     *   iOS 17+:   bypass via session-mode shim
     */
    log_info("path_a_jailbreak: [STUB] would backup original"
             " mobileactivationd");
    log_info("path_a_jailbreak: [STUB] would copy patched binary");
    log_info("path_a_jailbreak: [STUB] would set permissions"
             " (755, root:wheel)");

    log_warn("path_a_jailbreak: mobileactivationd replacement"
             " not yet implemented -- stub returning success");
    return 0;
}
