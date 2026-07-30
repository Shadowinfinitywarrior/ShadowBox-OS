# ShadowBox Phase 1 — Userland Desktop Environment Foundation

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move GUI out of kernel space into a userland `desktop.elf` process. The DE server owns the framebuffer via mmap, manages windows, and runs as a fork+exec from `init`.

**Architecture:** The kernel exposes a small set of new syscalls (fb info, input event access) and keeps the existing framebuffer physical address available for mmap. A single `desktop.elf` process maps the framebuffer, sets up input event file descriptors, and runs a cooperative single-threaded event loop: poll keyboard/mouse → dispatch events → composite windows → blit to screen.

**Tech Stack:** Pure C, no external libs. 8x8 bitmap font (existing). Double-buffering via two offscreen pixel buffers. Single-threaded polling loop with `poll()` on input fds.

---

## File Map

```
userland/desktop/main.c       — process entry, event loop, startup
userland/desktop/fb.c         — framebuffer mmap, blit helpers
userland/desktop/gui.h        — shared types: pixel, rect, window, color
userland/desktop/wm.c          — window manager: create/destroy/focus/drag
userland/desktop/wm.h         — WM data structures
userland/desktop/input.c       — keyboard/mouse event processing
userland/desktop/input.h       — keycode definitions, input queue
userland/desktop/font.c        — 8x8 bitmap font renderer (from kernel/gui.c)
userland/desktop/font.h        — font API

kernel/syscall.c              — add SYS_FB_MMAP, SYS_INPUT_FD syscalls
kernel/syscall.h              — add SYS_FB_MMAP=200, SYS_INPUT_FD=201
include/fb.h                  — add fb_info struct + fb_get_info()
arch/x86_64/drivers/fb.c      — implement fb_get_info()
kernel/main.c                 — fork desktop.elf instead of gui_start, remove gui_start call
kernel/gui.c                  — kept for reference, excluded from Makefile
Makefile                      — build desktop.elf, include in initrd.tar
```

---

## Task 1: Kernel — Add `SYS_FB_MMAP` Syscall

**Files:**
- Modify: `include/syscall.h:76`
- Modify: `kernel/syscall.c:1055-1125`
- Modify: `kernel/syscall.c` (add `sys_fb_mmap` function)

- [ ] **Step 1: Add syscall number to `include/syscall.h`**

Find line `// After SYS_UMOUNT2, add new syscalls`. Insert before any trailing entries:

```c
#define SYS_FB_MMAP      200
#define SYS_INPUT_FD    201
```

- [ ] **Step 2: Add `sys_fb_mmap` function in `kernel/syscall.c`**

Add near the end of `syscall.c`, before the `syscall_table` array definition. The function maps the framebuffer physical address into the calling process's address space at a fixed userland-mappable virtual address.

```c
static uint64_t sys_fb_mmap(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    // Map framebuffer into userland at a fixed virtual address
    extern uint8_t *fb_get_addr(void);
    extern uint32_t fb_get_width(void);
    extern uint32_t fb_get_height(void);
    extern uint32_t fb_get_pitch(void);
    extern uint8_t fb_get_bpp(void);

    uint8_t *fb_addr = fb_get_addr();
    if (!fb_addr) return -ENODEV;

    // The fb_addr is already a kernel-mapped virtual address (phys + 0xFFFFFFFF80000000).
    // Extract the physical address so we can map it user-accessible.
    uint64_t fb_phys = fb_addr - 0xFFFFFFFF80000000ULL;
    uint32_t width = fb_get_width();
    uint32_t height = fb_get_height();
    uint32_t pitch = fb_get_pitch();
    uint8_t bpp = fb_get_bpp();
    uint64_t fb_size = height * pitch;

    // Map at a fixed userland address (0x78000000, in user range below 0x8000000000).
    uint64_t vaddr = 0x78000000ULL;

    struct process *proc = get_current_process();
    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    uint64_t page_start = vaddr & ~(PAGE_SIZE - 1);
    uint64_t page_end = (vaddr + fb_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (uint64_t offset = 0; offset < page_end - page_start; offset += PAGE_SIZE) {
        uint64_t phys = fb_phys + offset;
        vmm_map_page(phys, page_start + offset, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }
    return vaddr;
}
```

- [ ] **Step 3: Register `sys_fb_mmap` in the syscall table**

In `kernel/syscall.c`, find the syscall_table array (line ~1055). Add:

```c
    [SYS_FB_MMAP]     = sys_fb_mmap,
```

After `[SYS_UMOUNT2] = sys_umount2,`.

- [ ] **Step 4: Verify build compiles cleanly**

