# OS Kernel from Scratch — Design Specification

**Date:** 2026-07-15
**Type:** Operating System Kernel (Layered Monolithic / Microkernel-inspired)
**Target:** x86_64, GRUB2 multiboot2, QEMU
**Goal:** Efficient, Linux-inspired kernel with clean subsystems and performance-critical design

---

## 1. Overview

Build a functioning OS kernel from scratch targeting x86_64 architecture. The kernel follows a layered monolithic design — microkernel-inspired separation of concerns (PMM, VMM, scheduler, IPC, VFS) implemented inside a single address space for performance. Linux is the reference for design philosophy, efficiency patterns, and syscall interface.

**Core principles:**
- Performance-critical from day one (per-CPU data, lock-free IPC, TLB optimization)
- Clean subsystem boundaries — each module has one clear purpose and well-defined interfaces
- No stub code — every subsystem works and is testable
- Build incrementally: boot → print → PMM → VMM → Scheduler → IPC → VFS → Shell

---

## 2. Architecture

### 2.1 Layered Design

```
Ring 3 (User Space)
  init process → shell → user programs / tests

Ring 0 (Kernel Space)
  System Call Interface (syscall/sysret)
  ┌─────────────┬─────────────┬─────────────┬──────────┐
  │ VMM (paging)│ PMM (buddy) │ Process/Sched│ IPC      │
  ├─────────────┴─────────────┴─────────────┴──────────┤
  │              VFS + Block Device Layer              │
  ├────────────────────────────────────────────────────┤
  │          Drivers (Serial, VGA, Keyboard, Timer)    │
  ├────────────────────────────────────────────────────┤
  │         Hardware Abstraction (GDT, IDT, APIC)     │
  └────────────────────────────────────────────────────┘
  GRUB2 multiboot2 → Stage1 (asm) → Stage2 (head.S) → kernel_main
```

### 2.2 Subsystem Responsibilities

| Subsystem | Responsibility | Key Interface |
|---|---|---|
| **PMM** | Physical memory allocation (buddy system) | `pmm_alloc_pages(order)`, `pmm_free_pages(addr, order)` |
| **VMM** | Virtual memory, kmalloc, vmap | `vmm_alloc_pages()`, `vmm_free_pages()`, `kmalloc(size)` |
| **Process** | PCB management, process creation | `process_create()`, `process_exit()`, `process_wait()` |
| **Scheduler** | Per-CPU run queues, MLFQ | `schedule()`, `yield()`, `wake_up()` |
| **IPC** | Lock-free message queues, signals, semaphores | `ipc_send()`, `ipc_recv()`, `signal()`, `sem_wait()` |
| **VFS** | Inodes, dentries, file abstraction | `vfs_open()`, `vfs_read()`, `vfs_write()`, `vfs_mount()` |
| **Drivers** | Hardware I/O | Serial port, VGA text, PS/2 keyboard, PIT/APIC timer |
| **HAL** | GDT, IDT, LAPIC, IOAPIC, PIC | Architecture-specific initialization |

---

## 3. Physical Memory Manager (PMM)

### 3.1 Buddy Allocator

- **Block sizes:** 2^4KB to 2^14KB (orders 0–10), plus order 10 = 2MB for large allocations
- **Free lists:** One list per order (11 total), doubly linked
- **Splitting:** Split higher-order blocks in half until requested order is reached
- **Coalescing:** When a block is freed, check if its buddy is also free → merge up
- **Memory map:** Parse multiboot2 memory map; mark usable regions, reserve reserved/ACPI
- **Bitmap:** Optional secondary bitmap for O(1) allocation checks on large systems

### 3.2 API

```c
void* pmm_alloc_pages(int order);  // order 0 = 4KB, order 10 = 2MB
void  pmm_free_pages(void* addr, int order);
void  pmm_init(multiboot2_info_t* mbi);
```

### 3.3 Performance Notes

- Allocation: O(log n) where n = max order (constant ~11 steps)
- Free: O(log n) with buddy merge
- Per-CPU caches not needed at this scale; revisit if PMM becomes a bottleneck

---

## 4. Virtual Memory Manager (VMM)

