#ifndef SHADOWBOX_PIT_H
#define SHADOWBOX_PIT_H

#include "types.h"

/*
 * pit_init - Initialize PIT with given frequency
 * @frequency: Desired timer interrupt frequency in Hz
 */
void pit_init(uint32_t frequency);

#endif
