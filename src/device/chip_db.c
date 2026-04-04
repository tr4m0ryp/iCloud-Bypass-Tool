#include "device/chip_db.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

/*
 * Memory address sources:
 *   A10 (0x8010), A11 (0x8015): from gaster (github.com/0x7ff/gaster)
 *     heap_base    = ttbr0_addr
 *     payload_base = insecure_memory_base
 *   A7-A9X: insecure_memory_base from gaster where available; placeholder
 *     values (marked PLACEHOLDER) for chips not detailed in gaster.
 *   A12+: not vulnerable, both fields are 0.
 */

/* clang-format off */
static const chip_info_t g_chip_table[] = {
    /*
     * A7 -- iPhone 5s, iPad Air, iPad mini 2/3
     * gaster: insecure_memory_base not listed for 0x8947 (32-bit, ARMv7s).
     * payload_dest_armv7 used instead; heap_base is PLACEHOLDER.
     */
    {
        0x8947,
        "A7",
        "iPhone 5s / iPad Air",
        1,
        0x34000000,  /* PLACEHOLDER: 32-bit DFU SRAM base               */
        0x34039800   /* payload_dest_armv7 from gaster (0x8947 variant)  */
    },

    /*
     * A8 -- iPhone 6/6 Plus, iPod touch 6G
     * gaster: insecure_memory_base = 0x180380000, ttbr0 not set (A8 uses
     * insecure_memory_base as the payload target directly).
     */
    {
        0x8960,
        "A8",
        "iPhone 6 / iPhone 6 Plus",
        1,
        0x180380000, /* insecure_memory_base doubles as heap base for A8  */
        0x180380000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A8 variant (iPad mini 4 / iPod touch 6G second silicon rev)
     * gaster: insecure_memory_base = 0x180380000 (same as 0x8960).
     */
    {
        0x7000,
        "A8",
        "iPod touch 6G / iPad mini 4",
        1,
        0x180380000,
        0x180380000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A8X -- iPad Air 2
     * gaster: insecure_memory_base = 0x180380000 (same family).
     */
    {
        0x7001,
        "A8X",
        "iPad Air 2",
        1,
        0x180380000,
        0x180380000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A9 -- iPhone 6s/6s Plus, iPhone SE (1st gen)
     * gaster: insecure_memory_base = 0x180380000.
     */
    {
        0x8000,
        "A9",
        "iPhone 6s / iPhone SE",
        1,
        0x180380000,
        0x180380000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A9 variant (Samsung-fab A9 used in some iPhone 6s/6s Plus units)
     * gaster: insecure_memory_base = 0x180380000.
     */
    {
        0x8003,
        "A9",
        "iPhone 6s (Samsung variant)",
        1,
        0x180380000,
        0x180380000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A9X -- iPad Pro 9.7-inch / 12.9-inch (1st gen)
     * gaster: ttbr0_addr = 0x180050000, no insecure_memory_base listed;
     * use ttbr0_addr as payload_base (PLACEHOLDER for payload_base).
     */
    {
        0x8001,
        "A9X",
        "iPad Pro (1st gen)",
        1,
        0x180050000, /* ttbr0_addr from gaster                            */
        0x180050000  /* PLACEHOLDER: insecure_memory_base not in gaster   */
    },

    /*
     * A10 Fusion -- iPhone 7/7 Plus, iPod touch 7G
     * gaster: ttbr0_addr = 0x1800A0000, insecure_memory_base = 0x1800B0000.
     */
    {
        0x8010,
        "A10",
        "iPhone 7 / iPhone 7 Plus",
        1,
        0x1800A0000, /* ttbr0_addr from gaster                            */
        0x1800B0000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A10X Fusion -- iPad Pro 10.5-inch / 12.9-inch (2nd gen), Apple TV 4K
     * gaster: ttbr0_addr = 0x1800A0000 (same as A10).
     */
    {
        0x8011,
        "A10X",
        "iPad Pro 10.5\" / Apple TV 4K",
        1,
        0x1800A0000, /* ttbr0_addr from gaster (same config as 0x8010)   */
        0x1800B0000  /* insecure_memory_base from gaster                  */
    },

    /*
     * A11 Bionic -- iPhone 8/8 Plus, iPhone X
     * gaster: ttbr0_addr = 0x18000C000, insecure_memory_base = 0x18001C000.
     */
    {
        0x8015,
        "A11",
        "iPhone 8 / iPhone 8 Plus / iPhone X",
        1,
        0x18000C000, /* ttbr0_addr from gaster                            */
        0x18001C000  /* insecure_memory_base from gaster                  */
    },

    /* ------------------------------------------------------------------ */
    /* A12 and later: NOT checkm8-vulnerable.                             */
    /* ------------------------------------------------------------------ */

    /* A12 Bionic -- iPhone XS/XS Max/XR */
    {
        0x8020,
        "A12",
        "iPhone XS / iPhone XR",
        0,
        0,
        0
    },

    /* A12Z Bionic -- iPad Pro (4th gen), Apple Silicon DTK */
    {
        0x8027,
        "A12Z",
        "iPad Pro (4th gen)",
        0,
        0,
        0
    },

    /* A13 Bionic -- iPhone 11 series */
    {
        0x8030,
        "A13",
        "iPhone 11 / iPhone 11 Pro",
        0,
        0,
        0
    },

    /* A14 Bionic -- iPhone 12 series, iPad Air (4th gen) */
    {
        0x8101,
        "A14",
        "iPhone 12 / iPad Air (4th gen)",
        0,
        0,
        0
    },

    /* M1 -- MacBook Air/Pro (early 2020), iPad Pro (5th gen) */
    {
        0x8103,
        "M1",
        "iPad Pro (5th gen) / MacBook (M1)",
        0,
        0,
        0
    },

    /* A15 Bionic -- iPhone 13 series, iPhone SE (3rd gen) */
    {
        0x8110,
        "A15",
        "iPhone 13 / iPhone SE (3rd gen)",
        0,
        0,
        0
    },

    /* M2 -- MacBook Air/Pro (2022), iPad Pro (6th gen) */
    {
        0x8112,
        "M2",
        "iPad Pro (6th gen) / MacBook (M2)",
        0,
        0,
        0
    },

    /* A16 Bionic -- iPhone 14 Pro/Pro Max */
    {
        0x8120,
        "A16",
        "iPhone 14 Pro / iPhone 14 Pro Max",
        0,
        0,
        0
    },

    /* A17 Pro -- iPhone 15 Pro/Pro Max */
    {
        0x8130,
        "A17",
        "iPhone 15 Pro / iPhone 15 Pro Max",
        0,
        0,
        0
    },

    /* Null-terminated sentinel */
    { 0, NULL, NULL, 0, 0, 0 }
};
/* clang-format on */

const chip_info_t *
chip_db_lookup(uint32_t cpid)
{
    const chip_info_t *p;

    for (p = g_chip_table; p->name != NULL; p++) {
        if (p->cpid == cpid)
            return p;
    }
    return NULL;
}

const char *
chip_db_name(uint32_t cpid)
{
    const chip_info_t *info = chip_db_lookup(cpid);
    return (info != NULL) ? info->name : "Unknown";
}

int
chip_db_is_checkm8_vulnerable(uint32_t cpid)
{
    const chip_info_t *info = chip_db_lookup(cpid);
    return (info != NULL) ? info->checkm8_vulnerable : 0;
}

void
chip_db_print_all(void)
{
    const chip_info_t *p;

    printf("%-8s  %-5s  %-38s  %s  %s  %s\n",
           "CPID", "Name", "Marketing", "Vuln", "HeapBase", "PayloadBase");
    printf("%-8s  %-5s  %-38s  %s  %s  %s\n",
           "--------", "-----", "--------------------------------------",
           "----", "----------", "-----------");

    for (p = g_chip_table; p->name != NULL; p++) {
        printf("0x%04X    %-5s  %-38s  %-4d  0x%09" PRIX64 "  0x%09" PRIX64 "\n",
               p->cpid,
               p->name,
               p->marketing,
               p->checkm8_vulnerable,
               p->heap_base,
               p->payload_base);
    }
}