### 4.1 4-Level Paging (x86_64)

- **Virtual address space:** 48-bit (256TB user, 256TB kernel split)
- **Page size:** 4KB default, 2MB large pages for kernel mappings
- **Page tables:** PML4 → PDPT → PD → PT (4KB each entry = 512 entries)
- **Kernel mapping:** Identity map low 1GB for early boot, then switch to high mapping (0xFFFF800000000000+)
- **User mapping:** 0x0000 – 0x00007FFFFFFFFFFF (128TB)

### 4.2 Kernel Heap (Slab Allocator on top of VMM)

- **Slab caches:** Per object-type caches (e.g., `proc_cache`, `vma_cache`, `inode_cache`)
- **Sizes:** 32B, 64B, 128B, 256B, 512B, 1KB, 2KB, 4KB, 8KB
- **Per-CPU slabs:** Reduce lock contention for common allocations
- **kmalloc:** Find smallest fitting slab cache; fallback to page allocator for large (> 8KB) allocations
- **kfree:** Determine cache from object address via slab redzone; return to appropriate cache

### 4.3 VM Areas (Linux-style)

- Per-process linked list of `vm_area_struct` (virtual memory areas)
- `mmap`: Allocate new VMA, insert into list, setup page tables on demand
- Page fault handler: Allocate physical page, map it, handle COW if needed

### 4.4 TLB Management

- **TLB flush:** `invlpg` for single page, `mov cr3` for full flush
- **TLB shootdown:** LAPIC IPI to all other CPUs requesting flush of a specific PML4 entry
- **Batch flushing:** Per-CPU pending flush bitmap; flush when returning to userspace or on timer tick
- **PCID:** Enable process-context identifiers to avoid full TLB flush on context switch (optimization)

### 4.5 API

```c
void*  vmm_alloc_pages(size_t count);        // Allocate contiguous virtual pages
void   vmm_free_pages(void* addr, size_t count);
void*  kmalloc(size_t size);
void   kfree(void* ptr);
int    vmm_map_range(uintptr_t virt, uintptr_t phys, size_t size, uint64_t flags);
int    vmm_unmap_range(uintptr_t virt, size_t size);
void   vmm_init(void);
```

---

## 5. Process Manager & Scheduler

### 5.1 Process Control Block (PCB)

```c
struct process {
    uint64_t pid;               // Process ID
    uint64_t parent_pid;        // Parent PID
    enum proc_state state;      // NEW, READY, RUNNING, BLOCKED, ZOMBIE
    uint8_t priority;           // 0–3 (lower = higher priority)
    uint64_t rbp, rsp;          // Kernel/user stack pointers
    uint64_t rip;               // Instruction pointer
    uint64_t cr3;               // Page table base (for ASLR-like isolation)
    struct vm_area_struct* mm;  // Memory descriptor
    struct file* files[128];    // File descriptor table (max 128 open files)
    uint64_t ticks;             // Total CPU time consumed
    struct list_node run_link;  // Link in run queue
    struct list_node wait_link;  // Link in wait queue
    void* kstack;               // Kernel stack (2 pages)
    // IPC state
    struct ipc_queue ipc_rx;    // Receive queue
    // Scheduling
    uint8_t queue_level;       // MLFQ queue level (0–3)
    uint64_t time_slice;        // Remaining ticks in current slice
    uint64_t total_ticks;       // Lifetime CPU ticks
};
```

### 5.2 Scheduler — Multi-Level Feedback Queue

- **Queues:** 4 priority levels (0=highest, 3=lowest)
- **Time slices:** Q0=4 ticks, Q1=8 ticks, Q2=16 ticks, Q3=32 ticks
- **Priority boost:** After waiting 1 second, move task up one queue level
- **Aging:** If a lower-priority process runs too long while higher-priority processes wait, demote the high-priority one
- **Per-CPU run queues:** Each CPU has its own run queue; lock-free for enqueue, per-CPU lock for dequeue
- **Idle task:** Each CPU has an idle task (PID 0) running when nothing else is runnable

### 5.3 Context Switch

