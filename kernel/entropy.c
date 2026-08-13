#include "kernel.h"
#include "types.h"

/* Simple stub implementation of an entropy subsystem.
   This provides minimal functions so that the file compiles
   and can be linked into the kernel build. No real randomness
   is generated – the functions return deterministic data.
*/

static int entropy_initialized = 0;

/* Initialize the entropy subsystem (stub). */
void entropy_init(void) {
    printk(KERN_INFO "entropy subsystem initialized (stub)\n");
    entropy_initialized = 1;
}

/* Fill a buffer with deterministic pseudo‑random data.
   Returns 0 on success, -1 on error (e.g., null buffer).
*/
int entropy_get(void *buf, size_t len) {
    if (!entropy_initialized) {
        entropy_init();
    }
    if (!buf) {
        return -1;
    }
    unsigned char *p = (unsigned char *)buf;
    for (size_t i = 0; i < len; ++i) {
        p[i] = (unsigned char)(i & 0xFF);
    }
    return 0;
}

/* Return a 32‑bit deterministic value. */
uint32_t entropy_get_u32(void) {
    uint32_t v = 0;
    entropy_get(&v, sizeof(v));
    return v;
}

/* Return a 64‑bit deterministic value. */
uint64_t entropy_get_u64(void) {
    uint64_t v = 0;
    entropy_get(&v, sizeof(v));
    return v;
}