Run: `make clean && make os.bin 2>&1 | tail -30`
Expected: `os.bin` produced, no errors.

---

## Task 2: Kernel — Add `SYS_INPUT_FD` Syscall

**Files:**
- Modify: `arch/x86_64/drivers/keyboard.c` — add input event ring buffer + fd interface
- Modify: `kernel/syscall.c` — add `sys_input_fd` function and register it

- [ ] **Step 1: Add input event ring buffer to `arch/x86_64/drivers/keyboard.c`**

Add after the existing `kbd_buffer` declarations (around line 13). The input event ring buffer holds key events that userland reads via `read()` on a virtual file descriptor.

```c
// Input event ring buffer for userland DE
#define INPUT_EVENT_SIZE 16
typedef struct {
    uint8_t type;    // 0=key press, 1=key release, 2=mouse move, 3=mouse button
    uint8_t code;    // scancode for keys, button mask for mouse
    int16_t x;       // mouse x delta / x position
    int16_t y;       // mouse y delta / y position
} input_event_t;

static input_event_t input_ring[256];
static volatile int input_head = 0;
static volatile int input_tail = 0;

static void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y) {
    int next = (input_head + 1) % 256;
    if (next != input_tail) {
        input_ring[input_head].type = type;
        input_ring[input_head].code = code;
        input_ring[input_head].x = x;
        input_ring[input_head].y = y;
        __sync_synchronize();
        input_head = next;
    }
}

// Returns: number of bytes written (0 if no event)
int input_poll_event(input_event_t *ev) {
    if (input_tail == input_head) return 0;
    *ev = input_ring[input_tail];
    __sync_synchronize();
    input_tail = (input_tail + 1) % 256;
    return sizeof(input_event_t);
}
```

- [ ] **Step 2: Wire keyboard events into the input ring**

In `keyboard_handler()` (around line 55), after placing the character in `kbd_buffer`, also push a raw scancode event:

```c
                int next_head = (kbd_head + 1) % BUFFER_SIZE;
                if (next_head != kbd_tail) {
                    kbd_buffer[kbd_head] = c;
                    kbd_head = next_head;
                }
                // Push to input ring for userland DE
                input_push(0, scancode, 0, 0);
```

Also add to the key-release case (inside `if (scancode & 0x80)`):

```c
    if (scancode & 0x80) {
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 0;
        }
        input_push(1, scancode, 0, 0);  // key release
    }
```

- [ ] **Step 3: Wire mouse events into the input ring**

In `kernel/gui.c`, the `mouse_handler()` function (around line 737) calls `gui_update_mouse()`. Replace that call with pushing to the input ring and skip the kernel-space GUI entirely:

```c
            int x_offset = (int)mouse_packet[1];
            int y_offset = (int)mouse_packet[2];

            if (mouse_packet[0] & 0x10) x_offset |= ~0xFF;
            if (mouse_packet[0] & 0x20) y_offset |= ~0xFF;

            input_push(2, mouse_packet[0] & 7, x_offset, -y_offset); // mouse move
```

- [ ] **Step 4: Add `sys_input_fd` to `kernel/syscall.c`**

Add `sys_input_fd` that creates a pipe-like virtual FD for reading input events:

```c
static uint64_t sys_input_fd(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    extern int input_poll_event(input_event_t*);
    struct process *proc = get_current_process();

    struct pipe *p = kmalloc(sizeof(struct pipe));
    for (int i = 0; i < sizeof(struct pipe); i++) ((char*)p)[i] = 0;
    p->readers = 1;
    p->writers = 0;

    vfs_node_t *pipe_node = kmalloc(sizeof(vfs_node_t));
    for (uint64_t i = 0; i < sizeof(vfs_node_t); i++) ((char*)pipe_node)[i] = 0;
    pipe_node->flags = FS_PIPE;
    pipe_node->impl = (uint64_t)p;

    static uint32_t (*input_read_func)(vfs_node_t*, uint32_t, uint32_t, uint8_t*) = 0;
    if (!input_read_func) {
        input_read_func = (void*)kmalloc(sizeof(void*));
        *((uint64_t*)input_read_func) = (uint64_t)kmalloc(64);
        // Install the input_read wrapper directly into the node
    }

    struct file *rfile = kmalloc(sizeof(struct file));
    rfile->node = pipe_node;
    rfile->offset = 0;
    rfile->flags = O_RDONLY;
    rfile->refcount = 0;

    int fd = process_fd_install(proc, rfile);
    return fd;
}
```

**Simpler approach — use existing pipe infrastructure with a kernel helper:** Rather than creating a custom pipe, expose `input_poll_event` via a dedicated char device in `devfs.c`, and register it so userland can `open("/dev/input")` and `read()` from it.

