/*
 * Assembly drawing primitives for the GUI subsystem.
 * Currently provides only draw_pixel which forwards to the C
 * framebuffer driver (fb_putpixel). Additional primitives can be
 * built on top of this.
 *
 * System V AMD64 ABI:
 *   rdi = x (int)
 *   rsi = y (int)
 *   rdx = color (uint32_t)
 */

    .text
    .global draw_pixel
    .type draw_pixel, @function

draw_pixel:
    # forward arguments to fb_putpixel (extern C function)
    # rdi, rsi, rdx already hold x, y, color
    jmp fb_putpixel
    # No return as fb_putpixel returns void
    .size draw_pixel, .-draw_pixel

    .global clear_screen
    .type clear_screen, @function
clear_screen:
    # Clear the entire framebuffer to black (0)
    # We'll call fb_get_addr, fb_get_width, fb_get_height, fb_get_pitch
    # and then loop in assembly.
    # For simplicity, we defer to a C helper (see draw.c) – this stub
    # Clear the screen using fb_fill_rect
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # Save original arguments (none needed)
    # Get framebuffer address
    call fb_get_addr
    mov %rax, %rbx               # fb base

    # Get stride (pitch)
    call fb_get_pitch
    mov %eax, %esi               # stride

    # Get width
    call fb_get_width
    mov %eax, %r13d              # width

    # Get height
    call fb_get_height
    mov %eax, %r14d              # height

    # Call fb_fill_rect(fb, stride, 0, 0, width, height, 0)
    mov %rbx, %rdi
    mov %rsi, %rsi
    xor %rdx, %rdx
    xor %rcx, %rcx
    mov %r13, %r8
    mov %r14, %r9
    xor %rax, %rax
    push %rax
    call fb_fill_rect
    add $8, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
    .size clear_screen, .-clear_screen

    .global draw_rect
    .type draw_rect, @function
/*
 * draw_rect(x, y, w, h, color)
 * Arguments: rdi=x, rsi=y, rdx=w, rcx=h, r8d=color
 */
draw_rect:
    # Simple implementation in C; stub returns.
    ret
    .size draw_rect, .-draw_rect

    .global draw_line
    .type draw_line, @function
/*
 * draw_line(x0, y0, x1, y1, color)
 * Arguments: rdi=x0, rsi=y0, rdx=x1, rcx=y1, r8d=color
 */
draw_line:
    # Implement line drawing using Bresenham algorithm and fb_putpixel
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # Save arguments
    mov %rdi, %r12   # x0
    mov %rsi, %r13   # y0
    mov %rdx, %r14   # x1
    mov %rcx, %r15   # y1
    mov %r8, %rbx    # color

    # Compute delta x and delta y
    mov %r14, %rax
    sub %r12, %rax
    mov %rax, %r8    # dx (signed)

    mov %r15, %rax
    sub %r13, %rax
    mov %rax, %r9    # dy (signed)

    # Determine step direction for x (sx) and y (sy)
    mov $1, %r10d    # sx = 1
    test %r8, %r8
    jge .dx_nonneg
    mov $-1, %r10d
    neg %r8          # make dx positive
.dx_nonneg:

    mov $1, %r11d    # sy = 1
    test %r9, %r9
    jge .dy_nonneg
    mov $-1, %r11d
    neg %r9          # make dy positive
.dy_nonneg:

    # Initialize error term
    mov %r8, %rax
    sub %r9, %rax    # err = dx - dy

.line_loop:
    # Plot pixel at (x, y)
    mov %r12, %rdi
    mov %r13, %rsi
    mov %rbx, %rdx
    call fb_putpixel

    # Check if reached endpoint
    cmp %r14, %r12
    jne .not_done
    cmp %r15, %r13
    je .line_done
.not_done:

    # e2 = 2*err
    lea (%rax,%rax,1), %rcx   # e2 in rcx

    # If e2 > -dy (i.e., e2 + dy > 0) then adjust x
    mov %rcx, %rdx
    add %r9, %rdx
    cmp $0, %rdx
    jle .skip_x
    sub %r9, %rax            # err -= dy
    add %r10, %r12           # x += sx
.skip_x:

    # If e2 < dx then adjust y
    cmp %r8, %rcx
    jge .skip_y
    add %r8, %rax             # err += dx
    add %r11, %r13            # y += sy
.skip_y:
    jmp .line_loop

.line_done:
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
    .size draw_line, .-draw_line

    .global draw_hline
    .type draw_hline, @function
/*
 * draw_hline(x, y, length, color)
 * Arguments: rdi=x, rsi=y, rdx=length, rcx=color
 */
draw_hline:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # Save arguments
    mov %rdi, %r12   # x
    mov %rsi, %r13   # y
    mov %rdx, %r14   # length (w)
    mov %rcx, %r15   # color

    # Get framebuffer address
    call fb_get_addr
    mov %rax, %rbx               # fb base

    # Get stride (pitch)
    call fb_get_pitch
    mov %eax, %esi               # stride

    # Call fb_fill_rect(fb, stride, x, y, w, 1, color)
    mov %rbx, %rdi
    mov %r12, %rdx
    mov %r13, %rcx
    mov %r14, %r8    # w = length
    mov $1, %r9      # h = 1
    push %r15        # color
    call fb_fill_rect
    add $8, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
    .size draw_hline, .-draw_hline

    .global draw_vline
    .type draw_vline, @function
/*
 * draw_vline(x, y, length, color)
 * Arguments: rdi=x, rsi=y, rdx=length, rcx=color
 */
draw_vline:
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15

    # Save arguments
    mov %rdi, %r12   # x
    mov %rsi, %r13   # y
    mov %rdx, %r14   # length (h)
    mov %rcx, %r15   # color

    # Get framebuffer address
    call fb_get_addr
    mov %rax, %rbx               # fb base

    # Get stride (pitch)
    call fb_get_pitch
    mov %eax, %esi               # stride

    # Call fb_fill_rect(fb, stride, x, y, 1, h, color)
    mov %rbx, %rdi
    mov %r12, %rdx
    mov %r13, %rcx
    mov $1, %r8    # w = 1
    mov %r14, %r9   # h = length
    push %r15       # color
    call fb_fill_rect
    add $8, %rsp

    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    ret
    .size draw_vline, .-draw_vline
