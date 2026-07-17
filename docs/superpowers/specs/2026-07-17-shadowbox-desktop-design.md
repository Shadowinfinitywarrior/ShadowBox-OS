# ShadowBox Desktop — Design Specification

**Date:** 2026-07-17
**Version:** 1.0
**Status:** Approved

---

## 1. Overview

ShadowBox Desktop is a modern, minimal, userland-based desktop environment for the ShadowBox OS kernel. It runs as a dedicated user-space process (`desktop.elf`) that owns the framebuffer, input devices, and window management. All GUI applications are separate ELF processes communicating with the DE server via a socket-based protocol.

**Core goals:**
- Modern minimal aesthetic (flat design, soft shadows, rounded corners)
- Full compositing window manager with transparency, shadows, and animations
- Complete file management (explorer with copy/cut/paste, virtual FS browsing, file associations)
- Rich app suite (terminal, editor, file explorer, image viewer, calculator, clock, notes, paint, settings)
- Robust input (mouse, keyboard, touch/pointer-friendly, vim-like shortcuts)
- System integration (process manager, notifications, settings panel, taskbar)
- Wide hardware support (x86_64, 512MB+ RAM, BIOS/UEFI, QEMU optimized)
- Scalable font system (bitmap, BDF, TTF, UTF-8, i18n)

---

## 2. Architecture

### 2.1 Process Model

```
┌─────────────────────────────────────────────────────┐
│                    Kernel (ShadowBox)               │
│  - pmm, vmm, scheduler, VFS, ext2, IPC, syscalls    │
│  - ELF loader, socket API, framebuffer driver       │
└─────────────────────────────────────────────────────┘
          │ syscalls              │ syscalls
          ▼                      ▼
┌──────────────────┐    ┌─────────────────────────────┐
│  desktop.elf     │    │  app.elf (terminal)         │
│  ─────────────   │    │  app.elf (file explorer)    │
│  Compositor      │◄───│  app.elf (text editor)       │
│  Window Manager  │    │  app.elf (image viewer)     │
│  App Launcher    │    │  app.elf (...)              │
│  Taskbar         │    └─────────────────────────────┘
│  System Tray     │              ▲
│  Notification    │    socket / domain protocol
│  Settings        │              │
└──────────────────┘──────────────┘
          ▲
          │ fb_init(), keyboard, mouse (via syscalls)
          │ framebuffer mmap, input events
          ▼
┌──────────────────────────────────────────────────────┐
│              /dev/fb0, /dev/input/*                  │
│              Kernel framebuffer + input drivers      │
└──────────────────────────────────────────────────────┘
```

### 2.2 DE Server Components

| Component | Responsibility |
|---|---|
| **Compositor** | Double-buffered rendering, blends window buffers to screen with dirty-rect optimization |
| **Window Manager** | Window lifecycle, positioning, Z-order, workspaces, tiling, focus |
| **Input Router** | Captures keyboard/mouse events, dispatches to focused window |
| **App Launcher** | App registry, launch/terminate, process supervision |
| **Taskbar** | Open window list, start menu, system tray, clock |
| **Notification Manager** | Notification queue, bubbles, action handling |
| **Settings Backend** | Config persistence, theme, input preferences |
| **File Associations** | MIME type registry, default handlers per extension |

### 2.3 App-to-DE Protocol

Applications communicate with the DE server over a domain socket. Each app process has a bidirectional socket connection.

**Message types:**

| Message | Direction | Description |
|---|---|---|
| `REGISTER_WINDOW` | app→de | Announce new window with title, size, position |
| `UPDATE_WINDOW_CONTENT` | app→de | Send raw pixel buffer for window content |
| `SET_WINDOW_TITLE` | app→de | Update window title |
| `CLOSE_WINDOW` | app→de | Request window close |
| `NOTIFICATION` | app→de | Push a notification |
| `INPUT_EVENT` | de→app | Key/mouse events for focused app |
| `WINDOW_RESIZED` | de→app | Window size changed by WM |
| `APP_LAUNCHED` | de→app | A new app was started |
| `APP_QUIT` | de→app | DE signals app to exit |

