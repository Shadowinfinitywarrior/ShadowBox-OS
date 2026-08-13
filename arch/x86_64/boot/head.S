//
// BOOT FLOW — POWER ON → BOOTLOADER → KERNEL ENTRY
//
// ┌──────────────────────────────────────────────────┐
// │ BOOTLOADER                                       │
// │ 1. Multiboot2 header (GRUB2/Limine detection)    │
// │ 2. Kernel ELF loaded by bootloader               │
// │ 3. Identity page tables + higher-half mapping    │
// │ 4. Switch to long mode (64-bit)                  │
// │ 5. Pass Multiboot2 info to kernel entry          │
// │ 6. Jump to _start → kernel_main                  │
// └──────────────────────────────────────────────────┘
//

.section .multiboot, "a"
.align 8
multiboot_header:
    .long 0xE85250D6
    .long 0
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    .align 8
    .word 1
    .word 0
    .long 20
    .long 6
    .long 3
    .long 8

    .align 8
    .word 5
    .word 0
    .long 20
    .long 1024
    .long 768
    .long 32

    .align 8
    .word 0
    .word 0
    .long 8
multiboot_header_end:

.align 8
multiboot1_header:
    .long 0x1BADB002
    .long 0
    .long -(0x1BADB002 + 0)

.section .boot.text, "ax"
.global _start
.code32
_start:
    // Step 5: Receive Multiboot2 info from bootloader
    mov $boot_stack_top, %esp
    push $0
    popf
    mov %eax, %ebp          // save magic
                            // %ebx = info_ptr (preserved by bootloader)

    // Step 3: Initial Page Tables — clear PML4
    mov $boot_pml4, %edi
    mov $0, %eax
    mov $4096, %ecx
    rep stosl

    // PML4[0] → PDP (identity map low 1GB)
    mov $boot_pdp, %eax
    or $0x03, %eax
    mov %eax, boot_pml4

    // PDP[0] → PD (identity map first 2MB)
    mov $boot_pd, %eax
    or $0x03, %eax
    mov %eax, boot_pdp

    // PD[0..511] → 0x83, 0x1000083, ... (2MB huge pages, present/write)
    // Identity-map + higher-half-map the full low 1GB so the Multiboot2
    // info structure and loaded modules are readable regardless of where
    // GRUB placed them (they routinely sit above the first 2MB).
    mov $boot_pd, %edi
    mov $0x83, %eax
    mov $512, %ecx
1:
    mov %eax, (%edi)
    add $0x200000, %eax
    add $8, %edi
    loop 1b

    // PML4[511] → PDP (higher-half recursive mapping)
    mov $boot_pdp, %eax
    or $0x03, %eax
    mov %eax, boot_pml4 + 511 * 8

    // PDP[510] → PD (higher-half kernel mapping)
    mov $boot_pd, %eax
    or $0x03, %eax
    mov %eax, boot_pdp + 510 * 8

    // Step 4: Enable Long Mode
    mov $boot_pml4, %eax
    mov %eax, %cr3

    mov %cr4, %eax
    or $0x20, %eax          // PAE
    mov %eax, %cr4

    mov $0xC0000080, %ecx   // EFER MSR
    rdmsr
    or $0x100, %eax         // LME = 1
    wrmsr

    mov %cr0, %eax
    or $0x80000000, %eax    // Paging enable
    mov %eax, %cr0

    lgdt gdt64_pointer

    ljmp $0x08, $long_mode_start

.code64
long_mode_start:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    // Adjust stack pointer to higher half
    mov $0xFFFFFFFF80000000, %rax
    add %rax, %rsp

    // Step 5-6: Pass Multiboot2 info to kernel entry
    mov %ebp, %edi          // magic
    mov %ebx, %esi          // info_ptr
    mov $kernel_main, %rax
    jmp *%rax

.section .boot.data, "aw"
.align 8
gdt64:
    .quad 0
    .quad 0x0020980000000000
    .quad 0x0000920000000000
gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64

.align 4096
boot_pdp:  .skip 4096
.global boot_pd
boot_pd:   .skip 4096

.section .boot.bss, "aw", @nobits
.align 4096
.global boot_pml4
boot_pml4:
    .skip 4096
boot_stack_bottom:
    .skip 16384
boot_stack_top:
