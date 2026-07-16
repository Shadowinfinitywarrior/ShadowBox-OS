# Kernel Design Specification

**Project:** Monolithic x86_64 Kernel (Linux-like)
**Date:** 2026-07-16
**Architecture:** x86_64 (x86-64)
**Language:** C (with inline assembly)
**Build System:** GNU Binutils (GCC + ld), Make
**Runtime Target:** QEMU (primary), real hardware via USB/ISO

---

## 1. Overview

A minimal but functional monolithic kernel for x86_64 that implements the core subsystems found in Linux: bootloader integration, memory management, process scheduling, virtual filesystem, basic device drivers, and a shell. The kernel is developed in phases, each producing a runnable, testable system. The design mirrors Linux's architecture: a single address space where kernel and user processes coexist, with system calls as the boundary.

---

## 2. Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   User Space                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────┐ │
│  │   Shell      │  │  User Proc   │  │  Utils   │ │
│  └──────┬───────┘  └──────┬───────┘  └────┬─────┘ │
│         │                 │                │        │
│         └──────────┬───────┴────────────────┘        │
│                    ▼                                  │
│            System Call Interface                     │
├─────────────────────────────────────────────────────┤
│                   Kernel Space                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐ │
│  │  VFS     │  │ Process  │  │  Device Drivers  │ │
│  │  Layer   │  │ Scheduler│  │  (Keyboard, VGA) │ │
│  └──────────┘  └──────────┘  └──────────────────┘ │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐ │
│  │  Memory  │  │  Block    │  │   Interrupt      │ │
│  │  Manager │  │  Device   │  │   Controller     │ │
│  └──────────┘  └──────────┘  └──────────────────┘ │
├─────────────────────────────────────────────────────┤
│                   Hardware Layer                     │
│        CPU (x86_64) │ RAM │ PIC │ PIT │ I/O        │
└─────────────────────────────────────────────────────┘
```

---

## 3. Phases

### Phase 1: Boot & Basics (Weeks 1-2)

**3.1.1 Multiboot2 Bootloader Integration**
- Support Multiboot2 spec (GRUB2 will load the kernel)
- Header at 8-byte aligned offset in the first 32KB
- Kernel entry point: `_start` in `arch/x86_64/boot/head.S`
- Jump to C kernel entry after verifying magic number

**3.1.2 GDT Setup**
- 4-entry GDT: null, kernel code (0x08), kernel data (0x10), user code (0x18), user data (0x20)
- Load GDT with `lgdt` instruction, reload segment registers
- Enable protected mode → long mode transition

**3.1.3 Long Mode (x86_64) Setup**
- Enable PAE (Physical Address Extension)
- Set up 4-level paging (PML4 → PDP → PD → PT)
- Identity-map low 1MB and kernel higher half
- Enable long mode via EFER MSR, then `jmp far` to 64-bit code

**3.1.4 Serial Port & Debug Output**
- UART 16550 on COM1 (I/O port 0x3F8)
- `printk()` function writing to serial + VGA text buffer
- VGA text mode: 80x25, 16-color, base at 0xB8000

**3.1.5 Interrupt Descriptor Table (IDT)**
- 256-entry IDT (64-bit interrupt gates)
- Remap PIC (IRQ 0-15 → 0x20-0x2F)
- Handle: divide error (0), debug (1), page fault (14), timer (32), keyboard (33)
- ISR stubs in assembly, C handlers dispatched via table

**3.1.6 Physical Memory Manager**
- Detect memory via Multiboot2 multiboot memory map
- Bitmap-based allocator: one bit per 4KB page frame
- `pmm_alloc_page()` / `pmm_free_page()`
- Initialize with usable memory regions from mmap

**3.1.7 Virtual Memory Manager**
- Kernel virtual address space: linear mapping of all physical memory + vmalloc region
- `vmm_map_page()` using 4-level paging, `vmm_unmap_page()`
- `vmalloc()` for large kernel allocations (page slab allocator)
- `kmalloc()` / `kfree()` for small objects (slab allocator, power-of-2 caches)

---

### Phase 2: Process & System Calls (Weeks 3-4)

**3.2.1 Process Descriptor**
- `struct task_struct`: PID, state (running/ready/blocked/zombie), kernel stack pointer, register snapshot, parent pointer, file descriptor table pointer, memory map
- `current` macro using per-CPU pointer (FS register on x86_64)
- Process 0 (idle) and process 1 (init) created at boot

**3.2.2 Scheduler**
- Preemptive round-robin scheduler
- Timer interrupt (IRQ 0) triggers schedule() on each tick
- `schedule()` picks next runnable task, switches context via `switch_to()`
- Process states: TASK_RUNNING, TASK_READY, TASK_BLOCKED, TASK_ZOMBIE

**3.2.3 Context Switching**
- `switch_to(prev, next)` in assembly: save/restore callee-saved registers (RBX, R12-R15, RBP, RSP)
- Save/restore RIP by pushing/popping return address
- Switch kernel stack pointer (RSP) between tasks

**3.2.4 System Call Interface**
- Syscall instruction (fast syscall on AMD), NR syscall in RAX, args in RDI, RSI, RDX
- `sys_call_table[]` array of function pointers, indexed by syscall number
- Initial syscalls: read, write, exit, fork, execve, open, close, brk, mmap

**3.2.5 ELF Loading (Minimal)**
- Parse ELF64 header (no sections, load segments PT_LOAD)
- Allocate user virtual memory, map pages
- Set up user stack, RIP, RSP, RFLAGS
- `iretq` to user mode

---

### Phase 3: VFS & Initrd (Weeks 5-6)

**3.3.1 Virtual Filesystem (VFS)**
- `struct file_operations`: open, read, write, close, ioctl, lseek
- `struct inode_operations`: create, lookup, mkdir, unlink
- Common VFS layer: `vfs_open()`, `vfs_read()`, `vfs_write()`, `vfs_close()`
- Path resolution: walk dentry tree, mount points

**3.3.2 FAT16 Filesystem Driver**
- BIOS Parameter Block parsing, directory entry parsing
- Read-only support initially (no write/flush)
- Cluster-based disk I/O

**3.3.3 Block Device Layer**
- Generic block request queue (no elevator initially)
- IDE/PATA driver using I/O ports 0x1F0-0x1F7
- Async read/write with polling (no DMA initially)

**3.3.4 Initramfs (Embedded Archive)**
- Embedded CPIO archive linked into kernel
- Extracted to a ramdisk (tmpfs-like tmpfs) at boot
- Contains /etc, /bin, /lib directories with shell and utilities

**3.3.5 Devtmpfs (Basic)**
- Create /dev directory in initramfs
- Static device nodes: /dev/null, /dev/zero, /dev/tty, /dev/fb0

---

### Phase 4: Shell & Polish (Weeks 7-8)

**3.4.1 Shell**
- Command parser: split by whitespace, handle arguments
- Built-in commands: ls, cat, echo, cd, pwd, mkdir, rmdir, ps, kill, help, reboot
- PATH variable, argument expansion
- Basic I/O redirection (>, >>, <)
- Background execution (&)

**3.4.2 Process Forking**
- `fork()` syscall: clone current task_struct, copy page tables (COW)
- `execve()`: replace address space with new ELF binary, load from VFS
- `wait()`: parent blocks until child exits
- `exit()`: set TASK_ZOMBIE, wake parent, schedule next process

**3.4.3 Keyboard Driver**
- PS/2 keyboard: read scan codes from I/O port 0x60
- PS/2 controller at I/O port 0x64
- Map scancodes to ASCII, buffer in circular ring buffer
- INT 0x09 interrupt handler

**3.4.4 Framebuffer Console**
- VBE (VESA BIOS Extensions) for graphics mode detection
- Linear framebuffer at address returned by VBE
- Bitmapped font rendering (8x16 or 8x8 monospace)
- Scroll support, multiple virtual terminals (TTY 1-6)

**3.4.5 Basic Signals**
- SIGTERM, SIGKILL, SIGCHLD, SIGINT (Ctrl+C)
- Signal delivery on return to userspace

---

## 4. Directory Structure

```
os/
├── Makefile                          # Top-level build
├── link.ld                           # Linker script
├── docs/
│   └── SUPERPOWERS/specs/
├── arch/
│   └── x86_64/
│       ├── boot/
│       │   ├── head.S                # Entry point, GDT, long mode jump
│       │   ├── idt.S                 # IDT, ISR stubs
│       │   └── gdt.h                 # GDT structures
│       ├── mm/
│       │   ├── pmm.c                  # Physical memory manager
│       │   ├── vmm.c                  # Virtual memory manager
│       │   └── slab.c                 # Slab allocator
│       ├── proc/
│       │   ├── sched.c                # Scheduler
│       │   ├── context.S              # Context switch
│       │   └── task.c                 # Process management
│       ├── syscall/
│       │   ├── syscall.c              # Syscall entry
│       │   └── syscalls.c             # Syscall implementations
│       └── drivers/
│           ├── serial.c               # UART 16550
│           ├── keyboard.c             # PS/2 keyboard
│           ├── vga.c                  # VGA text mode
│           ├── fb.c                   # Framebuffer console
│           ├── pit.c                  # Programmable Interval Timer
│           ├── pic.c                  # Programmable Interrupt Controller
│           └── ide.c                  # IDE/PATA driver
├── kernel/
│   ├── main.c                        # C entry point
│   ├── printk.c                      # Debug output
│   └── panic.c                      # Kernel panic
├── vfs/
│   ├── vfs.c                         # VFS layer
│   ├── fat16.c                       # FAT16 driver
│   ├── initrd.c                      # Initramfs extractor
│   └── dentry.c                      # Dentry cache
├── fs/
│   └── devfs.c                       # Device filesystem
├── lib/
│   ├── string.c                      # String utilities
│   ├── bitmap.c                       # Bitmap operations
│   └── sort.c                        # Utility functions
├── include/
│   ├── types.h                       # Standard types
│   ├── kernel.h                      # Kernel-wide declarations
│   ├── const.h                       # Constants
│   ├── memory.h                      # Memory management API
│   ├── process.h                     # Process API
│   ├── vfs.h                         # VFS API
│   └── syscall.h                     # Syscall numbers
├── init/
│   ├── init.c                        # Kernel init (PID 1)
│   └── ramdisk.c                     # Initramfs mounting
├── user/
│   ├── shell/
│   │   ├── main.c                    # Shell entry
│   │   ├── parser.c                  # Command parser
│   │   └── builtin.c                 # Built-in commands
│   └── init/
│       └── init.c                    # PID 1 init process
└── tests/
    ├── test_pmm.c                    # Physical memory tests
    ├── test_vmm.c                    # Virtual memory tests
    ├── test_vfs.c                    # VFS tests
    └── test_sched.c                  # Scheduler tests
