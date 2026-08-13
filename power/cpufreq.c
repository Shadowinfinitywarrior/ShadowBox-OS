/*
 * Minimal stub implementation for CPU frequency scaling.
 * This file provides placeholder functions that compile but contain no
 * actual hardware logic. It can be expanded later with real driver code.
 */

#include "power.h"
#include <stddef.h>

/* Initialize the CPU frequency scaling subsystem. */
int cpufreq_init(void)
{
    /* No real initialization performed. */
    return 0;
}

/* Set the CPU frequency to the requested value (in kHz).
 * Returns 0 on success, negative error code otherwise.
 */
int cpufreq_set_frequency(int freq_khz)
{
    (void)freq_khz; /* Suppress unused parameter warning */
    return 0;
}

/* Retrieve the current CPU frequency (in kHz).
 * The caller provides a pointer that will receive the frequency.
 */
int cpufreq_get_frequency(int *out_freq_khz)
{
    if (!out_freq_khz)
        return -1; /* Invalid argument */
    *out_freq_khz = 0; /* Stub value */
    return 0;
}

/* Cleanup any resources allocated by the CPU frequency subsystem. */
void cpufreq_cleanup(void)
{
    /* No cleanup required for stub implementation. */
}