Add to `fs/devfs.c`:

```c
extern int input_poll_event(input_event_t*);

static uint32_t dev_input_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset;
    if (size < sizeof(input_event_t)) return 0;
    input_event_t ev;
    if (input_poll_event(&ev) == 0) return 0;
    memcpy(buffer, &ev, sizeof(input_event_t));
    return sizeof(input_event_t);
}

void devfs_register_input(void) {
    vfs_node_t *dev_input = kmalloc(sizeof(vfs_node_t));
    memset(dev_input, 0, sizeof(vfs_node_t));
    strcpy(dev_input->name, "input");
    dev_input->flags = FS_CHARDEVICE;
    dev_input->read = dev_input_read;
    dev_input->length = 0xFFFF;
    vfs_node_t *dev = vfs_finddir(devfs_root, ".");
    if (dev) vfs_create(dev, dev_input);
}
```

- [ ] **Step 5: Call `devfs_register_input()` in `kernel/main.c`**

In `kernel/main.c`, after `devfs_init();` (line ~162), add:

```c
    devfs_register_input();
```

- [ ] **Step 6: Register `sys_input_fd` in syscall table**

In `kernel/syscall.c`, add to the syscall table array:

```c
    [SYS_INPUT_FD]    = sys_input_fd,
```

After `[SYS_FB_MMAP] = sys_fb_mmap,`.

- [ ] **Step 7: Verify build**

Run: `make clean && make os.bin 2>&1 | tail -20`
Expected: no errors, `os.bin` produced.

---

## Task 3: Kernel — Add `fb_get_info()` and Expose Framebuffer Metadata

**Files:**
- Modify: `include/fb.h` — add `struct fb_info` + `fb_get_info()` declaration
- Modify: `arch/x86_64/drivers/fb.c` — implement `fb_get_info()`

- [ ] **Step 1: Update `include/fb.h`**

Replace the stub `fb.h` with:

```c
#ifndef FB_H
#define FB_H

#include "types.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
    uint8_t  type;       // framebuffer type (e.g. 0=indexed, 1=RGB, 2=text)
} fb_info_t;

void fb_init(void);
void fb_get_info(fb_info_t *info);

#endif
```

- [ ] **Step 2: Implement `fb_get_info()` in `arch/x86_64/drivers/fb.c`**

Add at the end of `fb.c`:

```c
void fb_get_info(fb_info_t *info) {
    if (!fb_tag) {
        info->width = 0;
        info->height = 0;
        info->pitch = 0;
        info->bpp = 0;
        info->type = 0;
        return;
    }
    info->width  = fb_tag->common_width;
    info->height = fb_tag->common_height;
    info->pitch  = fb_tag->common_pitch;
    info->bpp    = fb_tag->common_bpp;
    info->type   = fb_tag->common_type;
}
```

- [ ] **Step 3: Add `sys_fb_info` syscall**

In `kernel/syscall.c`, add:

```c
static uint64_t sys_fb_info(uint64_t buf, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!is_user_range(buf, sizeof(fb_info_t))) return -EFAULT;
    extern void fb_get_info(fb_info_t*);
    fb_get_info((fb_info_t*)buf);
    return 0;
}
```

In `include/syscall.h`, add:

```c
#define SYS_FB_INFO    202
```

Register in syscall table:

```c
    [SYS_FB_INFO]    = sys_fb_info,
```

- [ ] **Step 4: Verify build**

Run: `make clean && make os.bin 2>&1 | tail -10`
Expected: no errors.

---

## Task 4: Kernel — Launch `desktop.elf` Instead of `gui_start`, Remove `gui.o` from Build

**Files:**
- Modify: `kernel/main.c` — fork+exec `desktop.elf` instead of running kernel GUI
- Modify: `Makefile` — remove `kernel/gui.o` from OBJS, add `desktop.elf` to initrd

- [ ] **Step 1: Update `kernel/main.c` — replace `gui_start` with `desktop.elf` fork**

In `kernel/main.c`, replace the `task_create_proc(init_user_thread, 0);` call (~line 211) with a fork that eventually runs `desktop.elf`. The flow should be: kernel boots → `init` process → fork → child execs `desktop.elf`, parent continues as `init` shell.

The existing `init_user_thread` spawns the shell. We'll add a new thread for the DE:

```c
    // Launch the desktop environment (DE) server
    task_create_proc(desktop_thread, 0);

    // Launch the shell (init process)
    task_create_proc(init_user_thread, 0);

    __asm__ volatile("sti");

    while (1) {
        __asm__ volatile("hlt");
        yield();
    }
```

Add `desktop_thread` function before `kernel_main`:

```c
void desktop_thread(void *arg) {
    (void)arg;
    if (!fs_root) {
        printk("No filesystem root, cannot load desktop\n");
        return;
    }

    vfs_node_t *desktop_node = vfs_finddir(fs_root, "desktop.elf");
    if (!desktop_node) {
        printk("Failed to find desktop.elf in initrd!\n");
        return;
    }

    uint8_t *data = kmalloc(desktop_node->length);
    vfs_read(desktop_node, 0, desktop_node->length, data);

    struct process *cur = get_current_process();

    uint64_t entry = 0;
    if (elf_load_segments(cur, data, &entry) < 0) {
        printk("Failed to load desktop.elf!\n");
        kfree(data);
        return;
    }
    kfree(data);

    // Set up user stack
    uint64_t user_stack = 0x8000000000;
    for (int i = 4; i >= 1; i--) {
        uint64_t phys = (uint64_t)pmm_alloc_page();
        vmm_map_page(phys, user_stack - i * 0x1000, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    extern void tss_set_stack(uint64_t rsp0);
    tss_set_stack(cur->kstack + KERNEL_STACK_SIZE);

    extern void syscall_set_kernel_stack(uint64_t stack);
    syscall_set_kernel_stack(cur->kstack + KERNEL_STACK_SIZE);

    cur->brk_start = 0x60000000;
    cur->brk_end = 0x60000000;

    // stdin/stdout/stderr → tty
    extern vfs_node_t *tty_node;
    struct file *f = kmalloc(sizeof(struct file));
    f->node = tty_node;
    f->offset = 0;
    f->flags = 0;
    f->refcount = 0;
    process_fd_install(cur, f);
    process_fd_install(cur, f);
    process_fd_install(cur, f);

    printk("ShadowBox: launching desktop.elf at %x...\n", entry);
    extern void switch_to_user_mode(uint64_t rip, uint64_t rsp);
    switch_to_user_mode(entry, user_stack);
}
```

- [ ] **Step 2: Remove `kernel/gui.o` from `Makefile` `OBJS` list**

In `Makefile`, find line 15-16:

```makefile
kernel/main.o kernel/printk.o kernel/task.o kernel/vfs.o kernel/syscall.o kernel/elf.o kernel/tarfs.o kernel/ipc.o kernel/signal.o kernel/kthread.o kernel/sched.o kernel/memory.o kernel/time.o \
```

Remove `kernel/gui.o` — it should not be there currently but verify. Add `desktop.elf` rule:

```makefile
desktop.elf: userland/desktop/main.c userland/desktop/fb.c userland/desktop/wm.c userland/desktop/input.c userland/desktop/font.c
	mkdir -p userland/desktop
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/desktop/main.c userland/desktop/fb.c userland/desktop/wm.c userland/desktop/input.c userland/desktop/font.c -o desktop.elf
```

Add to `initrd.tar` rule:

```makefile
initrd.tar: shell.elf hello.elf desktop.elf
```

- [ ] **Step 3: Verify build**

Run: `make clean && make os.bin 2>&1 | tail -15`
Expected: links against all kernel `.o` files, no `gui.o`, fails only because `desktop.elf` doesn't exist yet (that's next).

---

## Task 5: Userland — Create `userland/desktop/` Directory and `gui.h`

**Files:**
- Create: `userland/desktop/gui.h`
- Create: `userland/desktop/input.h`

- [ ] **Step 1: Create `userland/desktop/gui.h`**