```

---

## 5. Build System

**Makefile targets:**
- `make` / `make all` — compile all kernel and user space, link into `os.bin`
- `make kernel` — compile kernel only
- `make user` — compile user space programs
- `make clean` — remove build artifacts
- `make test` — run QEMU with kernel (for automated testing)
- `make debug` — run QEMU with GDB stub on port 1234

**Toolchain:**
- GCC for C (cross-compiler: `x86_64-elf-gcc`)
- GNU ld for linking
- GRUB2 (via `grub-mkrescue`) for ISO generation
- QEMU with `-kernel` or ISO boot

**Linker script (`link.ld`):**
- Output format: `elf64-x86-64` (with conversion to flat binary for QEMU `-kernel`)
- Sections: .text, .rodata, .data, .bss
- Kernel base address: `0xFFFFFFFF80000000` (higher half)
- Bootstrap GDT, IDT, initial stack in low memory

**Cross-compilation:**
- Use `x86_64-elf-gcc` to avoid glibc dependency
- No libc in kernel — implement minimal `libc` equivalent for user space

---

## 6. Testing Strategy

### Unit Testing (Inline)
- Each subsystem has `test_*.c` compiled into the kernel image
- `make test` builds with `DEBUG=1`, runs test functions before scheduler starts
- Failures print to serial/VGA, then halt

### Integration Testing (QEMU)
- `make run` launches QEMU with the kernel binary
- Serial port output captured to host stdout
- Automated shell-based tests via expect scripts

### CI (Future)
- GitHub Actions: compile kernel, run QEMU, check serial output for expected strings

---

## 7. Memory Layout (x86_64)

```
0xFFFF800000000000  ┌─────────────────────┐  Kernel canonical addresses
                    │   Kernel direct     │
                    │   mapping (512TB)   │
                    │                     │
                    │   vmalloc region    │