The DE server exposes a simple API via syscalls for mapping the shared framebuffer region and receiving input events. This keeps the protocol clean and leverages the existing `socket.c` infrastructure.

### 2.4 Startup Sequence

1. Kernel boots, starts `init` shell process
2. `init` launches `desktop.elf` as the DE server
3. `desktop.elf` mmaps the kernel framebuffer, sets up input event handlers
4. `desktop.elf` starts the compositor thread and window manager
5. DE shows login/desktop with app launcher, taskbar
6. User launches apps via launcher or file explorer — each app is a new ELF process
7. Apps connect to DE via socket, register their windows
8. Compositor blends all window buffers to the screen at 60Hz

---

## 3. Graphics Stack

### 3.1 Compositor

- **Double-buffering:** Each window has an offscreen RGBA buffer. The compositor maintains a back buffer for the full screen. After each frame, the back buffer is copied/flipped to the physical framebuffer.
- **Dirty rect tracking:** Only regions marked dirty are re-rendered. Windows report their dirty rectangles to the compositor.
- **Blending:** Standard alpha compositing (Porter-Duff OVER operator). Windows with alpha channels blend naturally.
- **Shadows:** Drop shadows rendered per-window using a 9-patch technique. Shadow parameters (blur, offset, color) configurable per window type.
- **V-sync:** Compositor aims for 60fps, can sync to display refresh rate if available.

### 3.2 Rendering Pipeline

```
App process (window buffer in shared memory) ──socket通知──► DE compositor ──blend──► Back buffer ──flip──► Screen
                                   │
                                   └── App marks dirty rects: {x, y, w, h} via shared memory flag
```

