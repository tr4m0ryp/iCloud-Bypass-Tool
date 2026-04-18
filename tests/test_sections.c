/*
 * test_sections.c -- Unit tests for logic that does not require USB/lockdownd hardware.
 *
 * Sections:
 *   1. DFU serial CPID/ECID parsing
 *   2. Chip database lookups
 *   3. Bypass module probe logic (via mock modules)
 *   4. Exploit retry constants
 *
 * Build: make test
 * Run:   ./tests/run_tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "device/chip_db.h"
#include "bypass/bypass.h"
#include "device/device.h"
#include "exploit/checkm8_internal.h"

/* ------------------------------------------------------------------ */
/* Minimal test framework                                              */
/* ------------------------------------------------------------------ */

static int g_passes   = 0;
static int g_failures = 0;

#define ASSERT(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__, __LINE__, #expr); \
        g_failures++; \
    } else { \
        g_passes++; \
    } \
} while (0)

#define ASSERT_EQ(a, b)  ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))
#define ASSERT_NULL(p)   ASSERT((p) == NULL)
#define ASSERT_NOTNULL(p) ASSERT((p) != NULL)

/* ------------------------------------------------------------------ */
/* Section 1 -- DFU serial CPID/ECID parsing                          */
/* ------------------------------------------------------------------ */

/*
 * Inline copy of parse_hex_field logic from usb_dfu.c.
 * Tested here independently of the hardware-dependent USB code.
 */
static int test_parse_hex_field(const char *serial, const char *key,
                                uint64_t *out)
{
    const char *p;
    char *endptr;

    if (!serial || !key || !out)
        return -1;
    p = strstr(serial, key);
    if (!p)
        return -1;
    endptr = NULL;
    *out = strtoull(p + strlen(key), &endptr, 16);
    if (!endptr || endptr == p + strlen(key)) {
        *out = 0;
    }
    return 0;
}

static void test_parse_cpid_normal(void)
{
    const char *serial = "CPID:8015 CPRV:11 BDID:0C ECID:001A2B3C4D5E6F70 "
                         "IBFL:3C SRTG:[iBoot-4142.0.0.0.1]";
    uint64_t cpid = 0, ecid = 0;

    ASSERT_EQ(test_parse_hex_field(serial, "CPID:", &cpid), 0);
    ASSERT_EQ(cpid, (uint64_t)0x8015);

    ASSERT_EQ(test_parse_hex_field(serial, "ECID:", &ecid), 0);
    ASSERT_EQ(ecid, (uint64_t)0x001A2B3C4D5E6F70ULL);
}

static void test_parse_cpid_a9_samsung(void)
{
    const char *serial = "CPID:8003 CPRV:10 BDID:02 ECID:AABB112233445566";
    uint64_t cpid = 0;

    ASSERT_EQ(test_parse_hex_field(serial, "CPID:", &cpid), 0);
    ASSERT_EQ(cpid, (uint64_t)0x8003);
}

static void test_parse_cpid_empty_serial(void)
{
    /* Bare DFU string -- no CPID/ECID fields */
    const char *serial = "Apple Mobile Device (DFU Mode)";
    uint64_t cpid = 0xDEAD, ecid = 0xDEAD;

    ASSERT_EQ(test_parse_hex_field(serial, "CPID:", &cpid), -1);
    ASSERT_EQ(test_parse_hex_field(serial, "ECID:", &ecid), -1);
    /* Outputs unchanged on failure */
    ASSERT_EQ(cpid, (uint64_t)0xDEAD);
    ASSERT_EQ(ecid, (uint64_t)0xDEAD);
}

static void test_parse_cpid_partial(void)
{
    /* CPID present, ECID absent */
    const char *serial = "CPID:8960 CPRV:01";
    uint64_t cpid = 0, ecid = 0xBEEF;

    ASSERT_EQ(test_parse_hex_field(serial, "CPID:", &cpid), 0);
    ASSERT_EQ(cpid, (uint64_t)0x8960);
    ASSERT_EQ(test_parse_hex_field(serial, "ECID:", &ecid), -1);
    ASSERT_EQ(ecid, (uint64_t)0xBEEF);  /* unchanged */
}