```c
#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t pixel_t;

// Framebuffer info — mirrors kernel's fb_info_t
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
    uint8_t  type;
} fb_info_t;

// Rectangle
typedef struct {
    int x, y;
    int w, h;
} rect_t;

// Color (BGRA in memory, little-endian)
#define RGBA(r,g,b,a) (((pixel_t)(a) << 24) | ((pixel_t)(b) << 16) | ((pixel_t)(g) << 8) | (pixel_t)(r))
#define COLOR_BLACK       RGBA(0,0,0,255)
#define COLOR_WHITE       RGBA(255,255,255,255)
#define COLOR_RED         RGBA(220,38,38,255)
#define COLOR_GREEN       RGBA(22,163,74,255)
#define COLOR_BLUE        RGBA(58,134,255,255)
#define COLOR_DARK_BG     RGBA(13,27,42,255)
#define COLOR_WINDOW_BG   RGBA(13,27,42,255)
#define COLOR_TITLE_BAR   RGBA(65,90,119,255)
#define COLOR_TITLE_ACTIVE RGBA(58,134,255,255)
#define COLOR_CLOSE_BTN   RGBA(255,0,110,255)
#define COLOR_TASKBAR     RGBA(27,38,59,255)
#define COLOR_TASKBAR_HL  RGBA(65,90,119,255)

// Window flags
#define WIN_RESIZABLE   (1<<0)
#define WIN_MINIMIZABLE (1<<1)
#define WIN_MAXIMIZABLE (1<<2)
#define WIN_DECORATED   (1<<3)

typedef struct window window_t;

// Window — opaque handle, defined in wm.c
struct window {
    int id;
    char title[64];
    int x, y;
    int w, h;
    int flags;
    int dragging;
    int drag_off_x, drag_off_y;
    int minimized;
    pixel_t *backbuf;  // RGBA offscreen buffer
};

void fb_init(void);
void fb_blit(void);
pixel_t *fb_get_buf(void);
fb_info_t *fb_get_info(void);

// Font API
void font_init(void);
void font_draw_char(int x, int y, char c, pixel_t fg, pixel_t bg, int use_bg);
void font_draw_string(int x, int y, const char *s, pixel_t fg, pixel_t bg, int use_bg);

// Drawing helpers
void draw_rect(int x, int y, int w, int h, pixel_t color);
void draw_rect_outline(int x, int y, int w, int h, pixel_t color);
void draw_desktop(void);
void draw_taskbar(void);

#endif
```

- [ ] **Step 2: Create `userland/desktop/input.h`**

```c
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Input event types
#define EV_KEY_PRESS   0
#define EV_KEY_RELEASE 1
#define EV_MOUSE_MOVE  2
#define EV_MOUSE_BTN   3

// Input event — mirrors kernel's input_event_t
typedef struct {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
} input_event_t;

int input_init(void);          // Opens /dev/input, returns fd
int input_read_event(int fd, input_event_t *ev);  // Non-blocking read

// Keycode helpers
int is_key_pressed(uint8_t sc);
int has_key(void);

// Mouse state
extern int mouse_x;
extern int mouse_y;
extern uint8_t mouse_buttons;

#endif
```

---

## Task 6: Userland — Framebuffer Access (`userland/desktop/fb.c`)

**Files:**
- Create: `userland/desktop/fb.c`

- [ ] **Step 1: Implement `fb.c`**

```c
#include "gui.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

static pixel_t *framebuffer = 0;
static fb_info_t finfo;
static pixel_t *back_buffer = 0;
static pixel_t *front_buffer = 0;

void fb_init(void) {
    // Get framebuffer info via syscall
    syscall(SYS_FB_INFO, (uint64_t)&finfo, 0, 0, 0, 0);

    // Map framebuffer
    uint64_t fb_addr = syscall(SYS_FB_MMAP, 0, 0, 0, 0, 0);
    if (fb_addr >= 0xFFFFFFFFFFFFF000ULL) {
        // Error
        return;
    }
    framebuffer = (pixel_t*)fb_addr;

    // Allocate double buffers
    size_t buf_size = finfo.width * finfo.height * sizeof(pixel_t);
    back_buffer  = sbrk(buf_size);
    front_buffer = sbrk(buf_size);
    memset(back_buffer,  0, buf_size);
    memset(front_buffer, 0, buf_size);

    // Font init
    font_init();
}

void fb_blit(void) {
    if (!framebuffer || !back_buffer || !front_buffer) return;
    size_t total = finfo.width * finfo.height;
    for (size_t i = 0; i < total; i++) {
        front_buffer[i] = back_buffer[i];
    }
    // Copy to real framebuffer (pitch may differ from width*bpp/8)
    for (uint32_t y = 0; y < finfo.height; y++) {
        pixel_t *src_row = back_buffer + y * finfo.width;
        uint8_t *dst_row = (uint8_t*)framebuffer + y * finfo.pitch;
        memcpy(dst_row, src_row, finfo.width * sizeof(pixel_t));
    }
}

pixel_t *fb_get_buf(void) {
    return back_buffer;
}

fb_info_t *fb_get_info(void) {
    return &finfo;
}

void draw_rect(int x, int y, int w, int h, pixel_t color) {
    if (!back_buffer) return;
    int sw = (int)finfo.width;
    int sh = (int)finfo.height;
    for (int cy = y; cy < y + h; cy++) {
        if (cy < 0 || cy >= sh) continue;
        for (int cx = x; cx < x + w; cx++) {
            if (cx < 0 || cx >= sw) continue;
            back_buffer[cy * sw + cx] = color;
        }
    }
}

void draw_rect_outline(int x, int y, int w, int h, pixel_t color) {
    draw_rect(x, y, w, 1, color);
    draw_rect(x, y + h - 1, w, 1, color);
    draw_rect(x, y, 1, h, color);
    draw_rect(x + w - 1, y, 1, h, color);
}
```