Each app window buffer lives in a shared memory region (mmap'd by both the app and the DE). Apps write pixel data directly to their buffer; the compositor reads it without any memcpy. The DE allocates shared memory pages per window and hands the app a file descriptor via the socket protocol.

### 3.3 Coordinate System

- Origin (0, 0) at top-left
- Window coordinates are relative to the desktop
- Screen coordinates are absolute
- Workspace coordinates match screen coordinates

---

## 4. Window Manager

### 4.1 Window Lifecycle

1. App sends `REGISTER_WINDOW` to DE with title, initial size/position, flags
2. DE allocates a window buffer, assigns a window ID
3. DE adds window to its list, renders it (possibly minimized)
4. App sends pixel data via `UPDATE_WINDOW_CONTENT`
5. DE composites the window
6. App sends `CLOSE_WINDOW` or DE destroys window on process exit

### 4.2 Window Flags

| Flag | Meaning |
|---|---|
| `RESIZABLE` | Can be resized from any edge/corner |
| `MINIMIZABLE` | Can be minimized to taskbar |
| `MAXIMIZABLE` | Can be maximized |
| `DECORATED` | Has title bar + controls |
| `TRANSPARENT` | Supports alpha blending |
| `FLOATING` | Always on top (like a dialog) |

### 4.3 Dragging

Click on title bar starts drag. Offset from mouse to window origin is recorded. On mouse move, window position updates immediately. No redraw of background needed during drag — compositor just moves the window buffer.

### 4.4 Resizing

Hover over any edge (4 edges + 4 corners) to show resize cursor. Drag resizes from that edge. Minimum window size enforced (e.g., 200x150). Resize events sent to app via `WINDOW_RESIZED`.

### 4.5 Minimizing

Click minimize button → window buffer is hidden, taskbar entry becomes active. Click taskbar entry → window restored with previous position/size.

### 4.6 Maximizing / Restoring

Click maximize button → window expands to fill current workspace. Second click → restores to previous position/size.

### 4.7 Z-Order

Active window is always on top. Clicking a window brings it to front. Floating windows (dialogs) always sit above normal windows.

### 4.8 Tiling / Window Snapping

- Drag window to left edge → snaps to left half of screen
- Drag window to right edge → snaps to right half
- Drag window to top edge → maximizes
- Drag window to top-left corner → top-left quarter
- Drag window to top-right corner → top-right quarter
- Keyboard shortcuts: `Super+Left` (tile left), `Super+Right` (tile right), `Super+Up` (maximize), `Super+Down` (restore)

### 4.9 Virtual Workspaces

- 4 virtual workspaces (Super+1..4 to switch)
- Each workspace has its own window list and Z-order
- Taskbar shows windows from all workspaces but dims those on other workspaces
- Workspace switch animates — windows slide out/in

---

## 5. Taskbar

### 5.1 Layout

```
[Start] [win1] [win2] [win3] ...           [systray] [clock]
  48px wide    window buttons (128px each)     right-aligned
```

### 5.2 Start Menu

Clicking Start button opens an application launcher menu:

```
┌──────────────────────────────────┐
│  🖥 Applications                  │
│  ────────────────────────────────│
│  📁 File Explorer                │
│  🖮 Terminal                      │
│  📝 Text Editor                  │
│  🖼 Image Viewer                  │
│  🔢 Calculator                   │
│  🕐 Clock                        │
│  📓 Notes                        │
│  🎨 Paint                        │
│  ⚙ Settings                      │
│  📊 System Monitor               │
│  ────────────────────────────────│
│  🖥 Desktop                      │
│  ⏻ Shut Down                    │
└──────────────────────────────────┘
```

### 5.3 System Tray

Right side of taskbar:
- **Network icon** — shows connection status
- **Volume icon** — placeholder (audio not yet implemented)
- **Battery icon** — placeholder (ACPI battery reads can be wired)
- Click any tray icon to toggle its popup/status panel

### 5.4 Clock

Right-most area of taskbar shows time in `HH:MM` format, updates every minute. Click to open the Clock app.

---

## 6. Font System

### 6.1 Font Loading

Fonts are loaded in order of priority:

1. **Embedded bitmap font** (existing 8x8 font, always available)
2. **BDF fonts** from initrd (`/fonts/*.bdf`) — parsed at boot, glyph bitmaps cached
3. **TTF fonts** from initrd (`/fonts/*.ttf`) — minimal rasterizer generates glyph bitmaps

### 6.2 Font Rendering Pipeline

```
Font file (BDF/TTF) → Parser → Glyph outline →
  Rasterizer (bi-level or anti-aliased) →
  Glyph bitmap → LRU cache → Render to window buffer
```

### 6.3 Glyph Cache

- Fixed-size LRU cache (configurable, default 256 entries)
- Cache key: (font_id, font_size, codepoint)
- Eviction on miss when cache is full
- Preload common ASCII/Latin glyphs at font load time

### 6.4 Text Rendering API

```c
void draw_text(ctx, x, y, "Hello 世界", font_id, size, color);
// Supports:
// - UTF-8 input (validated, invalid sequences shown as �)
// - Kerning pairs (BDF fonts)
// - Basic ligatures (fi, fl, etc.)
// - RTL text detection (Arabic, Hebrew)
// - Word wrap with configurable width
```

### 6.5 Internationalization

- Full UTF-8 text decoding
- Codepoint → glyph lookup with fallback to U+FFFD (�) for missing glyphs
- RTL layout detection and rendering for Arabic, Hebrew, Persian
- Input method hook — keyboard events can be transformed to codepoints
- Language tags in app metadata for font selection

### 6.6 Font Files

Ship in initrd under `/fonts/`:
- `dejavusans.bdf` — clean sans-serif, covers Latin + common scripts
- `notosans.bdf` or `.ttf` — extended Unicode coverage
- `term12.bdf` — monospace for terminal

---

## 7. File Explorer

### 7.1 Features

- **Navigation:** Breadcrumb path bar, back/forward history, bookmarks
- **List and grid views** (toggle with Ctrl+1, Ctrl+2)
- **Sorting:** by name, size, date modified, type (click column headers)
- **Selection:** single-click to navigate, Ctrl+click for multi-select, Shift+click for range
- **Context menu:** right-click for operations
- **Operations:**
  - Copy (Ctrl+C)
  - Cut (Ctrl+X)
  - Paste (Ctrl+V)
  - Delete (Del) — moves to `/trash/`
  - Rename (F2)
  - New Folder (Ctrl+Shift+N)
  - Properties (Alt+Enter)
- **Drag and drop:** drag files between Explorer windows to copy/move

### 7.2 Virtual Filesystem Integration

Explorer can browse:
- `/` — ext2 root filesystem
- `/dev/` — devfs entries (tty, fb, null, zero, etc.)
- `/proc/` — procfs (process list, memory maps, CPU info)
- `/sys/` — sysfs (device tree, kernel parameters)
- `/mnt/` — mounted external filesystems

Each virtual FS entry is fetched via the existing VFS layer. Files readable from procfs/sysfs show their content in a read-only viewer.

### 7.3 File Associations

| Extension | MIME Type | Default App |
|---|---|---|
| `.txt`, `.md` | text/plain | Text Editor |
| `.bmp`, `.png`, `.jpg` | image/* | Image Viewer |
| `.c`, `.h`, `.sh` | text/x-code | Text Editor |
| `.elf` | application/x-elf | (show info, don't execute) |
| `.json` | application/json | Text Editor |

When double-clicking a file, the DE looks up the MIME type and launches the associated app with the file path as argument.

### 7.4 Progress Dialogs

Long operations (copying many files, deleting large directories) show a progress dialog with:
- File being processed (current name)
- Progress bar (files completed / total)
- Cancel button

Operations are cancellable and restartable.

---

## 8. Core Applications

### 8.1 Terminal Emulator

- Connects to pseudo-TTY running the shell (loads `/shell.elf` or `/bin/sh` from initrd)
- Supports scrollback buffer (4096 lines)
- Copy/paste (Ctrl+Shift+C, Ctrl+Shift+V)
- Configurable font size (Ctrl++ / Ctrl+-)
- Full keyboard input passthrough
- Vim-like shortcuts: Ctrl+L (clear), Ctrl+C (interrupt), Tab (complete)
- Window resizing updates PTY size (SIGWINCH)

### 8.2 Text Editor

- Multi-line text buffer (up to 1MB)
- Line numbers in gutter
- Basic syntax highlighting (C, shell, JSON)
- Find/Replace (Ctrl+F / Ctrl+H)
- Save / Open via file picker (integrates with File Explorer)
- Word wrap toggle
- Undo/Redo (Ctrl+Z / Ctrl+Y)
- Status bar: line/col position, file modified state

### 8.3 Image Viewer

- Opens BMP images (native format, already supported)
- Opens PNG images (implement `lib/png.c` — parse PNG chunks, decompress IDAT with zlib, render to RGBA)
- Fit to window, actual size, zoom (1x, 2x, 4x, fit)
- Pan with mouse drag when zoomed in
- Previous/Next navigation when multiple images in same directory

### 8.4 Calculator

- Basic arithmetic (+, -, *, /)
- Decimal point support
- Clear (C), backspace
- Keyboard input (0-9, +, -, *, /, ., Enter, Esc)
- Running display of the full expression
- History of last 10 calculations

### 8.5 Clock / Calendar

- Analog clock widget
- Digital clock display
- Calendar month view
- Date/time picker for settings

### 8.6 Notes App

- Multiple notes, each a title + text body
- List of notes on left, selected note on right
- Auto-save on blur
- Stored in `/home/user/notes/` as `.txt` files

### 8.7 Paint App

- Canvas (white background)
- Tools: pencil (single pixel), brush (3px circle), eraser, fill bucket
- Color picker (RGB sliders)
- Clear canvas
- Save as BMP

### 8.8 Settings Panel

Tabbed interface:

| Tab | Settings |
|---|---|
| **Display** | Resolution (if switchable), theme colors, font size |
| **Keyboard** | Repeat rate, repeat delay, key bindings |
| **Mouse** | Pointer speed, double-click speed, tap-to-click |
| **Date & Time** | Set date, time, timezone |
| **About** | OS version, kernel info, memory usage |

### 8.9 System Monitor

- Per-process list: PID, name, CPU%, memory usage
- Sortable columns
- Kill process (right-click → Terminate / Kill)
- Total CPU and memory bar at top
- Refresh rate: 1 second

---

## 9. Input Handling

### 9.1 Keyboard

**Key event flow:**
1. PS/2 keyboard interrupt → kernel scancode → `keyboard.c`
2. Keyboard driver converts to ASCII/UTF-8 codepoint
3. DE input router checks global shortcuts first
4. If no global match, dispatch to focused app window

**Global shortcuts:**

| Shortcut | Action |
|---|---|
| `Super` | Opens Start menu |
| `Alt+Tab` | Cycle through windows |
| `Alt+Shift+Tab` | Cycle windows reverse |
| `Super+1..4` | Switch workspace |
| `Super+D` | Show desktop (minimize all) |
| `Ctrl+Alt+Delete` | Open system monitor |
| `Ctrl+Esc` | Open Start menu |
| `PrintScreen` | Screenshot (save to `/home/user/screenshots/`) |

**App-level shortcuts (vim-like for text apps):**

| Shortcut | Action |
|---|---|
| `h / j / k / l` | Move cursor (left / down / up / right) |
| `i` | Enter insert mode |
| `Esc` | Exit to command mode |
| `:` | Open command bar |
| `w` | Save |
| `q` | Close |
| `Ctrl+F` | Find |
| `Ctrl+N` | New |
| `0` | Beginning of line |
| `$` | End of line |

### 9.2 Mouse

**Event types:** move, left-click, right-click, middle-click, scroll wheel, drag

**Double-click detection:** two clicks within 500ms within 5px of each other → double-click action (open file, select word)

**Drag detection:** mousedown → mouse move > 3px → drag mode. Drop targets highlighted.

**Scroll wheel:** scroll events map to vertical scroll within focused scrollable widget.

### 9.3 Touch / Pointer

- Pointer events normalized to mouse event model
- Touch drag → scroll
- Touch tap → click
- Touch tap-and-hold → right-click
- Pinch-to-zoom in image viewer / map apps
- All interaction works with both mouse and touch input

### 9.4 Input Event Queue

DE maintains a per-app input queue (ring buffer). Events are serialized and sent over the socket protocol. App dequeues events and processes them.

---

## 10. System Integration

### 10.1 Process Manager

- DE tracks all app processes (PID, name, state)
- App crash detection via SIGCHLD / process exit notification
- On crash: close window, show crash notification with app name
- Process list accessible from System Monitor app

### 10.2 Notifications

- Apps push notifications via `NOTIFICATION` message
- Notification appears in top-right corner (slides in from right)
- Auto-dismiss after 5 seconds (configurable per notification)
- Stack vertically if multiple notifications
- Click to dismiss or open related app
- Notification center: click tray icon to expand all recent notifications

### 10.3 Settings Persistence

- Settings stored in `/home/user/.config/desktop.conf` (simple key=value format)
- Written on change (debounced, write 2s after last change)
- Read on DE startup

### 10.4 App Lifecycle

- Apps launched via `execve()` from the DE's process fork
- Apps can launch child apps (e.g., File Explorer → open file in Text Editor)
- App exit: window closes, process cleaned up
- DE provides a simple app registry at `/home/user/.local/share/applications/`

---

## 11. Performance Targets

### 11.1 Hardware Targets

| Target | Minimum | Recommended |
|---|---|---|
| CPU | x86_64, 1 core | x86_64, 2+ cores |
| RAM | 512 MB | 2 GB |
| GPU | Standard VGA framebuffer | VBE / GOP, 1024x768+ |
| Storage | 1 GB | 8 GB |
| Boot | BIOS or UEFI | UEFI |

### 11.2 Software Performance

| Metric | Target |
|---|---|
| Compositor FPS | 60 fps (16ms frame budget) |
| Boot to desktop | < 5 seconds (QEMU) |
| App launch latency | < 200ms |
| Memory per window buffer | ~3 MB (1024x768x4) |
| Max windows | 32 (with reasonable perf) |
| Dirty rect efficiency | > 70% of frames only update partial screen |

### 11.3 Optimization Strategies

- **Dirty rect tracking** — compositor only redraws changed regions
- **LRU glyph cache** — font rendering avoids repeated rasterization
- **Shared memory** — window buffers in shared memory pages, compositor and app share without copy
- **Kernel-side blit** — if possible, move compositing blit operation to a kernel helper to reduce user/kernel transitions
- **Batched socket sends** — app batches multiple small updates before sending to DE
- **Dirty-on-focus** — when a window becomes active, only mark it dirty (force repaint), others can skip if unchanged

---

## 12. Directory Structure

```
kernel/gui.c          → deprecate in favor of userland DE
kernel/gui.o          → remove from Makefile

userland/
  desktop/            → new DE server source
    main.c             — entry point, process setup
    compositor.c       — double-buffer, blend, dirty rect
    wm.c               — window manager
    input.c            — keyboard/mouse event router
    taskbar.c          — taskbar rendering and interaction
    notification.c     — notification queue and display
    font.c             — BDF/TTF parser, glyph cache
    font_bdf.c         — BDF font loader
    font_ttf.c         — TTF rasterizer (minimal)
    protocol.c         — socket protocol handling
    protocol.h         — message type definitions
  apps/                → built-in apps
    terminal/          — terminal emulator
    editor/            — text editor
    explorer/          — file explorer
    viewer/            — image viewer
    calculator/        — calculator
    clock/             — clock/calendar
    notes/             — notes app
    paint/             — paint app
    settings/          — settings panel
    monitor/           — system monitor
  lib/
    gui.h              — shared GUI drawing primitives
    font.h             — shared font API
    protocol.h         — shared protocol definitions
include/
  desktop.h           — DE server public API / syscalls
```

---

## 13. Build System Changes

Add to Makefile:

```makefile
desktop.elf: userland/desktop/*.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/desktop/*.c -o desktop.elf

apps: desktop.elf
	$(MAKE) -C userland/apps/terminal/terminal.elf
	$(MAKE) -C userland/apps/editor/editor.elf
	$(MAKE) -C userland/apps/explorer/explorer.elf
	# ... etc

initrd.tar: shell.elf hello.elf desktop.elf apps fonts/
	tar -cf initrd.tar shell.elf hello.elf desktop.elf apps/ fonts/
```

**Fonts bundle:** create `fonts/` directory with BDF/TTF fonts, include in initrd.

---

## 14. Phases

This is a large project. The implementation should proceed in clear phases:

1. **Phase 1: Foundation** — Userland DE server, basic compositor, window manager with drag/focus
2. **Phase 2: App Framework** — App-to-DE protocol, shared GUI library, first app (terminal)
3. **Phase 3: Full Window Manager** — Resize, minimize, maximize, tiling, workspaces
4. **Phase 4: Font System** — BDF parser, TTF rasterizer, glyph cache, UTF-8 text rendering
5. **Phase 5: File Explorer** — Full file operations, virtual FS, drag-and-drop, associations
6. **Phase 6: App Suite** — All apps (editor, viewer, calculator, clock, notes, paint, settings, monitor)
7. **Phase 7: Polish** — System tray, notifications, keyboard shortcuts, theming, performance tuning

---

## 15. Dependencies on Existing Code

- `kernel/fb.c` — framebuffer info exposed via syscalls to userland
- `kernel/keyboard.c` — keyboard events delivered via existing mechanism
- `kernel/vfs.c` — file operations for initrd access
- `kernel/elf.c` — ELF loader for running apps
- `kernel/socket.c` — socket API for DE-app communication
- `kernel/ipc.c` — existing IPC primitives
- `kernel/sched.c` — process/thread management
- `kernel/pmm.c` — memory info for system monitor
- `fs/procfs.c` — `/proc` data for system monitor
- `fs/sysfs.c` — `/sys` data for settings panel
- `fs/devfs.c` — `/dev` access

**Changes needed to existing code:**
- Extend framebuffer mmap to userland access
- Expose keyboard/mouse events to userland via file descriptor
- Add a syscall or socket-based API for the DE to register as input consumer
- Extend existing `syscall.c` with desktop-related syscalls (set wallpaper, get theme, etc.)
- Optionally move font data to initrd's `/fonts/` directory