0xFFFF000000000000  ├─────────────────────┤
                    │   Guard gap          │
                    │                     │
0x00007FFFFFFFFFFF  ├─────────────────────┤  User space (128TB)
                    │   User processes     │
                    │   0x0000000000400000 │
                    │   User stack         │
0x0000000000001000  ├─────────────────────┤
                    │   Null page guard    │
0x0000000000000000  └─────────────────────┘
```

---

## 8. Syscall Numbers

| Number | Name       | Description              |
|--------|------------|--------------------------|
| 0      | read       | Read from fd             |
| 1      | write      | Write to fd              |
| 2      | open       | Open file                |
| 3      | close      | Close fd                 |
| 4      | fork       | Clone process            |
| 5      | execve     | Execute program          |
| 6      | exit       | Terminate process        |
| 7      | wait       | Wait for child           |
| 8      | brk        | Change data segment size |
| 9      | mmap       | Map memory               |
| 10     | munmap     | Unmap memory             |
| 11     | kill       | Send signal              |
| 12     | getpid     | Get process ID           |
| 13     | getppid    | Get parent process ID    |
| 14     | getcwd     | Get current directory    |
| 15     | chdir      | Change directory         |
| 16     | mkdir      | Create directory         |
| 17     | rmdir      | Remove directory        |
| 18     | unlink     | Remove file              |
| 19     | dup        | Duplicate fd             |
| 20     | dup2       | Duplicate fd to specific |

---

## 9. Error Handling

- **Kernel panic:** unrecoverable errors halt the system with a stack trace printed to VGA + serial
- **Page faults:** dump faulting address, error code, and registers, then panic
- **NULL dereference:** caught by page table (guard page at 0x0)
- **Stack overflow:** guard page below kernel stacks catches this
- **ASSERT() macro:** `printk(FATAL)` and halt if debug build

---

## 10. Dependencies & External References

- **Multiboot2 spec:** https://www.gnu.org/software/grub/manual/multiboot2/
- **OSDev wiki:** https://wiki.osdev.org
- **AMD64 manual (Vol. 2 & 3):** AMD system programming manuals
- **Limine bootloader:** alternative to GRUB for simpler boot
- **Fonts:** 8x16 PC Screen Font (PSF) from Linux kernel source

---

## 11. Success Criteria

Phase 1: Kernel boots in QEMU, prints to VGA/serial, memory allocation works
Phase 2: Kernel schedules at least 2 processes, syscalls work from user space
Phase 3: Kernel mounts a filesystem, reads files from initramfs
Phase 4: Interactive shell with at least 5 working commands, processes can fork and exec

---

## 12. Scope Boundaries

**In scope:**
- Core kernel subsystems as described above
- Build system with cross-compiler
- QEMU testing and debugging setup

**Out of scope (not implemented):**
- Networking (TCP/IP stack)
- SMP (multi-core support)
- Filesystem write support
- Userspace libc (minimal stubs only)
- Graphics GUI (framebuffer shell only)
- Modular loadable drivers (LKM)
- Swap / virtual memory overcommit

---

*This spec is a living document. Phases may be adjusted as implementation reveals hidden complexity.*