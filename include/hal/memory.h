#ifndef SHADOWBOX_HAL_MEMORY_H
#define SHADOWBOX_HAL_MEMORY_H

#include "types.h"
#include "hal/hal.h"

// ============================================================================
// Memory Types
// ============================================================================

typedef enum {
    MEMORY_TYPE_CONVENTIONAL,   // Conventional RAM
    MEMORY_TYPE_RESERVED,       // Reserved (firmware, etc.)
    MEMORY_TYPE_ACPI,            // ACPI tables
    MEMORY_TYPE_NVS,             // Non-volatile storage (NVRAM)
    MEMORY_TYPE_UNUSABLE,       // Unusable memory
    MEMORY_TYPE_DISABLED,        // Disabled memory
    MEMORY_TYPE_PERSISTENT,      // Persistent memory (pmem)
    MEMORY_TYPE_UNKNOWN
} memory_type_t;

// ============================================================================
// Memory Region Structure
// ============================================================================

typedef struct {
    uint64_t base;              // Base physical address
    uint64_t size;              // Size in bytes
    memory_type_t type;         // Memory type
    uint32_t attributes;        // Memory attributes (see below)
} memory_region_t;

// Memory region attributes
#define MEMORY_ATTR_READABLE      (1 << 0)
#define MEMORY_ATTR_WRITABLE      (1 << 1)
#define MEMORY_ATTR_EXECUTABLE    (1 << 2)
#define MEMORY_ATTR_CACHEABLE     (1 << 3)
#define MEMORY_ATTR_UNCACHEABLE   (1 << 4)
#define MEMORY_ATTR_WRITE_COMBINE (1 << 5)
#define MEMORY_ATTR_WRITE_THROUGH (1 << 6)
#define MEMORY_ATTR_WRITE_BACK    (1 << 7)
#define MEMORY_ATTR_PREFETCHABLE  (1 << 8)

// ============================================================================
// DRAM Type
// ============================================================================

typedef enum {
    DRAM_TYPE_SDR,
    DRAM_TYPE_DDR,
    DRAM_TYPE_DDR2,
    DRAM_TYPE_DDR3,
    DRAM_TYPE_DDR4,
    DRAM_TYPE_DDR5,
    DRAM_TYPE_LPDDR,
    DRAM_TYPE_LPDDR2,
    DRAM_TYPE_LPDDR3,
    DRAM_TYPE_LPDDR4,
    DRAM_TYPE_LPDDR5,
    DRAM_TYPE_HBM,
    DRAM_TYPE_HBM2,
    DRAM_TYPE_HBM2E,
    DRAM_TYPE_HBM3,
    DRAM_TYPE_UNKNOWN
} dram_type_t;

// ============================================================================
// DRAM Information
// ============================================================================

typedef struct {
    dram_type_t type;           // DRAM type
    uint64_t total_size;        // Total DRAM size in bytes
    uint64_t usable_size;       // Usable DRAM size in bytes
    uint32_t channels;          // Number of memory channels
    uint32_t ranks;             // Number of ranks per channel
    uint32_t banks;             // Number of banks per rank
    uint32_t bank_groups;       // Number of bank groups
    uint32_t rows;              // Number of rows per bank
    uint32_t columns;           // Number of columns per row
    uint32_t data_width;        // Data width in bits
    uint32_t address_width;     // Address width in bits
    uint32_t speed;             // Speed in MT/s
    uint32_t voltage;           // Voltage in millivolts
    uint32_t ecc_support;       // ECC support (0 = none, 1 = SECDED, etc.)
} dram_info_t;

// ============================================================================
// Memory Map Entry (from firmware)
// ============================================================================

typedef struct {
    uint64_t base_addr;         // Base address
    uint64_t length;            // Length in bytes
    uint32_t type;              // Type (from firmware)
    uint32_t extended_attrs;    // Extended attributes
} memory_map_entry_t;

// ============================================================================
// Memory Management Statistics
// ============================================================================

typedef struct {
    uint64_t total_physical;    // Total physical memory
    uint64_t available_physical; // Available physical memory
    uint64_t used_physical;     // Used physical memory
    uint64_t total_virtual;     // Total virtual memory space
    uint64_t used_virtual;      // Used virtual memory
    uint64_t free_physical;     // Free physical memory
    uint64_t reserved_physical; // Reserved physical memory
    uint64_t page_faults;       // Number of page faults
    uint64_t page_allocations;  // Number of page allocations
    uint64_t page_frees;        // Number of page frees
} memory_stats_t;

// ============================================================================
// Memory Abstraction Functions
// ============================================================================

/**
 * @brief Initialize memory abstraction layer
 * @return hal_status_t Status of initialization
 */
hal_status_t memory_init(void);

/**
 * @brief Get memory map from firmware
 * @param map Pointer to array of memory_map_entry_t
 * @param max_entries Maximum number of entries to return
 * @return int Number of entries returned
 */
int memory_get_map(memory_map_entry_t *map, int max_entries);

/**
 * @brief Get DRAM information
 * @param info Pointer to dram_info_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t memory_get_dram_info(dram_info_t *info);

/**
 * @brief Get total physical memory size
 * @return uint64_t Total physical memory in bytes
 */
uint64_t memory_get_total_size(void);

/**
 * @brief Get available physical memory size
 * @return uint64_t Available physical memory in bytes
 */
uint64_t memory_get_available_size(void);

