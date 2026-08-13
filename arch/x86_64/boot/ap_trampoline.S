.section .trampoline, "ax"
.code16
.global ap_trampoline_start
ap_trampoline_start:
    cli
    cld

    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    lgdtl (ap_trampoline_gdt_desc - ap_trampoline_start + 0x1000)

    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    ljmp $0x08, $(ap_trampoline_pm - ap_trampoline_start + 0x1000)

.code32
ap_trampoline_pm:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    mov $0x18, %ax
    mov %ax, %fs
    mov %ax, %gs

    mov $0xA000, %esp

    mov (ap_trampoline_pml4 - ap_trampoline_start + 0x1000), %eax
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

    ljmp $0x08, $(ap_trampoline_lm - ap_trampoline_start + 0x1000)

.code64
ap_trampoline_lm:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    mov (ap_trampoline_stack - ap_trampoline_start + 0x1000), %rsp
    mov (ap_trampoline_entry - ap_trampoline_start + 0x1000), %rax
    jmp *%rax

.global ap_trampoline_end
ap_trampoline_end:

.section .boot.data
.global ap_trampoline_pml4
.global ap_trampoline_stack
.global ap_trampoline_entry
.global ap_trampoline_gdt
.global ap_trampoline_gdt_desc

.align 8
ap_trampoline_pml4:     .quad 0
ap_trampoline_stack:    .quad 0
ap_trampoline_entry:    .quad 0
ap_trampoline_gdt:      .quad 0
ap_trampoline_gdt_desc: .quad 0