- Save: RBP, RBX, R12, R13, R14, R15 (callee-saved) + RSP (via function return)
- Restore: Same registers
- **Target:** ~40-50 cycles (optimized assembly)
- **No FPU save/restore in v1** — kernel doesn't use floating point; add lazy FPU save/restore in v2

### 5.4 System Calls for Process Management

```c
int sys_fork(void);        // Clone current process
int sys_exec(const char* path, char** argv);  // Load and execute ELF binary
void sys_exit(int status); // Terminate current process
int sys_wait(int* status); // Wait for child process
int sys_yield(void);       // Voluntary yield
int sys_getpid(void);
int sys_getppid(void);
```

### 5.5 ELF Loader

- **Format:** ELF64 (little-endian x86_64)
- **Segments:** PT_LOAD — map segments into virtual address space
- **Sections:** .text, .rodata, .data, .bss — all zeroed
- **No dynamic linking** — static executables only in v1
- **Stack:** Setup initial stack with argc, argv, envp on user stack

---

## 6. IPC — Lock-Free Message Queues

### 6.1 Design

Each process has a receive queue (fixed-size ring buffer). Senders write into the receiver's queue.

```c
struct ipc_queue {
    uint64_t* buffer;      // Ring buffer (power of 2 size, e.g., 256 entries)
    uint64_t head;         // Written entries (atomic, lock-free)
    uint64_t tail;         // Read entries (atomic, lock-free)
    uint32_t capacity;     // Buffer capacity (must be power of 2)
    uint32_t msg_size;     // Size of each message (max 64 bytes for zero-copy)
};

// Zero-copy buffer for large messages (> 64 bytes)
struct ipc_msg {
    uint64_t size;
    uint64_t flags;          // MSG_COPY (copy), MSG_ZEROCOPY (shared page)
    void* data;              // Pointer to shared page or copied data
};
```

### 6.2 Operations

- **send(pid, msg, size):** Find receiver's queue, atomic write to ring buffer. If queue full, block sender.
- **recv(&msg):** Atomic read from local ring buffer. If empty, block receiver.
- **Both use `lock xadd` (x86 atomic)** — no kernel lock needed on fast path.

### 6.3 Signals

- Signal handlers registered via `signal(int signum, handler_t handler)`
- Delivered on return to userspace (check pending signal bitmap in `syscall` entry)
- Default actions: IGNORE, TERM, CORE, STOP, CONT

### 6.4 Semaphores

- Binary + counting semaphores
- Implemented as atomic operations on a counter: `wait` (decrement with block on zero), `post` (increment and wake one waiter)

### 6.5 API

```c
int ipc_send(uint64_t pid, const void* msg, size_t size);
int ipc_recv(void* buf, size_t max_size);
int ipc_reply(uint64_t pid, const void* msg, size_t size);
int signal(int signum, void (*handler)(int));
int semaphore_create(int initial_value);
int semaphore_wait(int sem);
int semaphore_post(int sem);
```

---

## 7. System Call Interface

### 7.1 Entry Point

- **x86_64 syscall instruction** — ~50 cycle overhead vs 300+ for INT
- **Syscall gate in IDT/LSTAR MSR** (`IA32_LSTAR` = 0xC0000082)
- **Argument passing:** RDI, RSI, RDX, R10, R8, R9 (System V AMD64 ABI)
- **Return:** RAX

### 7.2 Syscall Table

64 syscalls in v1:

