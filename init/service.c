/*
 * init/service.c - Minimal stub implementation for the init service subsystem.
 * This provides placeholder definitions for service management functions
 * declared in include/init.h. The implementation is intentionally lightweight
 * and contains no business logic; it merely satisfies the compiler and
 * links correctly with the rest of the kernel.
 */

#include "init.h"

/*
 * Core init system functions.
 * These are called during early boot to start the init system.
 * In this stub version they perform no actions.
 */
void init_system_start(void) {
    /* No operation – placeholder */
}

void init_mount_filesystems(void) {
    /* No operation – placeholder */
}

void init_parse_boot_order(void) {
    /* No operation – placeholder */
}

/*
 * Service manager functions.
 * They are expected to start and stop service units. The stubs simply
 * return success (0) without performing any work.
 */
int service_manager_start_unit(service_unit_t *unit) {
    (void)unit; /* suppress unused parameter warning */
    return 0;   /* success */
}

int service_manager_stop_unit(service_unit_t *unit) {
    (void)unit; /* suppress unused parameter warning */
    return 0;   /* success */
}
