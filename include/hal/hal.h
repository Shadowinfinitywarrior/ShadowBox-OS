#ifndef SHADOWBOX_HAL_H
#define SHADOWBOX_HAL_H

#include "types.h"

/**
 * @defgroup HAL Hardware Abstraction Layer
 * @brief Unified hardware abstraction for CPU, Memory, Storage, and Peripherals
 * 
 * The HAL provides a consistent interface for hardware access across different
 * architectures (x86_64, ARM64) and hardware types.
 */

// ============================================================================
// HAL Status Codes
// ============================================================================

typedef enum {
    HAL_SUCCESS = 0,
    HAL_ERROR_UNSUPPORTED,
    HAL_ERROR_NOT_FOUND,
    HAL_ERROR_NO_MEMORY,
    HAL_ERROR_TIMEOUT,
    HAL_ERROR_IO,
    HAL_ERROR_INIT_FAILED,
    HAL_ERROR_UNAVAILABLE
} hal_status_t;

// ============================================================================
// HAL Architecture Types
// ============================================================================

typedef enum {
    HAL_ARCH_X86_64,
    HAL_ARCH_ARM64,
    HAL_ARCH_UNKNOWN
} hal_arch_t;

// ============================================================================
// HAL Initialization
// ============================================================================

/**
 * @brief Initialize the Hardware Abstraction Layer
 * @return hal_status_t Status of initialization
 */
hal_status_t hal_init(void);

/**
 * @brief Get the current architecture
 * @return hal_arch_t The architecture type
 */
hal_arch_t hal_get_arch(void);

/**
 * @brief Shutdown the HAL
 */
void hal_shutdown(void);

// ============================================================================
// CPU Abstraction
// ============================================================================

#include "hal/cpu.h"

// ============================================================================
// Memory Abstraction
// ============================================================================

#include "hal/memory.h"

// ============================================================================
// Storage Abstraction
// ============================================================================

#include "hal/storage.h"

// ============================================================================
// Peripheral Abstraction
// ============================================================================

#include "hal/peripheral.h"

#endif // SHADOWBOX_HAL_H