| Number | Name | Description |
|---|---|---|
| 0 | read | Read from fd |
| 1 | write | Write to fd |
| 2 | open | Open file |
| 3 | close | Close fd |
| 4 | stat | File status |
| 5 | fstat | FD status |
| 6 | lseek | Seek in fd |
| 7 | mmap | Map memory |
| 8 | munmap | Unmap memory |
| 9 | mprotect | Set memory protection |
| 10 | brk | Set program break |
| 11 | fork | Clone process |
| 12 | execve | Execute program |
| 13 | waitpid | Wait for child |
| 14 | kill | Send signal |
| 15 | clone | Create thread/process |
| 16 | yield | Yield CPU |
| 17 | sleep | Sleep for N ms |
| 18 | getpid | Get PID |
| 19 | getppid | Get parent PID |
| 20 | getuid | Get UID |
| 21 | geteuid | Get effective UID |
| 22 | getgid | Get GID |
| 23 | getegid | Get effective GID |
| 24 | gettid | Get thread ID |
| 25 | dup | Duplicate fd |
| 26 | dup2 | Duplicate fd to fd |
| 27 | pipe | Create pipe |
| 28 | prctl | Process control |
| 29 | getcwd | Get working directory |
| 30 | mkdir | Create directory |
| 31 | rmdir | Remove directory |
| 32 | unlink | Remove file |
| 33 | link | Create hard link |
| 34 | rename | Rename file |
| 35 | chmod | Change mode |
| 36 | chown | Change owner |
| 37 | access | Check file access |
| 38 | sync | Sync filesystem |
| 39 | fsync | Sync fd |
| 40 | mount | Mount filesystem |
| 41 | umount | Unmount filesystem |
| 42 | socket | Create socket |
| 43 | connect | Connect socket |
| 44 | accept | Accept connection |
| 45 | send | Send to socket |
| 46 | recv | Receive from socket |
| 47 | sendto | Send to address |
| 48 | recvfrom | Receive from address |
| 49 | setsid | Create session |
| 50 | setpgid | Set process group |
| 51 | getpgid | Get process group |
| 52 | setsid | Get session ID |
| 53 | setuid | Set UID |
| 54 | setgid | Set GID |
| 55 | setreuid | Set real/effective UID |
| 56 | setregid | Set real/effective GID |
| 57 | getrlimit | Get resource limit |
| 58 | setrlimit | Set resource limit |
| 59 | gettimeofday | Get time |
| 60 | settimeofday | Set time |
| 61 | personality | Set personality |
| 62 | getdents | Get directory entries |
| 63 | exit | Exit process |

### 7.3 Syscall Dispatch

```c
// In syscall.S:
syscall_entry:
    // Save registers (no RAX, RCX, R11 — clobbered by syscall)
    push rbp; push rbx; push r12; push r13; push r14; push r15
    // Check stack alignment (16-byte for System V ABI)
    mov rdi, rax          // syscall number
    mov rsi, rcx          // arg1 (clobbered by syscall, but we used it)
    // Actually:
    // RDI=arg0, RSI=arg1, RDX=arg2, R10=arg3, R8=arg4, R9=arg5
    call [syscall_table + rax * 8]
    pop r15; pop r14; pop r13; pop r12; pop rbx; pop rbp
    sysret
```

---

## 8. VFS & Block Layer

### 8.1 VFS Core

- **Superblock:** Filesystem-level metadata (total blocks, free blocks, block size)
- **Inode:** Per-file metadata (size, blocks, mode, atime/mtime/ctime, operations)
- **Dentry:** Directory entry cache (name → inode mapping), per-directory
- **File:** Per-open-file descriptor (position, flags, inode reference)

### 8.2 VFS API

```c
int vfs_open(const char* path, int flags, mode_t mode, struct file** out);
int vfs_close(struct file* file);
ssize_t vfs_read(struct file* file, void* buf, size_t count);
ssize_t vfs_write(struct file* file, const void* buf, size_t count);
off_t   vfs_lseek(struct file* file, off_t offset, int whence);
int     vfs_stat(const char* path, struct stat* st);
int     vfs_mkdir(const char* path, mode_t mode);
int     vfs_unlink(const char* path);
int     vfs_mount(const char* device, const char* mount_point, const char* fs_type);
```

### 8.3 FAT32 Filesystem

- **Boot sector:** Parse BPB, verify media descriptor, read FSInfo
- **Directory entries:** Support 8.3 and long filename (VFAT) entries
- **Clusters:** Follow FAT chain for file data
- **Write support:** Allocate clusters, update FAT entries
- **Mount:** Register FAT32 as a filesystem type in VFS

### 8.4 Block Device Layer

- **Buffer cache:** LRU eviction, 4KB blocks, up to 256 cached blocks (1MB total)
- **Disk I/O:** PIO (Programmed I/O) — sufficient for QEMU; DMA in v2
- **Device abstraction:** `block_device_t` with `read(block, buf)`, `write(block, buf)` operations