static void run_serial_parse_tests(void)
{
    printf("--- Section 1: DFU serial parsing ---\n");
    test_parse_cpid_normal();
    test_parse_cpid_a9_samsung();
    test_parse_cpid_empty_serial();
    test_parse_cpid_partial();
}

/* ------------------------------------------------------------------ */
/* Section 2 -- Chip database                                         */
/* ------------------------------------------------------------------ */

static void test_chip_db_a9_tsmc(void)
{
    const chip_info_t *c = chip_db_lookup(0x8000);
    ASSERT_NOTNULL(c);
    ASSERT_EQ(c->checkm8_vulnerable, 1);
    ASSERT(strstr(c->name, "A9") != NULL);
}

static void test_chip_db_a9_samsung(void)
{
    const chip_info_t *c = chip_db_lookup(0x8003);
    ASSERT_NOTNULL(c);
    ASSERT_EQ(c->checkm8_vulnerable, 1);
}

static void test_chip_db_a11(void)
{
    const chip_info_t *c = chip_db_lookup(0x8015);
    ASSERT_NOTNULL(c);
    ASSERT_EQ(c->checkm8_vulnerable, 1);
    ASSERT(strstr(c->name, "A11") != NULL);
}

static void test_chip_db_a12(void)
{
    const chip_info_t *c = chip_db_lookup(0x8020);
    ASSERT_NOTNULL(c);
    ASSERT_EQ(c->checkm8_vulnerable, 0);
}

static void test_chip_db_a14(void)
{
    const chip_info_t *c = chip_db_lookup(0x8101);
    ASSERT_NOTNULL(c);
    ASSERT_EQ(c->checkm8_vulnerable, 0);
}

static void test_chip_db_unknown(void)
{
    ASSERT_NULL(chip_db_lookup(0xDEAD));
}

static void test_chip_db_zero(void)
{
    ASSERT_NULL(chip_db_lookup(0x0000));
}

static void run_chip_db_tests(void)
{
    printf("--- Section 2: Chip database ---\n");
    test_chip_db_a9_tsmc();
    test_chip_db_a9_samsung();
    test_chip_db_a11();
    test_chip_db_a12();
    test_chip_db_a14();
    test_chip_db_unknown();
    test_chip_db_zero();
}

/* ------------------------------------------------------------------ */
/* Section 3 -- Bypass module probe logic                             */
/*                                                                    */
/* Tests the probe selection logic directly via mock modules that     */
/* replicate the path_a/path_b probe conditions without requiring     */
/* any hardware linkage.                                              */
/* ------------------------------------------------------------------ */

/*
 * Mock probe: matches checkm8-vulnerable devices in DFU mode (Path A logic).
 */
static int mock_probe_path_a(device_info_t *dev)
{
    if (!dev) return -1;
    if (dev->checkm8_vulnerable != 1) return 0;
    if (dev->is_dfu_mode != 1) return 0;
    return 1;
}

/*
 * Mock probe: matches A12+ (non-checkm8) devices in DFU mode (Path B logic).
 */
static int mock_probe_path_b(device_info_t *dev)
{
    if (!dev) return -1;
    if (dev->checkm8_vulnerable != 0) return 0;
    if (dev->is_dfu_mode != 1) return 0;
    return 1;
}

static int mock_execute_noop(device_info_t *dev) { (void)dev; return 0; }

static const bypass_module_t mock_path_a = {
    .name        = "mock_path_a",
    .description = "Mock Path A for testing",
    .probe       = mock_probe_path_a,
    .execute     = mock_execute_noop
};

static const bypass_module_t mock_path_b = {
    .name        = "mock_path_b",
    .description = "Mock Path B for testing",
    .probe       = mock_probe_path_b,
    .execute     = mock_execute_noop
};

