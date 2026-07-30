.section .boot.text, "ax"
.code32
.global kaslr_relocate
kaslr_relocate:
    push %ebp
    mov %esp, %ebp

    mov 8(%ebp), %eax
    mov 12(%ebp), %ecx
    mov 16(%ebp), %edx

    mov $_kernel_start, %esi
    mov $_kernel_end, %edi
    sub %esi, %edi
    add %eax, %edx

    mov %ecx, %esi
    mov %eax, %edi
    mov %edx, %ecx

    cld
    rep movsb

    pop %ebp
    ret