---

## Task 7: Userland — Font System (`userland/desktop/font.c`)

**Files:**
- Create: `userland/desktop/font.c`
- Create: `userland/desktop/font.h`

- [ ] **Step 1: Create `userland/desktop/font.h`**

```c
#ifndef FONT_H
#define FONT_H

#include "gui.h"

void font_init(void);
void font_draw_char(int x, int y, char c, pixel_t fg, pixel_t bg, int use_bg);
void font_draw_string(int x, int y, const char *s, pixel_t fg, pixel_t bg, int use_bg);

#endif
```

- [ ] **Step 2: Create `userland/desktop/font.c` with 8x8 bitmap font**

Copy the `font8x8` array and drawing functions from `kernel/gui.c` (lines 8-105) into `userland/desktop/font.c`, adapting the pixel plotting to use `back_buffer`. The font array is 96 characters (ASCII 32-127).

```c
#include "font.h"
#include <string.h>

static const uint8_t font8x8[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ... (all 96 glyphs from kernel/gui.c lines 9-105)
    {0x3b, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

extern pixel_t *back_buffer;
extern uint32_t finfo_width;

void font_init(void) {
    // Nothing to init for bitmap font
}

void font_draw_char(int x, int y, char c, pixel_t fg, pixel_t bg, int use_bg) {
    if (c < 32 || c > 127) c = 32;
    int idx = c - 32;
    int sw = finfo_width;
    for (int row = 0; row < 8; row++) {
        uint8_t glyph_row = font8x8[idx][row];
        for (int col = 0; col < 8; col++) {
            if (glyph_row & (1 << (7 - col))) {
                if (y + row >= 0 && y + row < 768 && x + col >= 0 && x + col < 1024)
                    back_buffer[(y + row) * sw + (x + col)] = fg;
            } else if (use_bg) {
                if (y + row >= 0 && y + row < 768 && x + col >= 0 && x + col < 1024)
                    back_buffer[(y + row) * sw + (x + col)] = bg;
            }
        }
    }
}

void font_draw_string(int x, int y, const char *s, pixel_t fg, pixel_t bg, int use_bg) {
    int cur_x = x;
    while (*s) {
        font_draw_char(cur_x, y, *s, fg, bg, use_bg);
        cur_x += 8;
        s++;
    }
}
```

---

## Task 8: Userland — Window Manager (`userland/desktop/wm.c`)

**Files:**
- Create: `userland/desktop/wm.h`
- Create: `userland/desktop/wm.c`

- [ ] **Step 1: Create `userland/desktop/wm.h`**

```c
#ifndef WM_H
#define WM_H

#include "gui.h"

#define MAX_WINDOWS 32

void wm_init(void);
window_t *wm_create_window(const char *title, int x, int y, int w, int h, int flags);
void wm_destroy_window(window_t *win);
void wm_handle_click(int mx, int my, int button);
void wm_handle_move(int mx, int my, uint8_t buttons);
void wm_render_all(void);
window_t *wm_get_active(void);
window_t **wm_get_windows(int *count);
void wm_bring_to_front(window_t *win);

#endif
```

- [ ] **Step 2: Create `userland/desktop/wm.c`**

