#include "apic.h"
#include "kernel.h"
#include "vmm.h"
#include "kstring.h"
#include "io.h"

static volatile uint32_t* lapic_base = NULL;
static volatile uint32_t* ioapic_base = NULL;

void lapic_write(uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)((uint8_t*)lapic_base + reg) = val;
}

uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t*)((uint8_t*)lapic_base + reg);
}

void lapic_eoi(void) {
    lapic_write(LAPIC_EOI, 0);
}

void lapic_enable(void) {
    lapic_write(LAPIC_SVR, 0x1FF);
}

uint32_t ioapic_read(uint32_t reg) {
    volatile uint32_t* ioapic_idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* ioapic_data = (volatile uint32_t*)((uint8_t*)ioapic_base + 0x10);
    *ioapic_idx = reg;
    return *ioapic_data;
}

void ioapic_write(uint32_t reg, uint32_t val) {
    volatile uint32_t* ioapic_idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* ioapic_data = (volatile uint32_t*)((uint8_t*)ioapic_base + 0x10);
    *ioapic_idx = reg;
    *ioapic_data = val;
}

void ioapic_set_entry(uint8_t index, uint64_t data) {
    ioapic_write(0x10 + index * 2, (uint32_t)data);
    ioapic_write(0x10 + index * 2 + 1, (uint32_t)(data >> 32));
}

void ioapic_route_irq(uint8_t irq, uint8_t vector, uint32_t lapic_id) {
    uint64_t entry = vector;
    entry |= 0ull;          // fixed delivery mode, physical dest, edge triggered, active high
    entry |= (uint64_t)lapic_id << 56;
    ioapic_set_entry(irq, entry);
}

void apic_init(void) {
    printk(KERN_INFO "APIC: Initializing Local APIC...\n");
    lapic_base = (volatile uint32_t*)vmap_phys(LAPIC_DEFAULT_BASE, 0x1000);
    ioapic_base = (volatile uint32_t*)vmap_phys(IOAPIC_DEFAULT_BASE, 0x1000);
    
    lapic_write(LAPIC_TPR, 0);
    lapic_write(LAPIC_LVT_TIMER, 0x10000);
    lapic_write(LAPIC_LVT_LINT0, 0x700 | 0x20);
    lapic_write(LAPIC_LVT_LINT1, 0x10000);
    lapic_write(LAPIC_LVT_ERROR, 0xFE);
    
    ioapic_set_entry(0, 0x700 | 0x20);
    
    outb(0x21, 0xF8);
    outb(0xA1, 0xFF);
    
    lapic_enable();
    
    printk(KERN_INFO "APIC: Initialization complete (LINT0 + IOAPIC ExtINT).\n");
}

void apic_count_cpus(uint32_t *apic_ids, int max, int *count) {
    uint32_t lapic_id = lapic_read(LAPIC_ID) >> 24;
    if (*count < max) {
        apic_ids[0] = lapic_id;
        *count = 1;
    }

    extern uint32_t multiboot_info_ptr;
    if (!multiboot_info_ptr) return;

    uint8_t *mbi = (uint8_t *)((uint64_t)multiboot_info_ptr + 0xFFFFFFFF80000000);
    uint32_t total_size = *(uint32_t *)mbi;
    uint32_t off = 8;
    while (off < total_size) {
        struct { uint32_t type; uint32_t size; } *tag = (void *)(mbi + off);
        if (tag->type == 0) break;

        if (tag->type == 0x14 && *count < max) {
            struct {
                uint32_t type;
                uint32_t size;
                uint64_t rsdp_addr;
            } *acpi_old = (void *)tag;
            (void)acpi_old;
        }
        off += (tag->size + 7) & ~7;
    }

    uint8_t *rsdp = (uint8_t *)0xFFFFFFFF800E0000;
    int found = 0;
    for (uint32_t off = 0; off < 0x100; off += 16) {
        if (memcmp(rsdp + off, "RSD PTR ", 8) == 0) {
            found = 1;
            uint32_t rsdt_addr = *(uint32_t *)(rsdp + off + 16);
            uint32_t *rsdt = (uint32_t *)(rsdt_addr + 0xFFFFFFFF80000000);
            uint32_t rsdt_len = *(uint32_t *)((uint64_t)rsdt);
            int entries = (rsdt_len - 36) / 4;
            for (int i = 0; i < entries; i++) {
                uint32_t sdt_addr = rsdt[i + 36 / 4];
                uint8_t *sdt = (uint8_t *)(sdt_addr + 0xFFFFFFFF80000000);
                if (memcmp(sdt, "APIC", 4) == 0) {
                    uint32_t madt_len = *(uint32_t *)(sdt + 4);
                    uint32_t madt_off = 44;
                    while (madt_off < madt_len && *count < max) {
                        uint8_t entry_type = sdt[madt_off];
                        uint8_t entry_len = sdt[madt_off + 1];
                        if (entry_type == 0 && entry_len >= 8) {
                            uint32_t apic_id = sdt[madt_off + 3];
                            int already = 0;
                            for (int c = 0; c < *count; c++) {
                                if (apic_ids[c] == apic_id) { already = 1; break; }
                            }
                            if (!already) apic_ids[(*count)++] = apic_id;
                        }
                        madt_off += entry_len;
                    }
                }
            }
        }
    }
    (void)found;
    printk(KERN_INFO "APIC: Found %d CPU(s) via MADT\n", *count);
}
