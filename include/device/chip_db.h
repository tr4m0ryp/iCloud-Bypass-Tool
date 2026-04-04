#ifndef CHIP_DB_H
#define CHIP_DB_H

#include <stdint.h>

/*
 * chip_info_t -- static descriptor for a known Apple SoC variant.
 *
 * heap_base    : DFU SRAM heap base (ttbr0_addr from gaster); 0 if not
 *                vulnerable.
 * payload_base : Insecure memory / shellcode load address
 *                (insecure_memory_base from gaster); 0 if not vulnerable.
 */
typedef struct {
    uint32_t    cpid;
    const char *name;               /* "A7", "A8", ..., "A17"            */
    const char *marketing;          /* "iPhone 5s", "iPhone X", etc.     */
    int         checkm8_vulnerable; /* 1 = A5-A11, 0 = A12+             */
    uint64_t    heap_base;          /* DFU heap base   (0 if not vuln)   */
    uint64_t    payload_base;       /* Shellcode addr  (0 if not vuln)   */
} chip_info_t;

/*
 * chip_db_lookup -- return chip_info_t for the given CPID, or NULL if
 *                   the CPID is not in the table.
 */
const chip_info_t *chip_db_lookup(uint32_t cpid);

/*
 * chip_db_name -- return the short chip name ("A10", "A11", ...) for the
 *                 given CPID, or "Unknown" if not found.
 */
const char *chip_db_name(uint32_t cpid);

/*
 * chip_db_is_checkm8_vulnerable -- return 1 if the CPID is checkm8-
 *                                  vulnerable (A5-A11), 0 otherwise.
 */
int chip_db_is_checkm8_vulnerable(uint32_t cpid);

/*
 * chip_db_print_all -- print the full chip table to stdout (debug only).
 */
void chip_db_print_all(void);

#endif /* CHIP_DB_H */
