# ShadowBox OS

A from-scratch x86_64 hobby operating system with a custom kernel, graphical desktop, and unified input subsystem — bootable on QEMU via a GRUB multiboot2 ISO.

```
ShadowBox v973cde5-dirty booting on x86_64...
```

## Features

- **Custom x86_64 kernel** (no Linux/BSD code): own syscall interface, ELF loader, and initrd (tarfs)
- **Memory management**: PMM with zones, VMM with ASLR, buddy allocator, slab cache, kmalloc, swap, huge pages
- **Scheduling & IPC**: preemptive multitasking, kernel threads, micro-IPC, signals, futexes, `clone`/`morph` process creation
- **Filesystems**: VFS layer with tarfs (initrd), devfs, tmpfs, ext2, ext4, fat32
- **Device drivers**: PIC/PIT/APIC/ACPI, serial, PS/2 keyboard + mouse, PCI, AHCI SATA, USB (EHCI/XHCI), HDA audio, framebuffer, RTC, RTL8139/E1000 NIC
- **HAL layer** (`kernel/hal/`): CPU, RAM, storage, and peripheral abstractions
- **Unified input pipeline**: PS/2 / HID / trackpad → shared ring buffer → `/dev/input` → userland
- **Graphical desktop** (`userland/desktop.c`): window manager with drag, z-order, taskbar, scrollable start menu, and **18 built-in apps**: Terminal, File Explorer, System Monitor, Image Viewer, Calculator, Text Editor (saves to `/tmp/editor.txt`), Paint, Process Monitor, Hex Viewer, Snake, Tetris, 2048, Pong, Matrix Rain, Mandelbrot fractal viewer, Clock, Fortune, About — plus Shutdown (menu only; apps also launch via `Ctrl+Alt+<key>`: `t f m v c e p n x y g b k o w z`)
- **Userland CLI**: shell plus ~20 core utilities (ls, cat, calc, matrix, fortune, ...)

## Repository layout

```
arch/x86_64/     boot, kernel (gdt/idt/smp/syscalls), mm, drivers
kernel/          main, boot, vfs, syscall, sched, task, ipc, input, hal, ...
fs/              tarfs, devfs, tmpfs, ext2, ext4, fat32
drivers/         USB EHCI and other peripheral drivers
include/         public headers (kernel + syscall ABI)
userland/        init, shell, desktop, apps, input/wm services
gui/             C / C++ widget toolkit (Button, Label, TextBox, Compositor...)
init/            init & service bootstrap
docs/            building guide, design specs
```

## Building

Requires an x86_64 cross toolchain (`x86_64-linux-gnu-gcc`), `grub-mkrescue`, and `mke2fs` (for the test disk image).

```bash
make          # builds os.bin, initrd.tar, ahci_disk.img, os.iso
make run      # boot in QEMU (GUI window, AHCI disk, USB, HDA audio)
make run-nox  # same, headless (serial console only)
make debug    # QEMU with -s -S, ready for a debugger
make clean    # remove build objects
```

`os.iso` boots via GRUB multiboot2 with `initrd.tar` as the module.

## Input subsystem

Mouse and keyboard events flow through a single kernel ring buffer:

```
PS/2 / HID / trackpad drivers
        ↓ input_event_t (type 0=kbd, 1=key up, 2=mouse move, 3=mouse btn/wheel)
kernel/input.c ring buffer
        ↓ /dev/input (devfs)
userland desktop & services (sb_pull)
```

The desktop renders the cursor at a fixed ≤100 Hz frame rate (independent of PS/2 event bursts), which keeps movement smooth even under high injection rates.

All key presses (including arrows / function keys, not just printable chars) are forwarded to `/dev/input`, and the desktop tracks live key state so apps support:

- **Arrow keys** everywhere: editor cursor navigation (`Home`/`End`/`PgUp`/`PgDn`/`Del` too), terminal cursor movement, Snake / Tetris / 2048 / Hex Viewer / Fortune controls
- **Pong**: hold `W`/`S` or arrows to steer continuously
- **Real-time updates**: game ticks run every frame regardless of input activity; clock, taskbar clock, system monitor and blinking cursors refresh on a 10 Hz idle timer

## Docs

- [Building](docs/building.md)
- [Roadmap](ROADMAP.md)
- [Desktop design spec](docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md)
- [Phase 1 foundation plan](docs/superpowers/plans/2026-07-18-shadowbox-phase1-foundation.md)

## Knowledge graph

`graphify-out/` contains an interactive knowledge graph of the codebase:

- `graph.html` — interactive graph, open in any browser
- `GRAPH_REPORT.md` — community breakdown, god nodes, suggested questions
- `graph.json` — raw graph data (nodes, edges)
