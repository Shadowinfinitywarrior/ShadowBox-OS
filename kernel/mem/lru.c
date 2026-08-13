/*
 * Minimal stub implementation of an LRU cache for the kernel.
 * This file provides placeholder definitions so that the source tree
 * builds successfully. No actual caching logic is implemented.
 */

#include <stddef.h>

/* Opaque key/value types – the real kernel may define its own types.
 * Using void* keeps the stub generic and harmless.
 */

typedef struct lru_node {
    void *key;
    void *value;
    struct lru_node *prev;
    struct lru_node *next;
} lru_node_t;

/* Initialise the LRU subsystem. In the stub this does nothing. */
static inline void lru_init(void) {
    /* No-op */
}

/* Retrieve a value from the LRU cache by key.
 * Returns NULL in the stub implementation.
 */
static inline void *lru_get(void *key) {
    (void)key;  /* suppress unused warning */
    return NULL;
}

/* Insert or update a key/value pair in the LRU cache.
 * The stub simply discards the parameters.
 */
static inline void lru_put(void *key, void *value) {
    (void)key;
    (void)value;
    /* No-op */
}

/* Optional: clear the cache – stub does nothing. */
static inline void lru_clear(void) {
    /* No-op */
}
