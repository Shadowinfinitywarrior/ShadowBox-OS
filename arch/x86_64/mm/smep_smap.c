#include "aslr.h"
#include "kernel.h"

/*
 * Simple wrapper to initialize SMEP and SMAP protection.
 * Uses existing aslr_enable_smep() and aslr_enable_smap() which already
 * perform feature detection based on CONFIG_SMEP / CONFIG_SMAP.
 */
void smep_smap_init(void) {
    // Enable SMEP if supported.
    aslr_enable_smep();
    // Enable SMAP if supported.
    aslr_enable_smap();
}
