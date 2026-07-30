#ifndef SHADOWBOX_PIC_H
#define SHADOWBOX_PIC_H

#include "types.h"

/*
 * pic_init - Initialize the 8259 PIC and remap IRQs
 */
void pic_init(void);

/*
 * pic_send_eoi - Send end-of-interrupt signal
 * @irq: IRQ number to acknowledge
 */
void pic_send_eoi(uint8_t irq);

/*
 * pic_clear_mask - Unmask an IRQ
 * @irq: IRQ number
 */
void pic_clear_mask(uint8_t irq);

/*
 * pic_set_mask - Mask an IRQ
 * @irq: IRQ number
 */
void pic_set_mask(uint8_t irq);

#endif
