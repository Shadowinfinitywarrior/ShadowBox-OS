.global fork_child_entry
fork_child_entry:
    // After switch_to restored ctx (callee-saved regs + MSRs) and ret'd here,
    // RSP points to the fork_child register save area (pushed by task_fork).
    // Pop callee-saved regs first, then syscall regs, then user_rsp last.
    // RAX=0 is set manually (fork returns 0 in child). Then sysretq.

    pop %rbp             // user RBP
    pop %rbx             // user RBX
    pop %r12             // user R12
    pop %r13             // user R13
    pop %r14             // user R14
    pop %r15             // user R15

    pop %rcx             // user RIP (for sysretq)
    pop %r11             // user RFLAGS (for sysretq)
    pop %r10             // user R10
    pop %r9              // user R9
    pop %r8              // user R8
    pop %rdi             // user RDI
    pop %rsi             // user RSI
    pop %rdx             // user RDX

    pop %rsp             // user RSP (restore user stack)

    xor %eax, %eax       // RAX = 0 (fork return value for child)
    swapgs               // switch to user GS
    sysretq

.global switch_to

// void switch_to(struct context **old, struct context *new);
// RDI = old (struct context **), RSI = new (struct context *)
switch_to:
    // Save IA32_GS_BASE (MSR 0xC0000101)
    movl $0xC0000101, %ecx
    rdmsr
    shl $32, %rdx
    or %rax, %rdx
    push %rdx

    // Save IA32_KERNEL_GS_BASE (MSR 0xC0000102)
    movl $0xC0000102, %ecx
    rdmsr
    shl $32, %rdx
    or %rax, %rdx
    push %rdx

    // Save current callee-saved context
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    // Save RSP to *old
    mov %rsp, (%rdi)

    // Load RSP from new
    mov %rsi, %rsp

    // Restore new callee-saved context
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp

    // Restore IA32_KERNEL_GS_BASE (second push = first pop after callee regs)
    pop %rdx
    mov %rdx, %rax
    shr $32, %rdx
    movl $0xC0000102, %ecx
    wrmsr

    // Restore IA32_GS_BASE (first push = last pop)
    pop %rdx
    mov %rdx, %rax
    shr $32, %rdx
    movl $0xC0000101, %ecx
    wrmsr

    ret

.global thread_start
thread_start:
    pop %rdi
    pop %rsi
    sti
    call *%rdi
    xor %rdi, %rdi
    call task_exit
    hlt

.global switch_to_user_mode
switch_to_user_mode:
    cli
    // Arguments:
    // RDI = User RIP
    // RSI = User RSP
    
    // Save user RIP/RSP in callee-saved registers
    mov %rdi, %rbx
    mov %rsi, %rbp
    
    // Set up user segment registers (DS, ES, FS, GS)
    mov $0x1B, %ax       // User SS = 0x18 | 3
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    
    // Restore user RIP/RSP
    mov %rbx, %rdi
    mov %rbp, %rsi
    
    // Build IRETQ frame on stack: SS, RSP, RFLAGS, CS, RIP
    push $0x1B           // SS (user data selector | RPL=3)
    push %rsi            // RSP (user stack pointer)
    push $0x202          // RFLAGS (IF=1, bit 1=1)
    push $0x23           // CS (user code selector | RPL=3)
    push %rdi            // RIP (user entry point)
    
    iretq