#include "kernel.h"
#include "vfs.h"
#include "block.h"
#include "malloc.h"
#include "kstring.h"

// Stub implementation for FAT32 filesystem driver.
// This file provides the minimal symbols required for successful compilation.
// No actual FAT32 functionality is implemented.

// Global root node for the FAT32 filesystem (stub).
vfs_node_t *fat32_root = NULL;

// Initialize the FAT32 driver (stub).
void fat32_init(void) {
    // Allocate a root node placeholder if needed.
    // Actual initialization is omitted.
    (void)fat32_root;
}

// Mount a device as FAT32 (stub).
// Returns 0 on success, negative error code on failure.
int fat32_mount(void) {
    // (void)dev; // removed stub
    return 0; // success stub
}

// Read from a FAT32 file (stub).
static uint32_t fat32_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return 0; // indicate EOF / no data
}

// Write to a FAT32 file (stub).
static uint32_t fat32_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    (void)node; (void)offset; (void)size; (void)buffer;
    return size; // pretend all bytes were written
}

// Additional stub functions could be added here as needed for linking.
