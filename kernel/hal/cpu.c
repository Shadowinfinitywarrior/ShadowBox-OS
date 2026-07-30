#include "hal/cpu.h"
#include "kernel.h"
#include "io.h"
#include "apic.h"
#include "smp.h"

extern int cpu_count;

static cpu_info_t cpu_info;
static bool cpu_initialized = false;

static void cpuid(uint32_t func, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    __asm__ volatile("cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(func), "c"(0));
}

hal_status_t cpu_init(void) {
    if (cpu_initialized) return HAL_SUCCESS;

    uint32_t a, b, c, d;
    cpuid(0, &a, &b, &c, &d);
    cpu_info.cpuid_max = a;

    char vendor[13];
    ((uint32_t*)vendor)[0] = b;
    ((uint32_t*)vendor)[1] = d;
    ((uint32_t*)vendor)[2] = c;
    vendor[12] = 0;

    if (vendor[0] == 'G' && vendor[1] == 'e' && vendor[2] == 'n') {
        cpu_info.vendor = CPU_VENDOR_INTEL;
    } else if (vendor[0] == 'A' && vendor[1] == 'u' && vendor[2] == 't') {
        cpu_info.vendor = CPU_VENDOR_AMD;
    } else {
        cpu_info.vendor = CPU_VENDOR_UNKNOWN;
    }

    cpu_info.arch = CPU_ARCH_X86;

    if (cpu_info.cpuid_max >= 1) {
        cpuid(1, &a, &b, &c, &d);
        cpu_info.family = (a >> 8) & 0xF;
        cpu_info.model = (a >> 4) & 0xF;
        cpu_info.stepping = a & 0xF;

        cpu_info.features.sse = (d >> 25) & 1;
        cpu_info.features.sse2 = (d >> 26) & 1;
        cpu_info.features.vtx = (c >> 5) & 1;
        cpu_info.features.aes = (c >> 25) & 1;
        cpu_info.features.rdrand = (c >> 30) & 1;
    }

    if (cpu_info.cpuid_max >= 4) {
        cpuid(4, &a, &b, &c, &d);
        cpu_info.topology.cores_per_package = ((a >> 26) & 0x3F) + 1;
        cpu_info.topology.logical_cpus = cpu_count > 0 ? cpu_count : 1;
        cpu_info.topology.cores = cpu_info.topology.cores_per_package;
        cpu_info.topology.threads_per_core = 1;
        cpu_info.topology.packages = 1;
    }

    if (cpu_info.cpuid_max >= 0x80000002) {
        uint32_t *brand = (uint32_t*)cpu_info.brand_string;
        cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
        cpu_info.brand_string[48] = 0;
    }

    cpu_initialized = true;
    printk(KERN_INFO "CPU: %s (%d cores, %d logical)\n",
           cpu_info.brand_string[0] ? cpu_info.brand_string : cpu_get_vendor_string(),
           cpu_info.topology.cores, cpu_info.topology.logical_cpus);
    return HAL_SUCCESS;
}

hal_status_t cpu_get_info(cpu_info_t *info) {
    if (!cpu_initialized) return HAL_ERROR_INIT_FAILED;
    if (!info) return HAL_ERROR_IO;
    *info = cpu_info;
    return HAL_SUCCESS;
}

uint32_t cpu_get_id(void) {
    uint32_t id = 0;
    __asm__ volatile("mov %%gs:0, %0" : "=r"(id) : : "memory");
    return id;
}

const char* cpu_get_vendor_string(void) {
    switch (cpu_info.vendor) {
        case CPU_VENDOR_INTEL: return "Intel";
        case CPU_VENDOR_AMD:   return "AMD";
        case CPU_VENDOR_ARM:   return "ARM";
        default:               return "Unknown";
    }
}

bool cpu_has_feature(const char *feature) {
    (void)feature;
    return false;
}

uint64_t cpu_read_tsc(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void cpu_pause(void) {
    __asm__ volatile("pause");
}

void cpu_halt(void) {
    __asm__ volatile("hlt");
}

void cpu_enable_interrupts(void) {
    __asm__ volatile("sti");
}

void cpu_disable_interrupts(void) {
    __asm__ volatile("cli");
}

bool cpu_interrupts_enabled(void) {
    uint64_t flags;
    __asm__ volatile("pushf; pop %0" : "=r"(flags));
    return (flags & 0x200) != 0;
}

void cpu_save_registers(cpu_registers_t *regs) {
    (void)regs;
}

void cpu_restore_registers(cpu_registers_t *regs) {
    (void)regs;
}

void cpu_flush_tlb(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0; mov %0, %%cr3" : "=r"(cr3) :: "memory");
}

void cpu_invalidate_tlb(uint64_t addr) {
    __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

#ifdef __x86_64__
uint64_t cpu_read_msr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

void cpu_write_msr(uint32_t msr, uint64_t value) {
    __asm__ volatile("wrmsr" :: "a"((uint32_t)value), "d"((uint32_t)(value >> 32)), "c"(msr));
}

void cpu_cpuid(uint32_t function, uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(function), "c"(leaf));
}

uint64_t cpu_get_cr0(void) {
    uint64_t val;
    __asm__ volatile("mov %%cr0, %0" : "=r"(val));
    return val;
}

void cpu_set_cr0(uint64_t value) {
    __asm__ volatile("mov %0, %%cr0" :: "r"(value) : "memory");
}

uint64_t cpu_get_cr3(void) {
    uint64_t val;
    __asm__ volatile("mov %%cr3, %0" : "=r"(val));
    return val;
}

void cpu_set_cr3(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(value) : "memory");
}

uint64_t cpu_get_cr4(void) {
    uint64_t val;
    __asm__ volatile("mov %%cr4, %0" : "=r"(val));
    return val;
}

void cpu_set_cr4(uint64_t value) {
    __asm__ volatile("mov %0, %%cr4" :: "r"(value) : "memory");
}
#endif
