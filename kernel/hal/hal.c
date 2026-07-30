#include "hal/hal.h"
#include "hal/cpu.h"
#include "hal/memory.h"
#include "hal/storage.h"
#include "hal/peripheral.h"
#include "kernel.h"

static hal_arch_t detected_arch = HAL_ARCH_X86_64;
static bool hal_initialized = false;

hal_status_t hal_init(void) {
    if (hal_initialized) return HAL_SUCCESS;

    printk(KERN_INFO "HAL: Initializing Hardware Abstraction Layer...\n");

    cpu_init();
    memory_init();
    peripheral_init();

    printk(KERN_INFO "HAL: x86_64 platform ready\n");
    hal_initialized = true;
    return HAL_SUCCESS;
}

hal_arch_t hal_get_arch(void) {
    return detected_arch;
}

void hal_shutdown(void) {
    printk(KERN_INFO "HAL: Shutting down hardware...\n");
    hal_initialized = false;
}
