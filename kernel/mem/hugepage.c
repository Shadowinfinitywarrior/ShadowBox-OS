// Minimal stub for hugepage support in the kernel.
// This file provides placeholder types and functions so that the
// source tree builds without requiring actual hugepage implementation.

#include <stddef.h>

/* Simple placeholder structure representing a huge page. */
struct hugepage {
    unsigned long addr;   /* Physical base address */
    size_t size;          /* Size in bytes */
};

/* Initialize hugepage subsystem. Returns 0 on success. */
int hugepage_init(void)
{
    return 0;
}

/* Allocate a hugepage of at least `size` bytes. Returns NULL in stub. */
void *hugepage_alloc(size_t size)
{
    (void)size; /* suppress unused warning */
    return NULL;
}

/* Free a previously allocated hugepage. No-op in stub. */
void hugepage_free(void *ptr)
{
    (void)ptr; /* suppress unused warning */
}

/* Exported symbols (if needed). */
/* The real kernel may reference these via extern declarations. */