static device_info_t make_dev(int checkm8_vuln, int is_dfu)
{
    device_info_t dev;
    memset(&dev, 0, sizeof(dev));
    dev.checkm8_vulnerable = checkm8_vuln;
    dev.is_dfu_mode        = is_dfu;
    return dev;
}

static void test_probe_path_a_compatible(void)
{
    device_info_t dev = make_dev(1, 1);
    ASSERT_EQ(mock_probe_path_a(&dev), 1);
}

static void test_probe_path_a_not_dfu(void)
{
    device_info_t dev = make_dev(1, 0);
    ASSERT_EQ(mock_probe_path_a(&dev), 0);
}

static void test_probe_path_a_a12(void)
{
    device_info_t dev = make_dev(0, 1);
    ASSERT_EQ(mock_probe_path_a(&dev), 0);
}

static void test_probe_path_b_compatible(void)
{
    device_info_t dev = make_dev(0, 1);
    ASSERT_EQ(mock_probe_path_b(&dev), 1);
}

static void test_probe_path_b_a11(void)
{
    device_info_t dev = make_dev(1, 1);
    ASSERT_EQ(mock_probe_path_b(&dev), 0);
}

static void test_probe_path_b_not_dfu(void)
{
    device_info_t dev = make_dev(0, 0);
    ASSERT_EQ(mock_probe_path_b(&dev), 0);
}

static void test_probe_select_prefers_a(void)
{
    /* Both modules registered; checkm8-vulnerable device -> Path A selected */
    bypass_register(&mock_path_a);
    bypass_register(&mock_path_b);

    device_info_t dev = make_dev(1, 1);
    const bypass_module_t *sel = bypass_select(&dev);
    ASSERT_NOTNULL(sel);
    ASSERT_EQ(strcmp(sel->name, "mock_path_a"), 0);
}

static void test_probe_select_falls_to_b(void)
{
    /* A12+ device -> Path A probe returns 0, Path B selected */
    device_info_t dev = make_dev(0, 1);
    const bypass_module_t *sel = bypass_select(&dev);
    ASSERT_NOTNULL(sel);
    ASSERT_EQ(strcmp(sel->name, "mock_path_b"), 0);
}

static void test_probe_select_no_match(void)
{
    /* Normal mode A12+ device -> neither probe matches */
    device_info_t dev = make_dev(0, 0);
    ASSERT_NULL(bypass_select(&dev));
}

static void run_bypass_probe_tests(void)
{
    printf("--- Section 3: Bypass probe logic ---\n");
    test_probe_path_a_compatible();
    test_probe_path_a_not_dfu();
    test_probe_path_a_a12();
    test_probe_path_b_compatible();
    test_probe_path_b_a11();
    test_probe_path_b_not_dfu();
    /* These two share a module registry -- must run after the individual tests */
    test_probe_select_prefers_a();
    test_probe_select_falls_to_b();
    test_probe_select_no_match();
}

/* ------------------------------------------------------------------ */
/* Section 4 -- Exploit retry constants                               */
/* ------------------------------------------------------------------ */

static void test_retry_constant(void)
{
    ASSERT_EQ(MAX_EXPLOIT_TRIES, 3);
}

static void test_reconnect_delay(void)
{
    ASSERT_EQ(USB_RECONNECT_DELAY_USEC, 2000000);
}

static void run_constant_tests(void)
{
    printf("--- Section 4: Exploit constants ---\n");
    test_retry_constant();
    test_reconnect_delay();
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    printf("tr4mpass unit tests\n");
    printf("===================\n");

    run_serial_parse_tests();
    run_chip_db_tests();
    run_bypass_probe_tests();
    run_constant_tests();

    printf("===================\n");
    printf("Results: %d passed, %d failed\n", g_passes, g_failures);

    return g_failures > 0 ? 1 : 0;
}
