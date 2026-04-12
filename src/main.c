/* main.c -- tr4mpass entry point and orchestration. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#include "tr4mpass.h"
#include "device/device.h"
#include "device/usb_dfu.h"
#include "device/chip_db.h"
#include "bypass/bypass.h"
#include "bypass/path_a.h"
#include "bypass/path_b.h"
#include "util/log.h"

typedef struct {
    int verbose;
    int dry_run;
    int detect_only;
    int force_path_a;
    int force_path_b;
    uint32_t cpid_override;
    uint64_t ecid_override;
    int has_cpid_override;
    int has_ecid_override;
} cli_opts_t;

static void print_banner(void)
{
    printf("========================================\n"
           "  tr4mpass v%s\n"
           "  Activation lock bypass research tool\n"
           "========================================\n\n", TR4MPASS_VERSION);
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [OPTIONS]\n\n"
           "Options:\n"
           "  -v, --verbose       Enable debug logging\n"
           "  -n, --dry-run       Show what would run, do not execute\n"
           "  -d, --detect-only   Detect device and exit\n"
           "  -a, --force-path-a  Force Path A (checkm8, A5-A11)\n"
           "  -b, --force-path-b  Force Path B (identity, A12+)\n"
           "      --cpid <hex>    Override CPID (e.g. --cpid 0x8960)\n"
           "      --ecid <hex>    Override ECID (e.g. --ecid 0x1F70CB1331C)\n"
           "  -h, --help          Show this help message\n", prog);
}

static int parse_args(int argc, char *argv[], cli_opts_t *opts)
{
    enum { OPT_CPID = 256, OPT_ECID = 257 };

    static const struct option longopts[] = {
        { "verbose",      no_argument,       NULL, 'v' },
        { "dry-run",      no_argument,       NULL, 'n' },
        { "detect-only",  no_argument,       NULL, 'd' },
        { "force-path-a", no_argument,       NULL, 'a' },
        { "force-path-b", no_argument,       NULL, 'b' },
        { "cpid",         required_argument, NULL, OPT_CPID },
        { "ecid",         required_argument, NULL, OPT_ECID },
        { "help",         no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    memset(opts, 0, sizeof(*opts));

    int c;
    while ((c = getopt_long(argc, argv, "vndabh", longopts, NULL)) != -1) {
        switch (c) {
        case 'v': opts->verbose      = 1; break;
        case 'n': opts->dry_run      = 1; break;
        case 'd': opts->detect_only  = 1; break;
        case 'a': opts->force_path_a = 1; break;
        case 'b': opts->force_path_b = 1; break;
        case OPT_CPID:
            opts->cpid_override     = (uint32_t)strtoul(optarg, NULL, 16);
            opts->has_cpid_override = 1;
            break;
        case OPT_ECID:
            opts->ecid_override     = (uint64_t)strtoull(optarg, NULL, 16);
            opts->has_ecid_override = 1;
            break;
        case 'h': print_usage(argv[0]); return 1;
        default:  print_usage(argv[0]); return -1;
        }
    }

    if (opts->force_path_a && opts->force_path_b) {
        log_error("Cannot force both Path A and Path B simultaneously");
        return -1;
    }

    return 0;
}

static int detect_device(device_info_t *dev)
{
    libusb_device_handle *usb_handle = NULL;
    memset(dev, 0, sizeof(*dev));

    if (usb_dfu_find(&usb_handle) == 0) {
        log_info("Device found in DFU mode");
        uint32_t cpid = 0;
        uint64_t ecid = 0;
        char serial[DFU_SERIAL_MAX] = {0};
        if (usb_dfu_read_info(usb_handle, &cpid, &ecid,
                              serial, sizeof(serial)) < 0)
            log_warn("Failed to read DFU serial info");
        dev->cpid        = cpid;
        dev->ecid        = ecid;
        dev->is_dfu_mode = 1;
        dev->usb         = usb_handle;
        snprintf(dev->serial, sizeof(dev->serial), "%s", serial);
        return 0;
    }

    log_info("No DFU device found, trying normal mode...");
    if (device_detect(dev) < 0) {
        log_error("No device detected. Is it connected via USB?");
        return -1;
    }
    if (device_query_info(dev) < 0)
        log_warn("Incomplete device info (partial data may be available)");
    return 0;
}

static void enrich_chip_info(device_info_t *dev)
{
    if (dev->cpid == 0) {
        log_warn("CPID not available, chip lookup skipped");
        return;
    }
    const chip_info_t *chip = chip_db_lookup(dev->cpid);
    if (!chip) {
        log_warn("CPID 0x%04X not found in chip database", dev->cpid);
        snprintf(dev->chip_name, sizeof(dev->chip_name), "Unknown");
        dev->checkm8_vulnerable = 0;
        return;
    }
    snprintf(dev->chip_name, sizeof(dev->chip_name), "%s", chip->name);
    dev->checkm8_vulnerable = chip->checkm8_vulnerable;
    log_debug("Chip: %s (CPID 0x%04X) -- checkm8 %s",
              chip->name, chip->cpid,
              chip->checkm8_vulnerable ? "vulnerable" : "not vulnerable");
}

static void register_modules(void)
{
    if (bypass_register(&path_a_module) < 0)
        log_warn("Failed to register Path A module");
    if (bypass_register(&path_b_module) < 0)
        log_warn("Failed to register Path B module");
    log_debug("Registered %d bypass module(s)", bypass_count());
}

static void print_module_diagnostics(const device_info_t *dev)
{
    log_error("No compatible bypass module for this device");
    log_error("  DFU=%s CPID=0x%04X Chip=%s checkm8=%s Product=%s",
              dev->is_dfu_mode ? "YES" : "NO", dev->cpid,
              dev->chip_name[0] ? dev->chip_name : "(unknown)",
              dev->checkm8_vulnerable ? "YES" : "NO",
              dev->product_type[0] ? dev->product_type : "(unknown)");

    if (!dev->is_dfu_mode)
        log_error("Both paths require DFU mode. Use ./start.sh or enter DFU manually.");
    else if (dev->cpid == 0)
        log_error("CPID is 0x0000 -- try --cpid to set manually.");
    else if (dev->chip_name[0] == '\0' || strcmp(dev->chip_name, "Unknown") == 0)
        log_error("Chip CPID 0x%04X not in database.", dev->cpid);
}

static const bypass_module_t *select_module(device_info_t *dev,
                                            const cli_opts_t *opts)
{
    if (opts->force_path_a) { log_info("Path A forced"); return &path_a_module; }
    if (opts->force_path_b) { log_info("Path B forced"); return &path_b_module; }
    const bypass_module_t *mod = bypass_select(dev);
    if (!mod) { print_module_diagnostics(dev); return NULL; }
    log_info("Auto-selected module: %s", mod->name);
    return mod;
}

static void cleanup(device_info_t *dev)
{
    if (dev->usb) {
        usb_dfu_close(dev->usb);
        dev->usb = NULL;
    }
    device_free(dev);
    usb_dfu_cleanup();
}

int main(int argc, char *argv[])
{
    cli_opts_t opts;
    device_info_t dev;
    int ret;

    print_banner();

    ret = parse_args(argc, argv, &opts);
    if (ret != 0)
        return (ret > 0) ? 0 : 1;
    if (opts.verbose)
        log_set_level(LOG_DEBUG);

    if (usb_dfu_init() < 0) {
        log_error("Failed to initialize USB subsystem");
        return 1;
    }
    if (detect_device(&dev) < 0) {
        usb_dfu_cleanup();
        return 1;
    }

    /* Apply CLI overrides for CPID/ECID */
    if (opts.has_cpid_override) {
        if (dev.cpid == 0) {
            dev.cpid = opts.cpid_override;
            log_info("CPID overridden: 0x%04X (from --cpid)", dev.cpid);
        } else {
            log_info("CPID already detected (0x%04X), ignoring --cpid override",
                     dev.cpid);
        }
    }
    if (opts.has_ecid_override) {
        if (dev.ecid == 0) {
            dev.ecid = opts.ecid_override;
            log_info("ECID overridden: 0x%llX (from --ecid)",
                     (unsigned long long)dev.ecid);
        } else {
            log_info("ECID already detected (0x%llX), ignoring --ecid override",
                     (unsigned long long)dev.ecid);
        }
    }

    enrich_chip_info(&dev);
    device_print_info(&dev);

    if (opts.detect_only) {
        log_info("Detect-only mode, exiting");
        cleanup(&dev);
        return 0;
    }

    register_modules();
    bypass_list();

    const bypass_module_t *mod = select_module(&dev, &opts);
    if (!mod) { cleanup(&dev); return 1; }

    if (opts.dry_run) {
        printf("[dry-run] Would execute: %s\n", mod->name);
        printf("[dry-run] Description:   %s\n", mod->description);
        cleanup(&dev);
        return 0;
    }

    log_info("Executing module: %s", mod->name);
    ret = bypass_execute(mod, &dev);
    if (ret == 0)
        printf("\n[+] Bypass completed successfully.\n");
    else
        printf("\n[-] Bypass failed (error %d).\n", ret);

    cleanup(&dev);
    return (ret == 0) ? 0 : 1;
}