/**
 * @brief Get memory statistics
 * @param stats Pointer to memory_stats_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t memory_get_stats(memory_stats_t *stats);

/**
 * @brief Allocate physical memory
 * @param size Size in bytes
 * @param align Alignment in bytes (must be power of 2)
 * @return void* Physical address of allocated memory (NULL on failure)
 */
void* memory_alloc_physical(uint64_t size, uint64_t align);

/**
 * @brief Free physical memory
 * @param addr Physical address to free
 * @param size Size in bytes
 */
void memory_free_physical(void *addr, uint64_t size);

/**
 * @brief Map physical memory to virtual address
 * @param phys Physical address
 * @param size Size in bytes
 * @param flags Mapping flags (readable, writable, executable, etc.)
 * @return void* Virtual address of mapped memory (NULL on failure)
 */
void* memory_map_physical(uint64_t phys, uint64_t size, uint32_t flags);

/**
 * @brief Unmap virtual address
 * @param virt Virtual address to unmap
 * @param size Size in bytes
 */
void memory_unmap_physical(void *virt, uint64_t size);

/**
 * @brief Read from physical memory
 * @param phys Physical address
 * @param buf Buffer to read into
 * @param size Size in bytes
 * @return hal_status_t Status of operation
 */
hal_status_t memory_read_physical(uint64_t phys, void *buf, uint64_t size);

/**
 * @brief Write to physical memory
 * @param phys Physical address
 * @param buf Buffer to write from
 * @param size Size in bytes
 * @return hal_status_t Status of operation
 */
hal_status_t memory_write_physical(uint64_t phys, const void *buf, uint64_t size);

/**
 * @brief Copy memory (possibly overlapping)
 * @param dest Destination address
 * @param src Source address
 * @param size Size in bytes
 */
void memory_copy(void *dest, const void *src, uint64_t size);

/**
 * @brief Set memory to a value
 * @param dest Destination address
 * @param value Value to set (byte)
 * @param size Size in bytes
 */
void memory_set(void *dest, uint8_t value, uint64_t size);

/**
 * @brief Zero memory
 * @param dest Destination address
 * @param size Size in bytes
 */
void memory_zero(void *dest, uint64_t size);

/**
 * @brief Compare memory regions
 * @param a First memory region
 * @param b Second memory region
 * @param size Size in bytes
 * @return int 0 if equal, <0 if a < b, >0 if a > b
 */
int memory_compare(const void *a, const void *b, uint64_t size);

/**
 * @brief Check if memory region is valid (readable/writable)
 * @param addr Address to check
 * @param size Size in bytes
 * @return bool true if valid
 */
bool memory_is_valid(void *addr, uint64_t size);

/**
 * @brief Set memory attributes (cacheable, etc.)
 * @param addr Address
 * @param size Size in bytes
 * @param attributes Attributes to set (see MEMORY_ATTR_*)
 * @return hal_status_t Status of operation
 */
hal_status_t memory_set_attributes(void *addr, uint64_t size, uint32_t attributes);

/**
 * @brief Flush memory caches
 */
void memory_flush_caches(void);

/**
 * @brief Flush memory range from caches
 * @param addr Address
 * @param size Size in bytes
 */
void memory_flush_range(void *addr, uint64_t size);

/**
 * @brief Invalidate memory range in caches
 * @param addr Address
 * @param size Size in bytes
 */
void memory_invalidate_range(void *addr, uint64_t size);

// ============================================================================
// Memory Barrier Functions
// ============================================================================

/**
 * @brief Memory barrier (prevent reordering)
 */
void memory_barrier(void);

/**
 * @brief Read memory barrier
 */
void memory_read_barrier(void);

/**
 * @brief Write memory barrier
 */
void memory_write_barrier(void);

/**
 * @brief Full memory barrier (read + write)
 */
void memory_full_barrier(void);

// ============================================================================
// DMA Memory Functions
// ============================================================================

/**
 * @brief Allocate DMA-coherent memory
 * @param size Size in bytes
 * @param align Alignment in bytes
 * @return void* Virtual address of allocated memory (NULL on failure)
 */
void* memory_alloc_dma(uint64_t size, uint64_t align);

/**
 * @brief Free DMA-coherent memory
 * @param addr Virtual address to free
 * @param size Size in bytes
 */
void memory_free_dma(void *addr, uint64_t size);

/**
 * @brief Get physical address of DMA memory
 * @param virt Virtual address
 * @return uint64_t Physical address
 */
uint64_t memory_dma_to_physical(void *virt);

/**
 * @brief Flush DMA buffer
 * @param virt Virtual address
 * @param size Size in bytes
 * @param direction DMA direction (0 = to device, 1 = from device)
 */
void memory_flush_dma(void *virt, uint64_t size, int direction);

// ============================================================================
// NUMA Support
// ============================================================================

/**
 * @brief Get number of NUMA nodes
 * @return uint32_t Number of NUMA nodes
 */
uint32_t memory_get_numa_nodes(void);

/**
 * @brief Get NUMA node for a physical address
 * @param addr Physical address
 * @return int NUMA node ID (-1 on error)
 */
int memory_get_numa_node(uint64_t addr);

/**
 * @brief Allocate memory on a specific NUMA node
 * @param size Size in bytes
 * @param node NUMA node ID
 * @return void* Allocated memory (NULL on failure)
 */
void* memory_alloc_on_node(uint64_t size, int node);

#endif // SHADOWBOX_HAL_MEMORY_H
