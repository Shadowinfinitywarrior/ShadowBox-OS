#ifndef SHADOWBOX_ACPI_H
#define SHADOWBOX_ACPI_H

#include "types.h"

/*
 * acpi_rsdp_t - Root System Description Pointer
 * @signature: "RSD PTR " string
 * @checksum: Checksum for verification
 * @oem_id: OEM identifier
 * @revision: ACPI revision number
 * @rsdt_address: Physical address of RSDT
 */
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

/*
 * acpi_init - Initialize ACPI subsystem
 * Parses RSDP and RSDT/XSDT tables
 */
void acpi_init(void);

#endif