---

## 9. Drivers

### 9.1 Serial (16550 UART)

- **Port:** COM1 (I/O port 0x3F8)
- **Baud:** 115200, 8N1
- **Usage:** QEMU `-serial stdio` for debug output
- **FIFO:** Enabled (16-byte TX/RX FIFO)
- **API:** `serial_write(char c)`, `serial_write_str(const char* s)`

### 9.2 VGA Text Mode

- **Mode:** 80×25, 16 colors
- **Memory:** 0xB8000 (color) — mapped to virtual address
- **API:** `vga_write(int x, int y, char c, uint8_t color)`, `vga_clear()`, `vga_init()`

### 9.3 Keyboard (PS/2)

- **IRQ:** IRQ1 (ISA, remapped to vector 33)
- **Scancode:** AT set 2 (scancode set 2)
- **Buffer:** Ring buffer of 32 keys; consumed by keyboard interrupt handler
- **API:** `keyboard_read()` — returns ASCII or 0 if empty; `keyboard_get_char()` — blocking

### 9.4 Timer

- **PIT:** Channel 0, IRQ0, rate generator (~100 Hz for scheduler tick)
- **APIC Timer:** Configured for per-CPU timer interrupts (higher resolution)
- **TSC:** Used for high-resolution time (`rdtsc`)
- **API:** `timer_get_ticks()`, `timer_sleep(uint32_t ms)`

---

## 10. Hardware Abstraction Layer (HAL)

### 10.1 GDT

- **Segments:** Flat model — CS, DS, SS all base=0, limit=0xFFFFFFFF (4GB)
- **Entries:** Null (0), Kernel CS (1), Kernel DS (2), User CS (3), User DS (4), TSS (5)
- **TSS:** Task State Segment for IST (Interrupt Stack Table) — used for double-fault, NMI, etc.

### 10.2 IDT

- **256 entries:** 0–31 (CPU exceptions), 32–47 (IRQs), 48–255 (syscall, APIC)
- **Trap gate:** For exceptions (error code pushed)
- **Interrupt gate:** For IRQs (clears IF flag)
- **Error codes:** CPU pushes error code for exceptions that have one (page fault, GPF, etc.)

### 10.3 PIC / APIC

- **Legacy PIC:** 8259A, IRQ0–15, remapped to vectors 32–47
- **LAPIC:** Enabled in x2APIC mode; each CPU has its own LAPIC
- **IOAPIC:** For routing ISA IRQs to LAPIC on SMP systems
- **IPI:** Used for TLB shootdowns, process migration, reschedule inter-processor interrupts (IPI)

### 10.4 SMP

- **BSP (Bootstrap Processor):** The CPU that boots first
- **APs (Application Processors):** Brought up via INIT/SIPI sequence from BSP
- **Per-CPU data:** Accessed via `percpu_read(var)` using GS segment base
- **Per-CPU run queues:** Each CPU has its own scheduler, no lock needed for enqueue

---

## 11. Boot Process

```
GRUB2 (multiboot2 compliant)
    │
    ▼ Stage 1: arch/x86_64/boot/start.asm
    - Enter from 16-bit real mode
    - Enable A20 line
    - Switch to 32-bit protected mode (GDT)
    - Jump to 32-bit kernel
    │
    ▼ Stage 2: arch/x86_64/boot/head.S (32-bit startup)
    - Detect CPU (long mode support check)
    - Build initial page tables (PML4, PDPT, PD, PT) — identity + high mapping
    - Enable PAE + paging
    - Switch to long mode (64-bit)
    - Jump to higher-half kernel
    │
    ▼ Stage 3: arch/x86_64/boot/head64.S (64-bit entry)
    - Setup GS base for per-CPU data
    - Clear BSS
    - Call kernel_main()
    │
    ▼ kernel/main.c: kernel_main()
    1. Parse multiboot2 info (memory map, boot device, cmdline)
    2. Initialize serial (early, for debug output)
    3. Initialize GDT (per-CPU)
    4. Initialize IDT
    5. Initialize PIC (legacy)
    6. Initialize LAPIC
    7. Initialize PIT timer
    8. Initialize PMM (buddy allocator)
    9. Initialize VMM (kmalloc, slab)
    10. Initialize VFS + FAT32
    11. Initialize block device (disk)
    12. Initialize keyboard
    13. Initialize scheduler (create PID 0 idle task)
    14. Create init process (PID 1)
    15. Create shell process (PID 2)
    16. Start scheduler → idle loop
```

