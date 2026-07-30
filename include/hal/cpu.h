#ifndef SHADOWBOX_HAL_CPU_H
#define SHADOWBOX_HAL_CPU_H

#include "types.h"
#include "hal/hal.h"

// ============================================================================
// CPU Vendor Types
// ============================================================================

typedef enum {
    CPU_VENDOR_INTEL,
    CPU_VENDOR_AMD,
    CPU_VENDOR_ARM,
    CPU_VENDOR_QUALCOMM,
    CPU_VENDOR_APPLE,
    CPU_VENDOR_UNKNOWN
} cpu_vendor_t;

// ============================================================================
// CPU Architecture Types
// ============================================================================

typedef enum {
    CPU_ARCH_X86,
    CPU_ARCH_ARM,
    CPU_ARCH_RISCV,
    CPU_ARCH_MIPS,
    CPU_ARCH_UNKNOWN
} cpu_arch_t;

// ============================================================================
// CPU Instruction Set Extensions
// ============================================================================

typedef struct {
    bool sse;        // Streaming SIMD Extensions
    bool sse2;       // SSE2
    bool sse3;       // SSE3
    bool ssse3;      // Supplemental SSE3
    bool sse4_1;     // SSE4.1
    bool sse4_2;     // SSE4.2
    bool avx;        // Advanced Vector Extensions
    bool avx2;       // AVX2
    bool avx512;     // AVX-512
    bool aes;        // AES-NI
    bool sha;        // SHA extensions
    bool rdrand;     // RDRAND
    bool rdseed;     // RDSEED
    bool fsgsbase;   // FSGSBASE
    bool bmi1;       // BMI1
    bool bmi2;       // BMI2
    bool adx;        // ADX
    bool mpx;        // MPX
    bool sgx;        // SGX
    bool vtx;        // Virtualization Technology
    bool vtx2;       // Virtualization Technology 2
} cpu_features_t;

// ============================================================================
// CPU Topology Information
// ============================================================================

typedef struct {
    uint32_t cores;          // Total number of physical cores
    uint32_t logical_cpus;   // Total number of logical CPUs (including HT/SMT)
    uint32_t threads_per_core; // Threads per core (SMT level)
    uint32_t packages;       // Number of CPU packages (sockets)
    uint32_t cores_per_package; // Cores per package
} cpu_topology_t;

// ============================================================================
// CPU Cache Information
// ============================================================================

typedef struct {
    uint32_t size;           // Cache size in bytes
    uint32_t line_size;      // Cache line size in bytes
    uint32_t associativity;  // Cache associativity
    uint32_t level;          // Cache level (1, 2, 3, etc.)
    bool unified;            // true if unified (data + instruction)
} cpu_cache_t;

// ============================================================================
// CPU Information Structure
// ============================================================================

typedef struct {
    cpu_vendor_t vendor;     // CPU vendor
    cpu_arch_t arch;         // CPU architecture
    uint32_t family;         // CPU family
    uint32_t model;          // CPU model
    uint32_t stepping;       // CPU stepping
    char brand_string[49];   // CPU brand string (null-terminated)
    uint32_t cpuid_max;      // Maximum CPUID function supported
    cpu_features_t features; // CPU feature flags
    cpu_topology_t topology; // CPU topology
    cpu_cache_t caches[8];   // Cache information (max 8 levels)
    uint32_t cache_count;    // Number of cache levels
    uint64_t base_frequency; // Base frequency in Hz
    uint64_t max_frequency;  // Maximum frequency in Hz
    uint64_t tsc_frequency;  // TSC frequency in Hz
} cpu_info_t;

// ============================================================================
// CPU Register State (for context switching)
// ============================================================================

#ifdef __x86_64__
typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cs;
    uint64_t ss;
    uint64_t ds;
    uint64_t es;
    uint64_t fs;
    uint64_t gs;
} cpu_registers_t;
#elif defined(__aarch64__)
typedef struct {
    uint64_t x[31];          // x0-x30
    uint64_t sp;             // Stack pointer
    uint64_t pc;             // Program counter
    uint64_t cpsr;           // Current Program Status Register
    uint64_t fp;             // Frame pointer (x29)
    uint64_t lr;             // Link register (x30)
} cpu_registers_t;
#else
#error "Unsupported architecture for CPU register state"
#endif

// ============================================================================
// CPU Abstraction Functions
// ============================================================================

/**
 * @brief Initialize CPU abstraction layer
 * @return hal_status_t Status of initialization
 */
hal_status_t cpu_init(void);

/**
 * @brief Get CPU information
 * @param info Pointer to cpu_info_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t cpu_get_info(cpu_info_t *info);

/**
 * @brief Get current CPU ID (logical CPU number)
 * @return uint32_t Current CPU ID
 */
uint32_t cpu_get_id(void);

/**
 * @brief Get CPU vendor string
 * @return const char* Vendor string (e.g., "Intel", "AMD", "ARM")
 */
const char* cpu_get_vendor_string(void);

