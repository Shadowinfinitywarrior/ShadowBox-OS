#include "gdt.h"
#include "kernel.h"
#include "kstring.h"

struct gdt_entry gdt[7];
struct gdt_ptr gdtp;
struct tss_entry tss;

static void gdt_set_gate(int32_t num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

static void gdt_set_tss(int32_t num, uint64_t base) {
    uint32_t limit = sizeof(struct tss_entry);
    gdt_set_gate(num, base, limit, 0x89, 0x00);
    // TSS is 16 bytes in x86_64, uses next slot too
    struct gdt_entry *tss_high = &gdt[num + 1];
    uint32_t base_upper = (base >> 32);
    *((uint32_t*)tss_high) = base_upper;
    *((uint32_t*)tss_high + 1) = 0;
}

void tss_set_stack(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}

void gdt_init(void) {
    gdtp.limit = (sizeof(struct gdt_entry) * 7) - 1;
    gdtp.base = (uint64_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20); // Kernel Code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0x00); // Kernel Data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xF2, 0x00); // User Data
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xFA, 0x20); // User Code

    memset(&tss, 0, sizeof(struct tss_entry));
    tss.iopb_offset = sizeof(struct tss_entry);
    gdt_set_tss(5, (uint64_t)&tss);             // TSS

    __asm__ volatile(
        "lgdt %0\n"
        "push $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "mov $0x28, %%ax\n" // 5 * 8
        "ltr %%ax\n"
        : : "m"(gdtp) : "rax", "memory"
    );

    printk(KERN_INFO "GDT and TSS initialized.\n");
}

static struct gdt_entry ap_gdt[7];
static struct tss_entry ap_tss;

void gdt_init_ap(int cpu_id) {
    (void)cpu_id;
    struct gdt_ptr ap_gdtp;
    ap_gdtp.limit = (sizeof(struct gdt_entry) * 7) - 1;
    ap_gdtp.base = (uint64_t)&ap_gdt;

    memset(&ap_gdt, 0, sizeof(ap_gdt));
    struct gdt_entry *g = ap_gdt;
    g[0] = (struct gdt_entry){0};
    g[1].limit_low = 0xFFFF; g[1].base_low = 0; g[1].base_mid = 0;
    g[1].access = 0x9A; g[1].granularity = 0x20; g[1].base_high = 0;
    g[2].limit_low = 0xFFFF; g[2].base_low = 0; g[2].base_mid = 0;
    g[2].access = 0x92; g[2].granularity = 0x00; g[2].base_high = 0;
    g[3].limit_low = 0xFFFF; g[3].base_low = 0; g[3].base_mid = 0;
    g[3].access = 0xF2; g[3].granularity = 0x00; g[3].base_high = 0;
    g[4].limit_low = 0xFFFF; g[4].base_low = 0; g[4].base_mid = 0;
    g[4].access = 0xFA; g[4].granularity = 0x20; g[4].base_high = 0;

    memset(&ap_tss, 0, sizeof(ap_tss));
    ap_tss.iopb_offset = sizeof(ap_tss);

    uint64_t tss_base = (uint64_t)&ap_tss;
    uint32_t limit = sizeof(ap_tss);
    g[5].limit_low = limit & 0xFFFF;
    g[5].base_low = tss_base & 0xFFFF;
    g[5].base_mid = (tss_base >> 16) & 0xFF;
    g[5].access = 0x89;
    g[5].granularity = 0x00;
    g[5].base_high = (tss_base >> 24) & 0xFF;
    *(uint32_t*)&g[6] = (tss_base >> 32);

    __asm__ volatile("lgdt %0" : : "m"(ap_gdtp));
    __asm__ volatile(
        "push $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "mov $0x28, %%ax\n"
        "ltr %%ax\n"
        : : : "rax", "memory"
    );
}
