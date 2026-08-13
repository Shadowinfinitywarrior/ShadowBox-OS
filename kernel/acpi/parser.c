// SPDX-License-Identifier: MIT
// Minimal stub for ACPI parser in the kernel

#include <stddef.h>
#include "acpi.h" // Expected header for ACPI definitions

/* Placeholder structure representing an ACPI parser instance */
struct acpi_parser {
    void *data; // Opaque data pointer
};

/* Initialize the ACPI parser. Returns 0 on success. */
int acpi_parser_init(void) {
    // Stub implementation – no real initialization performed
    return 0;
}

/* Parse ACPI tables. Returns 0 on success. */
int acpi_parse(void) {
    // Stub implementation – parsing is not performed
    return 0;
}

/* Cleanup resources used by the ACPI parser. */
void acpi_parser_cleanup(void) {
    // Stub implementation – nothing to clean up
}
