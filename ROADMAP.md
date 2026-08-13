# ShadowBox OS — Roadmap

## Completed
- [x] Custom x86_64 kernel with syscall interface (`syscall.h` / `syscall.c`)
- [x] Unified input ring buffer (`include/input.h`, `kernel/input.c`)
- [x] PS/2 mouse + keyboard drivers (`arch/x86_64/drivers/mouse.c`, `keyboard.c`)
- [x] HID keyboard subsystem with scancode translation (`kernel/hid_kbd.c`)
- [x] Trackpad gesture engine (`kernel/trackpad.c`)
- [x] `/dev/input` VFS bridge (`fs/devfs.c`)
- [x] Framebuffer mmap + info syscalls
- [x] Interactive GUI desktop (`userland/desktop.c`)
  - Window manager: drag, close, z-order swap, taskbar
  - Start Menu with app launching
  - Built-in apps: Terminal, File Explorer, System Monitor, Image Viewer, Snake
- [x] Input protocol alignment between kernel and userland (`input_event_t`)

## In Progress
- [ ] **GUI polish:** window open/close animation, rendered system cursor, TextBox keyboard input
- [ ] **Multitask launcher:** Start Menu items spawn new desktop instances via `clone`/`morph`
- [ ] **QEMU validation:** `make run` smoke test (mouse, keyboard, windowing)

## Next Up
- [ ] C++ compositor layer (`gui/cpp/Compositor.cpp`, `Window.cpp`)
- [ ] Text input framework (TextBox + IME hooks)
- [ ] Theming / wallpaper API
- [ ] Networking stack (TCP/IP, sockets)
- [ ] Process groups / session manager
