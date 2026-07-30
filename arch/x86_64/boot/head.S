.section .multiboot, "a"
.align 8
multiboot_header:
    .long 0xE85250D6
    .long 0
    .long multiboot_header_end - multiboot_header
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    // Information request tag
    .align 8
    .word 1 // type = information request
    .word 0 // flags = 0
    .long 20 // size = 20
    .long 6 // MULTIBOOT_TAG_TYPE_MMAP
    .long 3 // MULTIBOOT_TAG_TYPE_MODULE
    .long 8 // MULTIBOOT_TAG_TYPE_FRAMEBUFFER

    // Framebuffer request tag
    .align 8
    .word 5 // type = framebuffer request
    .word 0 // flags = 0
    .long 20 // size = 20
    .long 1024 // width
    .long 768  // height
    .long 32   // depth

    // End tag
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
    mov $boot_stack_top, %esp
    push $0
    popf

    // Save multiboot magic and info ptr in %ebp, %ebx
    mov %eax, %ebp
    // %ebx already contains info_ptr, and it is preserved

    mov $boot_pml4, %edi
    mov $0, %eax
    mov $4096, %ecx
    rep stosl

    mov $boot_pdp, %eax
    or $0x03, %eax
    mov %eax, boot_pml4

    mov $boot_pd, %eax
    or $0x03, %eax
    mov %eax, boot_pdp

    mov $0x83, %eax
    mov %eax, boot_pd

    mov $boot_pdp, %eax
    or $0x03, %eax
    mov %eax, boot_pml4 + 511 * 8

    mov $boot_pd, %eax
    or $0x03, %eax
    mov %eax, boot_pdp + 510 * 8

    mov $boot_pml4, %eax
    mov %eax, %cr3

    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    mov %cr0, %eax
    or $0x80000000, %eax
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

    // Update RSP to point to the higher-half virtual address of the stack
    mov $0xFFFFFFFF80000000, %rax
    add %rax, %rsp

    mov %ebp, %edi
    mov %ebx, %esi

    mov $kernel_main, %rax
    jmp *%rax

.section .boot.data, "aw"
.align 8
gdt64:
    .quad 0
    .quad 0x0020980000000000 // Kernel code
    .quad 0x0000920000000000 // Kernel data
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