---

## 12. Project Structure

```
kernel/
├── Makefile
├── scripts/
│   ├── build.sh
│   ├── run-qemu.sh
│   ├── debug.sh
│   └── build-cross.sh
├── arch/
│   └── x86_64/
│       ├── Makefile
│       ├── boot/
│       │   ├── start.asm          # 16→32→64 boot
│       │   ├── head64.S          # Long mode entry
│       │   └── multiboot2.h      # Multiboot2 constants
│       ├── gdt.c / gdt.h
│       ├── idt.c / idt.h
│       ├── isr.S                 # ISRs 0–31
│       ├── irq.S                 # IRQ 0–15 handlers
│       ├── pic.c / pic.h         # 8259 PIC
│       ├── apic.c / apic.h       # LAPIC + IOAPIC
│       ├── paging.c / paging.h  # 4-level paging
│       ├── tlb.c / tlb.h         # TLB ops + shootdown
│       ├── syscall.S             # Syscall entry/exit
│       ├── smp.c / smp.h         # AP bringup
│       ├── cpu.c / cpu.h         # CPU detection
│       ├── spinlock.h            # Per-CPU spinlocks
│       ├── percpu.h              # Per-CPU macros
│       └── asm_macros.h          # Inline asm helpers
├── kernel/
│   ├── main.c                    # kernel_main()
│   ├── klib/
│   │   ├── kstring.c / kstring.h
│   │   ├── kprintf.c / kprintf.h
│   │   └── list.c / list.h
│   ├── memory/
│   │   ├── pmm.c / pmm.h        # Buddy allocator
│   │   ├── vmm.c / vmm.h        # kmalloc, vmm
│   │   └── slab.c / slab.h      # Slab caches
│   ├── process/
│   │   ├── sched.c / sched.h    # MLFQ scheduler
│   │   ├── process.c / process.h
│   │   ├── context.S            # Context switch
│   │   └── elf.c / elf.h        # ELF64 loader
│   ├── ipc/
│   │   ├── ipc.c / ipc.h        # Lock-free queues
│   │   ├── signal.c / signal.h
│   │   └── semaphore.c / semaphore.h
│   ├── vfs/
│   │   ├── vfs.c / vfs.h
│   │   ├── fat32.c / fat32.h
│   │   ├── block.c / block.h
│   │   ├── dentry.c / dentry.h
│   │   ├── inode.c / inode.h
│   │   └── file.c / file.h
│   ├── drivers/
│   │   ├── serial.c / serial.h
│   │   ├── vga.c / vga.h
│   │   ├── keyboard.c / keyboard.h
│   │   └── timer.c / timer.h
│   └── sync/
│       ├── spinlock.c / spinlock.h
│       └── atomic.h
├── user/
│   ├── init.c                    # PID 1 — execs shell
│   ├── shell/
│   │   ├── main.c
│   │   └── commands.c
│   └── tests/
│       ├── test_pmm.c
│       ├── test_vmm.c
│       ├── test_sched.c
│       └── test_ipc.c
├── include/
│   ├── types.h
│   ├── stdarg.h
│   ├── stddef.h
│   ├── errno.h
│   ├── kernel.h
│   ├── multiboot2.h
│   ├── elf.h
│   └── queue.h
├── .gdbinit
├── docs/
│   ├── architecture.md
│   ├── syscalls.md
│   └── coding_style.md
└── GRUB/
    └── grub.cfg
```

---

## 13. Build System

### 13.1 Toolchain

```bash
# Build cross-compiler (if not available)
x86_64-elf-gcc --version  # or build via scripts/build-cross.sh
```

### 13.2 Compilation Flags

