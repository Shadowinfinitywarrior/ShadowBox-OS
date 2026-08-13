// Minimal RNG stub for kernel

#include "kernel.h"

/* Simple xorshift64* implementation */
static uint64_t rng_state = 0;

/* Initialize RNG – deterministic seed based on address */
void rng_init(void) {
    /* Use the address of this function as a simple seed */
    rng_state = (uint64_t)(uintptr_t)rng_init;
    /* Mix the seed a bit */
    rng_state ^= rng_state << 21;
    rng_state ^= rng_state >> 35;
    rng_state ^= rng_state << 4;
    printk(KERN_INFO "rng: initialized\n");
}

/* Return a pseudo‑random 64‑bit value */
uint64_t rng_get(void) {
    /* xorshift64* algorithm */
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}
