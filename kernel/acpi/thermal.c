// SPDX-License-Identifier: MIT
// Minimal stub for ACPI thermal management in the kernel

#include <stddef.h>
#include "acpi.h"

/* Placeholder structure for thermal zone */
struct acpi_thermal_zone {
    int temperature; // placeholder temperature value
};

/* Initialize thermal management. Returns 0 on success. */
int acpi_thermal_init(void) {
    // Stub implementation – no real initialization performed
    return 0;
}

/* Retrieve temperature for a given thermal zone.
 * On success returns 0 and writes the temperature to *temp.
 */
int acpi_thermal_get_temp(struct acpi_thermal_zone *zone, int *temp) {
    if (!zone || !temp)
        return -1;
    *temp = zone->temperature;
    return 0;
}

/* Cleanup thermal management resources. */
void acpi_thermal_cleanup(void) {
    // Stub implementation – nothing to clean up
}
