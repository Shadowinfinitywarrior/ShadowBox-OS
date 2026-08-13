/* Simple wrappers for kernel code that may mistakenly call libc malloc/free.
   They forward to the kernel's own allocator functions. */

#include "malloc.h"
#include <stddef.h>

static void free(void *ptr) {
    kfree(ptr);
}

static void *malloc(size_t size) {
    return kmalloc(size);
}