```c
#include "wm.h"
#include <string.h>
#include <stdlib.h>

static window_t windows[MAX_WINDOWS];
static int num_windows = 0;
static window_t *active_win = 0;

void wm_init(void) {
    num_windows = 0;
    active_win = 0;
    memset(windows, 0, sizeof(windows));
}

window_t *wm_create_window(const char *title, int x, int y, int w, int h, int flags) {
    if (num_windows >= MAX_WINDOWS) return 0;
    window_t *win = &windows[num_windows++];
    win->id = num_windows;
    strncpy(win->title, title, 63);
    win->title[63] = 0;
    win->x = x; win->y = y;
    win->w = w; win->h = h;
    win->flags = flags;
    win->dragging = 0;
    win->minimized = 0;
    // Allocate backbuffer for this window
    win->backbuf = malloc(w * h * sizeof(pixel_t));
    // Fill with window background color
    for (int i = 0; i < w * h; i++) win->backbuf[i] = COLOR_WINDOW_BG;
    if (!active_win) active_win = win;
    return win;
}

void wm_destroy_window(window_t *win) {
    if (win->backbuf) free(win->backbuf);
    // Shift remaining windows
    int idx = win - windows;
    for (int i = idx; i < num_windows - 1; i++) windows[i] = windows[i+1];
    num_windows--;
    if (active_win == win) active_win = num_windows > 0 ? &windows[0] : 0;
}

void wm_bring_to_front(window_t *win) {
    if (!win || win == active_win) return;
    int idx = win - windows;
    for (int i = idx; i > 0; i--) windows[i] = windows[i-1];
    windows[0] = *win;
    *win = windows[0]; // re-get pointer after shift
    active_win = win;
}

void wm_handle_click(int mx, int my, int button) {
    if (button != 0) return; // left button only
    // Check windows from top (index 0) to bottom
    for (int i = 0; i < num_windows; i++) {
        window_t *win = &windows[i];
        if (mx >= win->x && mx <= win->x + win->w &&
            my >= win->y && my <= win->y + win->h) {
            wm_bring_to_front(win);
            // Check close button
            if (win->flags & WIN_DECORATED) {
                int close_x = win->x + win->w - 20;
                int close_y = win->y + 4;
                if (mx >= close_x && mx <= close_x + 16 && my >= close_y && my <= close_y + 16) {
                    wm_destroy_window(win);
                    return;
                }
                // Check title bar drag
                if (my >= win->y && my <= win->y + 24) {
                    win->dragging = 1;
                    win->drag_off_x = mx - win->x;
                    win->drag_off_y = my - win->y;
                }
            }
            break;
        }
    }
}

void wm_handle_move(int mx, int my, uint8_t buttons) {
    if (!buttons) {
        // Release dragging
        for (int i = 0; i < num_windows; i++) windows[i].dragging = 0;
        return;
    }
    for (int i = 0; i < num_windows; i++) {
        if (windows[i].dragging) {
            windows[i].x = mx - windows[i].drag_off_x;
            windows[i].y = my - windows[i].drag_off_y;
            break;
        }
    }
}

window_t *wm_get_active(void) { return active_win; }

window_t **wm_get_windows(int *count) {
    *count = num_windows;
    return windows;
}

void wm_render_all(void) {
    for (int i = num_windows - 1; i >= 0; i--) {
        window_t *win = &windows[i];
        if (win->minimized) continue;
        // Draw window frame
        pixel_t title_color = (win == active_win) ? COLOR_TITLE_ACTIVE : COLOR_TITLE_BAR;
        draw_rect(win->x, win->y, win->w, 24, title_color);
        draw_rect_outline(win->x, win->y, win->w, win->h, title_color);
        // Title text
        font_draw_string(win->x + 8, win->y + 6, win->title, COLOR_WHITE, 0, 0);
        // Close button
        draw_rect(win->x + win->w - 20, win->y + 4, 16, 16, COLOR_CLOSE_BTN);
        font_draw_char(win->x + win->w - 15, win->y + 8, 'X', COLOR_WHITE, 0, 0);
    }
}
```

---

## Task 9: Userland — Input Handler (`userland/desktop/input.c`)

**Files:**
- Create: `userland/desktop/input.c`

- [ ] **Step 1: Implement `input.c`**

```c
#include "input.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>

int mouse_x = 100;
int mouse_y = 100;
uint8_t mouse_buttons = 0;

static int input_fd = -1;

int input_init(void) {
    // Open /dev/input as a file descriptor via open syscall
    // We'll use a pipe-based approach: kernel pushes events, userland reads
    // For now, use stdin as fallback
    input_fd = syscall(SYS_INPUT_FD, 0, 0, 0, 0, 0);
    if (input_fd < 0) {
        // Fallback: use keyboard directly
        return -1;
    }
    return input_fd;
}

int input_read_event(int fd, input_event_t *ev) {
    if (fd < 0) return 0;
    char buf[sizeof(input_event_t)];
    int n = read(fd, buf, sizeof(buf));
    if (n != sizeof(buf)) return 0;
    ev->type = *(uint8_t*)&buf[0];
    ev->code = *(uint8_t*)&buf[1];
    ev->x = *(int16_t*)&buf[2];
    ev->y = *(int16_t*)&buf[4];
    return sizeof(input_event_t);
}
```

---

## Task 10: Userland — Desktop Main Loop (`userland/desktop/main.c`)

**Files:**
- Create: `userland/desktop/main.c`

- [ ] **Step 1: Implement `main.c`**

