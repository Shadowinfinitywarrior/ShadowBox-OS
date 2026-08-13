#include "pit.h"
#include "io.h"
#include "kernel.h"
#include "pic.h"
#include "apic.h"

void pit_handler(void) {
    extern void syscall_tick(void);
    syscall_tick();
    
    extern void schedule(void);
    schedule();
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193182 / frequency;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    pic_clear_mask(0);
    ioapic_route_irq(0, 32, 0);
    printk(KERN_INFO "PIT initialized at %d Hz.\n", frequency);
}
