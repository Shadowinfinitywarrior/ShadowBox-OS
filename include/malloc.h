#ifndef SHADOWBOX_MALLOC_H
#define SHADOWBOX_MALLOC_H

#include "types.h"

/*
 * malloc_init - Initialize kernel heap allocator
 */
void malloc_init(void);

/*
 * kmalloc - Allocate kernel memory
 * @size: Number of bytes to allocate
 * Returns: Pointer to allocated memory, or NULL on failure
 */
void *kmalloc(size_t size);

/*
 * kfree - Free kernel memory
 * @ptr: Pointer to memory to free
 */
void kfree(void *ptr);

#endif
