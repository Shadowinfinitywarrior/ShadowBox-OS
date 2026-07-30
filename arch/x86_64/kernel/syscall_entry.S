.global syscall_entry
.extern syscall_handler

syscall_entry:
    swapgs
    mov %rsp, %gs:8      // Save user RSP to GS:8
    mov %gs:0, %rsp      // Load kernel RSP from GS:0
    
    // Save all registers (syscall only preserves RCX=RIP, R11=RFLAGS in 64-bit)
    // Callee-saved regs MUST be saved for fork to work correctly
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    push %rcx            // User RIP
    push %r11            // User RFLAGS
    push %r10
    push %r9
    push %r8
    push %rdi
    push %rsi
    push %rdx
    push %rax            // Syscall number / return value

    mov %r8, %r9
    mov %r10, %r8
    mov %rdx, %rcx
    mov %rsi, %rdx
    mov %rdi, %rsi
    mov %rax, %rdi
    
    call syscall_handler

    mov %rax, %r15       // save return value

    call check_and_deliver_signals

    mov %r15, %rax       // restore return value

    add $8, %rsp         // skip pushed rax
    pop %rdx
    pop %rsi
    pop %rdi
    pop %r8
    pop %r9
    pop %r10
    pop %r11             // RFLAGS
    pop %rcx             // RIP

    // Restore callee-saved registers
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp

    mov %gs:8, %rsp
    swapgs
    
    sysretq