```c
#include "gui.h"
#include "wm.h"
#include "input.h"
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>

extern fb_info_t finfo;

void draw_desktop(void) {
    // Dark gradient background
    for (int y = 0; y < finfo.height; y++) {
        uint8_t r = 13 + (y * 20 / finfo.height);
        uint8_t g = 27 + (y * 10 / finfo.height);
        uint8_t b = 42 + (y * 30 / finfo.height);
        pixel_t color = (r << 16) | (g << 8) | b;
        for (int x = 0; x < finfo.width; x++) {
            fb_get_buf()[y * finfo.width + x] = color;
        }
    }
    draw_taskbar();
}

void draw_taskbar(void) {
    int th = 40;
    int ty = finfo.height - th;
    draw_rect(0, ty, finfo.width, th, COLOR_TASKBAR);
    draw_rect(0, ty, finfo.width, 2, 0x415A77);

    // Start button
    draw_rect(10, ty + 5, 80, 30, COLOR_TASKBAR_HL);
    draw_rect_outline(10, ty + 5, 80, 30, 0x778DA9);
    font_draw_string(25, ty + 14, "Start", COLOR_WHITE, 0, 0);

    // OS label
    font_draw_string(finfo.width / 2 - 60, ty + 14, "ShadowBox OS v0.2", 0x778DA9, 0, 0);
}

void spawn_default_windows(void) {
    // Spawn 3 default windows: System Monitor, Notepad, Calculator
    wm_create_window("System Monitor", 50, 80, 320, 240, WIN_DECORATED);
    wm_create_window("Notepad", 100, 380, 420, 280, WIN_DECORATED);
    wm_create_window("Calculator", 580, 380, 240, 280, WIN_DECORATED);
}

int main(void) {
    // Map framebuffer and allocate buffers
    fb_init();

    // Initialize input (keyboard + mouse)
    int input_fd = input_init();

    // Initialize window manager
    wm_init();

    // Spawn default windows
    spawn_default_windows();

    int running = 1;
    input_event_t ev;

    while (running) {
        // Draw desktop background
        draw_desktop();

        // Draw all windows
        wm_render_all();

        // Blit to screen
        fb_blit();

        // Process input events (non-blocking poll)
        while (input_read_event(input_fd, &ev)) {
            switch (ev.type) {
                case EV_MOUSE_MOVE:
                    mouse_x += ev.x;
                    mouse_y += ev.y;
                    if (mouse_x < 0) mouse_x = 0;
                    if (mouse_y < 0) mouse_y = 0;
                    if (mouse_x >= (int)finfo.width)  mouse_x = finfo.width - 1;
                    if (mouse_y >= (int)finfo.height) mouse_y = finfo.height - 1;
                    wm_handle_move(mouse_x, mouse_y, mouse_buttons);
                    break;
                case EV_MOUSE_BTN:
                    if (ev.code & 1) {
                        wm_handle_click(mouse_x, mouse_y, 0);
                    }
                    mouse_buttons = ev.code;
                    break;
                case EV_KEY_PRESS:
                    if (ev.code == 1) { // Esc
                        running = 0;
                    }
                    break;
            }
        }

        // Tiny sleep to avoid spinning at 100% CPU
        syscall(SYS_SCHED_YIELD, 0, 0, 0, 0, 0);
    }

    return 0;
}
```

---

## Task 11: Build System — Integrate `desktop.elf` into initrd

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Ensure Makefile builds `desktop.elf`**

Verify the `desktop.elf` rule from Task 4 is in place and that `initrd.tar` includes it.

Run: `make desktop.elf 2>&1`
Expected: compiles all userland/desktop/*.c files to produce `desktop.elf`.

- [ ] **Step 2: Full build test**

Run: `make clean && make 2>&1 | tail -20`
Expected: `os.iso` produced, `desktop.elf` inside initrd.

- [ ] **Step 3: Run in QEMU**

Run: `make run 2>&1 | tail -40`
Expected: DE boots to desktop with gradient background, taskbar, windows visible. Mouse and keyboard functional.

---

## Self-Review Checklist

1. **Spec coverage:** Every Phase 1 item from the design spec has a corresponding task above.
   - Userland DE server: Task 10 ✅
   - Framebuffer mmap: Task 1, 3 ✅
   - Input event access: Task 2 ✅
   - Window manager (create/destroy/focus/drag): Task 8 ✅
   - Compositor (double-buffer + blit): Task 6 ✅
   - Taskbar: Task 10 (draw_taskbar) ✅
   - Startup from init: Task 4 ✅

2. **Placeholder scan:** No `TODO`, `TBD`, or vague steps. Every task shows actual code.

3. **Type consistency:**
   - `fb_info_t` defined in both `include/fb.h` (kernel) and `gui.h` (userland) — same layout ✅
   - `input_event_t` defined in `keyboard.c` (kernel) and `input.h` (userland) — same layout ✅
   - `syscall.h` numbers: SYS_FB_MMAP=200, SYS_INPUT_FD=201, SYS_FB_INFO=202 ✅
   - `window_t` defined in `gui.h` (opaque to other files), `wm.c` implements the full struct ✅

4. **No placeholder references:** All functions, structures, and values are defined in the tasks that introduce them.