#include "power.h"

/* Suspend subsystem implementation.
 *
 * This component provides a simple interface to enter system suspend (sleep)
 * and to perform basic cleanup after resume (wake). It uses the kernel's
 * power_suspend() routine to trigger ACPI suspend. After a resume, power_init()
 * is called to re-initialize power management structures.
 */

int power_suspend_init(void) {
    // No special initialization needed for now.
    return 0;
}

int power_suspend_enter(void) {
    // Trigger the kernel suspend routine. This will halt the CPU and
    // resume execution after the hardware wakes the system.
    power_suspend();
    return 0;
}

int power_suspend_exit(void) {
    // Reinitialize power management after wake-up.
    power_init();
    return 0;
}
