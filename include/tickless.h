#ifndef SHADOWBOX_TICKLESS_H
#define SHADOWBOX_TICKLESS_H

/*
 * tickless.h - Tickless timer mode interface
 *
 * This header declares the initialization function for the tickless timer
 * subsystem.  The implementation resides in arch/x86_64/drivers/tickless.c.
 */

void tickless_init(void);

#endif // SHADOWBOX_TICKLESS_H