```
CFLAGS = -Wall -Wextra -O2 -ffreestanding -fno-pic -fno-stack-protector \
         -fno-asynchronous-unwind-tables -fno-ident -m64 -march=x86_64
ASFLAGS = -f elf64
LDFLAGS = -nostdlib -T kernel.ld --warn-common -z max-page-size=0x1000
```

### 13.3 Disk Image

```bash
# Create 100MB disk image with FAT32
dd if=/dev/zero of=disk.img bs=1M count=100
mkfs.vfat -F 32 disk.img
# Mount and copy kernel + init
# ...
```

### 13.4 Run

```bash
# Via QEMU
qemu-system-x86_64 -kernel kernel.bin -hda disk.img \
    -serial stdio -smp 2 -m 256M

# With GRUB iso
qemu-system-x86_64 -cdrom os.iso -serial stdio -smp 2 -m 256M

# With GDB
qemu-system-x86_64 -kernel kernel.elf -hda disk.img \
    -serial stdio -smp 2 -m 256M -s -S
# Then: gdb -x .gdbinit kernel.elf
```

---

## 14. Testing Strategy

### 14.1 Boot Testing (Phase 1)
- QEMU starts, kernel reaches serial output
- VGA shows kernel messages
- Memory map printed to serial

### 14.2 Unit Tests (Phase 2)
- PMM: Allocate all memory in order-0 blocks, free, verify no corruption
- VMM: kmalloc/kfree 100K times, verify allocation patterns
- Buddy: Stress test with random alloc/free patterns

### 14.3 System Tests (Phase 3)
- `cat /proc/meminfo` → correct total/free memory
- `ls` from FAT32 disk
- `fork` + `exec` → child process runs and exits
- IPC: send/recv between parent and child
- Scheduler: `ps` shows multiple processes with CPU time

### 14.4 QEMU / Benchmark (Phase 4)
- 2-core SMP boot
- Scheduler balance across CPUs
- Syscall throughput: count syscalls per second (target: 500K+/sec)

---

## 15. Acceptance Criteria

1. **Boot:** GRUB loads kernel, serial output shows boot messages
2. **PMM:** Buddy allocator functional with correct memory accounting
3. **VMM:** kmalloc/vmm_alloc work correctly under stress
4. **Scheduler:** Process switching visible, per-CPU run queues operational
5. **IPC:** Lock-free message passing works between processes
6. **VFS:** FAT32 read/write works; `ls` and `cat` functional
7. **Syscalls:** 30+ syscalls implemented and testable from shell
8. **SMP:** Kernel boots on 2 CPUs with per-CPU scheduling
9. **Performance:** Syscall overhead < 100 cycles, context switch < 60 cycles

---

## 16. Scope Boundaries

**In scope:**
- All items in this spec, implemented to working quality

**Out of scope (v1):**
- Networking (TCP/IP stack)
- Dynamic linking / shared libraries
- Filesystems other than FAT32
- Virtual filesystems (/proc, /dev)
- Swap / demand paging (basic COW fork only)
- User-space dynamic memory (libc, malloc) — programs get a static heap via brk()
- SMP load balancing (tasks stay on the CPU they were scheduled on)
- Filesystem journaling
- Userspace threading (kernel threads only)

---

## 17. Phases

| Phase | Deliverable | Subsystems |
|---|---|---|
| **0** | Build setup, cross-compiler, GRUB disk | Makefile, build scripts, disk image |
| **1** | Boot to serial output | boot, GDT, IDT, serial, VGA |
| **2** | PMM + VMM functional | PMM (buddy), VMM, slab, heap |
| **3** | Scheduler + processes + syscalls | Scheduler, process, ELF loader, syscall table |
| **4** | IPC + signals | Lock-free queues, semaphores, signals |
| **5** | VFS + FAT32 + block layer | VFS, FAT32, block device |
| **6** | Keyboard + shell + init | Keyboard driver, shell, init process |
| **7** | SMP + APIC + timer | LAPIC, SMP bringup, APIC timer |
| **8** | Testing + benchmarks | Test programs, QEMU integration |

---

*Spec version: 1.0 — 2026-07-15*