/**
 * @brief Check if CPU has a specific feature
 * @param feature Feature to check (see cpu_features_t)
 * @return bool true if feature is available
 */
bool cpu_has_feature(const char *feature);

/**
 * @brief Get CPU topology information
 * @param topology Pointer to cpu_topology_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t cpu_get_topology(cpu_topology_t *topology);

/**
 * @brief Get CPU cache information
 * @param level Cache level (1, 2, 3, etc.)
 * @param cache Pointer to cpu_cache_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t cpu_get_cache_info(uint32_t level, cpu_cache_t *cache);

/**
 * @brief Get current CPU frequency
 * @return uint64_t Current frequency in Hz
 */
uint64_t cpu_get_frequency(void);

/**
 * @brief Get TSC frequency
 * @return uint64_t TSC frequency in Hz
 */
uint64_t cpu_get_tsc_frequency(void);

/**
 * @brief Read TSC (Time Stamp Counter)
 * @return uint64_t Current TSC value
 */
uint64_t cpu_read_tsc(void);

/**
 * @brief Pause CPU execution (for spinlocks, etc.)
 */
void cpu_pause(void);

/**
 * @brief Halt CPU execution
 */
void cpu_halt(void);

/**
 * @brief Enable interrupts
 */
void cpu_enable_interrupts(void);

/**
 * @brief Disable interrupts
 */
void cpu_disable_interrupts(void);

/**
 * @brief Get interrupt state
 * @return bool true if interrupts are enabled
 */
bool cpu_interrupts_enabled(void);

/**
 * @brief Save CPU register state
 * @param regs Pointer to cpu_registers_t structure to fill
 */
void cpu_save_registers(cpu_registers_t *regs);

/**
 * @brief Restore CPU register state
 * @param regs Pointer to cpu_registers_t structure
 */
void cpu_restore_registers(cpu_registers_t *regs);

/**
 * @brief CPU-specific initialization for secondary CPUs (SMP)
 * @param cpu_id Logical CPU ID
 */
void cpu_secondary_init(uint32_t cpu_id);

/**
 * @brief Send IPI (Inter-Processor Interrupt)
 * @param cpu_id Target CPU ID
 * @param vector Interrupt vector
 */
void cpu_send_ipi(uint32_t cpu_id, uint8_t vector);

/**
 * @brief Flush CPU caches
 */
void cpu_flush_caches(void);

/**
 * @brief Flush TLB
 */
void cpu_flush_tlb(void);

/**
 * @brief Invalidate TLB entry
 * @param addr Virtual address to invalidate
 */
void cpu_invalidate_tlb(uint64_t addr);

// ============================================================================
// CPU Architecture-Specific Functions
// ============================================================================

#ifdef __x86_64__

/**
 * @brief Read MSR (Model Specific Register)
 * @param msr MSR address
 * @return uint64_t MSR value
 */
uint64_t cpu_read_msr(uint32_t msr);

/**
 * @brief Write MSR (Model Specific Register)
 * @param msr MSR address
 * @param value Value to write
 */
void cpu_write_msr(uint32_t msr, uint64_t value);

/**
 * @brief Read CPUID
 * @param function CPUID function
 * @param leaf CPUID leaf
 * @param eax Pointer to EAX output
 * @param ebx Pointer to EBX output
 * @param ecx Pointer to ECX output
 * @param edx Pointer to EDX output
 */
void cpu_cpuid(uint32_t function, uint32_t leaf, 
               uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

/**
 * @brief Get current CR0 register
 * @return uint64_t CR0 value
 */
uint64_t cpu_get_cr0(void);

/**
 * @brief Set CR0 register
 * @param value Value to set
 */
void cpu_set_cr0(uint64_t value);

/**
 * @brief Get current CR3 register (page table base)
 * @return uint64_t CR3 value
 */
uint64_t cpu_get_cr3(void);

/**
 * @brief Set CR3 register
 * @param value Value to set
 */
void cpu_set_cr3(uint64_t value);

/**
 * @brief Get current CR4 register
 * @return uint64_t CR4 value
 */
uint64_t cpu_get_cr4(void);

/**
 * @brief Set CR4 register
 * @param value Value to set
 */
void cpu_set_cr4(uint64_t value);

/**
 * @brief Read GDT
 * @param entry Selector value
 * @return uint64_t GDT entry
 */
uint64_t cpu_read_gdt(uint16_t entry);

/**
 * @brief Read IDT
 * @param entry Interrupt vector
 * @return uint64_t IDT entry
 */
uint64_t cpu_read_idt(uint8_t entry);

#endif // __x86_64__

#ifdef __aarch64__

/**
 * @brief Read system register
 * @param reg Register name
 * @return uint64_t Register value
 */
uint64_t cpu_read_sysreg(const char *reg);

/**
 * @brief Write system register
 * @param reg Register name
 * @param value Value to write
 */
void cpu_write_sysreg(const char *reg, uint64_t value);

#endif // __aarch64__

#endif // SHADOWBOX_HAL_CPU_H
