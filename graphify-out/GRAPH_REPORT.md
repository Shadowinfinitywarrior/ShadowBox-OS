# Graph Report - OS  (2026-08-18)

## Corpus Check
- 13 files · ~268,918 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3401 nodes · 7133 edges · 257 communities (214 shown, 43 thin omitted)
- Extraction: 78% EXTRACTED · 22% INFERRED · 0% AMBIGUOUS · INFERRED: 1567 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Shell & Command History
- AC97 Audio Codec
- Desktop Server & Sysmon
- Public Kernel Headers
- Userland CLI Utilities
- Widget Event System
- Paint & 2D Drawing
- Clock Window Widget
- Compositor & Input Router
- GUI Canvas Drawing
- Basic Userland Tools
- Archive & File Tools
- VFS Locks & Pipes
- Framebuffer Drawing
- TextBox Widget
- Syscall Wrappers & IPC
- Desktop Applications
- Slab Allocator & HAL
- Boot & Device Init
- UI Theme & Colors
- HTTP Browser
- Kernel IPC Messaging
- GUI Framebuffer Bridge
- Network Stack Core
- Mouse & Poll Threads
- SMP Bootstrap
- Security & Capabilities
- Syscall FS Operations
- App Icon Rendering
- Ext2 Filesystem
- Browser Navigation
- HAL CPU Abstractions
- Sockets & HTTP Client
- Input Router Internals
- ShadowBridge IPC
- Animation Engine
- Tmpfs Filesystem
- GUIManager C++ Core
- GUI C Wrappers
- ACPI & Heap Alloc
- I2C Master Bus
- HAL Memory Layer
- Button Widget
- Login Screen
- Bluetooth Core
- Block Device Layer
- WiFi Driver
- Cursor & Input Routing
- Tetris Game
- Desktop Icons & Launcher
- APIC Interrupts
- GUI Manager Demo
- HDA Audio & PCI
- DRM Core
- IRQ Monitor & Audit
- GUI Button C Wrapper
- Freestanding C Runtime
- Context Menu
- ACPI Power States
- Framebuffer Double Buffer
- HDMI Audio Driver
- GUI Compositor C Wrapper
- Calculator Window
- GUI Textbox C Wrapper
- ScrollView Widget
- Snake Game
- Toggle Widget
- File Manager App
- Intel GPU Driver
- ASLR & SMEP/SMAP
- Design Specs & Plans
- Input Event Types
- Dirty Rect Tracking
- HAL Architecture Layer
- Virtio Block Driver
- HDA Audio Streams
- Device & Bus Registry
- Security Labels
- GUI Widget Toolkit
- Desktop Corruption Fallback
- Workspace Manager
- I2S Audio Driver
- Camera Subsystem
- Desktop ELF Server
- Devfs Pseudo-Files
- GUI Scrollview C Wrapper
- Hex Editor Window
- Sysfs Kobject Model
- WM Focus & Decorations
- Desktop Popup & Mouse
- USB Audio Class
- GUI Widget C Wrapper
- Pseudo-Terminal
- Time & Clock Services
- README Concepts
- CI & Build Logs
- VFS Sysfs Attributes
- Kernel Boot & Init
- Root Session
- Hex Editor CLI
- Media Player
- E1000 NIC Driver
- PS/2 Mouse Driver
- Procfs
- GUI Context Menu C Wrapper
- Interactive Desktop
- Matrix Rain App
- Paint Window Logic
- AHCI SATA Driver
- Buddy Allocator
- GUI Label C Wrapper
- Subsystem Init Entry
- Evdev Input Events
- HID Core
- GUI Window Dispatch
- Layout Engine
- Ext4 Filesystem
- Taskbar Drawing
- HID Keyboard
- Notification Daemon
- WiFi Network Device
- Physical Memory Manager
- Sysfs Filesystem
- GUI Manager Renderer
- Widget Hit Testing
- Keyboard Shortcuts
- Module System
- Test Python Scripts
- Scientific Calculator
- Terminal Rendering
- Search Utility
- App Launcher Registry
- Framebuffer Syscalls
- ShadowFS
- Screensaver Engine
- System Diagnostics Apps
- Init Services
- Keyboard Layouts
- Input Multiplexer
- TarFS
- Settings Daemon
- Text Input Widget
- Browser HTTP Rendering
- Bluetooth Device Layer
- GUI Render Headers
- UI Font Rendering
- FAT32 Filesystem
- Trackpad Input
- NUMA Memory
- Color Picker
- QEMU Boot Smoke Test
- ACPI AML Interpreter
- MMap Syscalls
- Build Error Logs
- Directory Watch
- PCI MSI
- Wallpaper Engine
- Entropy Source
- Printk & VGA
- Pipe Permissions
- WM Window Core
- Ramdisk
- GDT Setup
- Sys Wait4 Errors
- Clock App Icons
- XHCI Driver
- Fortune App
- Hex Viewer Icons
- AC97 PCI Driver
- Syscall Entry
- IPv6
- WiFi BSS Scan
- Notepad App
- Reminders App
- Snake Icons
- Graphify Plugin
- Tar Test
- ISR Scripts
- Text Editor App
- Build Docs
- Font Viewer App
- Paint App
- Pong Game
- Settings Icon
- Init Script
- RTC Time
- Sb Pull Wrapper
- Sb Push Wrapper
- Bluetooth Status
- Keyboard Patch Script
- Net Device Type
- PCI Device Type
- Audio Device Type
- Bluetooth Connection
- L2CAP Channel
- Color Type
- Void Function Type
- Browser Icon
- Calculator App Icon
- Image Viewer Icon
- Keyboard Power Icon
- Mandelbrot Icon
- Matrix Rain Icon
- Package Manager Icon
- Tetris Icon
- VFS Node Type
- Net Device Refs
- Text Align Type
- Window Type
- SB Msg Type
- Widget Flags
- Wrap Mode

## God Nodes (most connected - your core abstractions)
1. `printk()` - 175 edges
2. `sb_push()` - 122 edges
3. `Widget` - 98 edges
4. `_start()` - 89 edges
5. `kmalloc()` - 81 edges
6. `Window` - 70 edges
7. `sb_pull()` - 67 edges
8. `sb_terminate()` - 59 edges
9. `sb_release()` - 56 edges
10. `sb_acquire()` - 55 edges

## Surprising Connections (you probably didn't know these)
- `devfs_register_input()` --calls--> `printk()`  [INFERRED]
  fs/devfs.c → kernel/printk.c
- `fb_init()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/fb.c → kernel/printk.c
- `hdmi_audio_irq_handler()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/hdmi_audio.c → kernel/printk.c
- `i2s_set_volume()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/i2s.c → kernel/printk.c
- `rtc_init()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/rtc.c → kernel/printk.c

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Core Kernel Subsystems** — readme_custom_x86_64_kernel, readme_memory_management, readme_scheduling_ipc, readme_filesystems, readme_device_drivers [EXTRACTED 1.00]
- **Kernel Build -> Link -> Boot Validation Flow** — _github_workflows_ci, cmakelists_makefile, link_cmd_os_bin_link, qemu_output_qemu_smoke [INFERRED 0.85]
- **Userland Desktop Environment (desktop.elf)** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_desktop_elf, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_window_manager, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_double_buffering, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_shadowbox_desktop, readme_graphical_desktop [INFERRED 0.85]

## Communities (257 total, 43 thin omitted)

### Community 0 - "Shell & Command History"
Cohesion: 0.06
Nodes (94): add_history(), alias_get(), alias_set(), atoi(), cmd_alias(), cmd_banner(), cmd_basename(), cmd_bench() (+86 more)

### Community 1 - "AC97 Audio Codec"
Cohesion: 0.05
Nodes (69): ac97_device_t, ac97_codec_reset(), ac97_configure_mixer(), ac97_create_audio_dev(), ac97_detect_codecs(), ac97_init(), ac97_irq_handler(), ac97_mixer_read() (+61 more)

### Community 2 - "Desktop Server & Sysmon"
Cohesion: 0.04
Nodes (54): main(), _start(), sb_msg_t, sys_status_t, window_t, draw_sysmon(), get_num_processes(), atoi_octal() (+46 more)

### Community 4 - "Userland CLI Utilities"
Cohesion: 0.06
Nodes (56): print(), print(), print(), print_uint(), _start(), print(), print_uint(), _start() (+48 more)

### Community 5 - "Widget Event System"
Cohesion: 0.05
Nodes (41): DirtyList, EventFn, ClockWindow::ClockWindow(), HexEditWindow::HexEditWindow(), KernelConfigView, content_, scroll_view_, InputRouter (+33 more)

### Community 6 - "Paint & 2D Drawing"
Cohesion: 0.04
Nodes (55): paint, raise_to_top, set_visible, blend_alpha(), close_btn_clicked(), InputEvent, Rect, Color (+47 more)

### Community 7 - "Clock Window Widget"
Cohesion: 0.05
Nodes (37): ClockWindow, tick, ticks_, time_label_, fmt_time(), TextAlign, Widget, WrapMode (+29 more)

### Community 8 - "Compositor & Input Router"
Cohesion: 0.06
Nodes (27): repaint, MouseCursor, InputRouter, MouseCursor, paint_self, router_, paint_children, TopBar (+19 more)

### Community 9 - "GUI Canvas Drawing"
Cohesion: 0.07
Nodes (35): block_write(), gui_clear_screen(), gui_draw_filled_rect(), gui_draw_pixel(), gui_draw_rect(), calendar_get_date(), calendar_get_time(), is_leap_year() (+27 more)

### Community 10 - "Basic Userland Tools"
Cohesion: 0.06
Nodes (33): u8, _start(), _start(), main(), _start(), print(), _start(), attempt_login() (+25 more)

### Community 11 - "Archive & File Tools"
Cohesion: 0.12
Nodes (44): main(), parse_size(), print_uint(), read_line(), _start(), create_window(), _start(), maybe_load_wallpaper() (+36 more)

### Community 12 - "VFS Locks & Pipes"
Cohesion: 0.10
Nodes (42): spin_lock_irqsave(), spin_unlock_irqrestore(), pipe_close(), pipe_read(), pipe_write(), softirq_do_pending(), softirq_init(), softirq_raise() (+34 more)

### Community 13 - "Framebuffer Drawing"
Cohesion: 0.11
Nodes (34): blend(), fb_draw_rect(), fb_draw_rect_round(), fb_draw_text(), fb_draw_text_wrap(), fb_fill_rect(), fb_fill_rect_round(), fb_text_width() (+26 more)

### Community 14 - "TextBox Widget"
Cohesion: 0.05
Nodes (36): ChangeFn, Color, TextBox, bg_focused, bg_normal, border_focused, border_normal, buf_ (+28 more)

### Community 15 - "Syscall Wrappers & IPC"
Cohesion: 0.05
Nodes (3): pipe_count_inc(), sys_dup(), sys_dup2()

### Community 16 - "Desktop Applications"
Cohesion: 0.12
Nodes (39): browser_init(), create_window(), editor_insert_char(), editor_load(), editor_newline(), fortune_next(), g2048_can_move(), g2048_init() (+31 more)

### Community 17 - "Slab Allocator & HAL"
Cohesion: 0.09
Nodes (31): tss_set_stack(), slab_alloc(), slab_create_cache(), slab_free(), cpu_halt(), idle_loop(), idle_tasks_init(), heapify_down() (+23 more)

### Community 18 - "Boot & Device Init"
Cohesion: 0.09
Nodes (30): ramdisk_init(), slab_init(), display_output_t, device_t, i2c_hid_init(), i2c_hid_probe(), i2c_hid_remove(), device_t (+22 more)

### Community 19 - "UI Theme & Colors"
Cohesion: 0.05
Nodes (29): accent_color(), DarkTheme, Accent, AccentHover, AccentPress, Background, Border, Danger (+21 more)

### Community 20 - "HTTP Browser"
Cohesion: 0.11
Nodes (32): http_request_t, atoi(), browser_main_loop(), browser_navigate(), draw_browser(), htonl(), htons(), is_tracker_url() (+24 more)

### Community 21 - "Kernel IPC Messaging"
Cohesion: 0.08
Nodes (23): msgget(), msgq_find(), msgrcv(), msgsnd(), sem_find(), semget(), semop(), shmat() (+15 more)

### Community 22 - "GUI Framebuffer Bridge"
Cohesion: 0.10
Nodes (29): fb_blit_rect(), gui_blit_screen(), gui_fb_flip(), sys_fb_flip(), Compositor, add_dirty, add_root, animate_roots (+21 more)

### Community 23 - "Network Stack Core"
Cohesion: 0.13
Nodes (33): arp_handle_packet(), net_device_t, icmp_handle_packet(), ip_handle_packet(), net_checksum(), net_handle_packet(), net_device_t, tcp_checksum() (+25 more)

### Community 24 - "Mouse & Poll Threads"
Cohesion: 0.07
Nodes (10): mouse_start_poll_thread(), net_device_t, virtio_net_send_packet(), UNUSED, swap_init(), swap_page_in(), swap_page_out(), kthread_create() (+2 more)

### Community 25 - "SMP Bootstrap"
Cohesion: 0.11
Nodes (28): apic_count_cpus(), lapic_read(), gdt_init_ap(), idt_init_ap(), ap_trampoline_copy(), current_cpu_id(), heapify_down(), heapify_up() (+20 more)

### Community 26 - "Security & Capabilities"
Cohesion: 0.12
Nodes (26): kernel_cap_t, cap_and(), cap_clear(), cap_isclear(), cap_isfull(), cap_lower(), cap_or(), cap_raise() (+18 more)

### Community 27 - "Syscall FS Operations"
Cohesion: 0.06
Nodes (31): is_user_range(), sb_morph(), sys_access(), sys_chdir(), sys_chmod(), sys_chown(), sys_clock_gettime(), sys_fb_info() (+23 more)

### Community 28 - "App Icon Rendering"
Cohesion: 0.15
Nodes (31): blend_color(), draw_app_window(), draw_bell_icon(), draw_bt_icon(), draw_button(), draw_char(), draw_circle(), draw_cursor() (+23 more)

### Community 29 - "Ext2 Filesystem"
Cohesion: 0.27
Nodes (27): kfree(), kmalloc(), ext2_inode_t, vfs_node_t, ext2_add_dir_entry(), ext2_alloc_block_from_group(), ext2_allocate_block(), ext2_allocate_inode() (+19 more)

### Community 30 - "Browser Navigation"
Cohesion: 0.20
Nodes (29): browser_back(), browser_fwd(), browser_load(), browser_navigate(), browser_refresh(), calc_click(), calc_input(), editor_save() (+21 more)

### Community 31 - "HAL CPU Abstractions"
Cohesion: 0.08
Nodes (9): cpu_info_t, cpu_registers_t, hal_status_t, cpu_get_info(), cpu_get_vendor_string(), cpu_init(), cpu_restore_registers(), cpu_save_registers() (+1 more)

### Community 32 - "Sockets & HTTP Client"
Cohesion: 0.10
Nodes (24): http_get(), socket_close(), socket_connect(), socket_create(), socket_recv(), socket_send(), atoi(), main() (+16 more)

### Community 33 - "Input Router Internals"
Cohesion: 0.07
Nodes (25): InputRouter, comp_, cursor_, inject_key_release, inject_mouse_packet, KEY_BACKSPACE, KEY_DELETE, KEY_DOWN (+17 more)

### Community 34 - "ShadowBridge IPC"
Cohesion: 0.15
Nodes (26): sb_msg_t, sb_acquire(), sb_ipc_call(), sb_ipc_reply_wait(), sb_morph(), sb_pull(), sb_push(), sb_release() (+18 more)

### Community 35 - "Animation Engine"
Cohesion: 0.10
Nodes (19): bezier_curve_t, spring_physics_t, animation_engine_init(), animation_engine_set_hz(), animation_step_bezier(), animation_step_spring(), animation_viewer_init(), animation_viewer_tick() (+11 more)

### Community 36 - "Tmpfs Filesystem"
Cohesion: 0.19
Nodes (24): vfs_node_t, tmpfs_access(), tmpfs_create_entry(), tmpfs_create_file_entry(), tmpfs_create_func(), tmpfs_find_child(), tmpfs_finddir(), tmpfs_finddir_func() (+16 more)

### Community 37 - "GUIManager C++ Core"
Cohesion: 0.13
Nodes (25): Widget, GUIManager, add_root, comp_, composite_count_, cursor_, focused, frame (+17 more)

### Community 38 - "GUI C Wrappers"
Cohesion: 0.14
Nodes (24): button_click_trampoline(), gui_button_create(), gui_compositor_remove_root(), gui_compositor_set_focus(), gui_input_router_create(), gui_input_router_destroy(), gui_input_router_key_press(), gui_input_router_key_release() (+16 more)

### Community 39 - "ACPI & Heap Alloc"
Cohesion: 0.20
Nodes (22): acpi_init(), page_fault_handler(), expand_heap(), malloc_init(), split_block(), pmm_alloc_page(), pmm_total_pages(), invlpg() (+14 more)

### Community 40 - "I2C Master Bus"
Cohesion: 0.11
Nodes (13): device_t, driver_t, i2c_master_match(), i2c_master_probe(), i2c_master_remove(), device_t, spi_master_probe(), spi_master_register() (+5 more)

### Community 41 - "HAL Memory Layer"
Cohesion: 0.10
Nodes (13): dram_info_t, hal_status_t, memory_alloc_physical(), memory_get_dram_info(), memory_get_map(), memory_get_stats(), memory_init(), memory_map_physical() (+5 more)

### Community 42 - "Button Widget"
Cohesion: 0.14
Nodes (17): Button, bg_hover, bg_normal, bg_press, button_bg_pressed(), border_col, corner_radius, fg_hover (+9 more)

### Community 43 - "Login Screen"
Cohesion: 0.28
Nodes (22): u32, u64, blend(), draw_background(), draw_button(), draw_char(), draw_cursor(), draw_field() (+14 more)

### Community 44 - "Bluetooth Core"
Cohesion: 0.20
Nodes (21): bt_hci_dev_t, bt_alloc_dma(), bt_connect_le(), bt_disconnect(), bt_get_device(), bt_handle_packet(), bt_hardware_init(), bt_hci_init() (+13 more)

### Community 45 - "Block Device Layer"
Cohesion: 0.12
Nodes (12): block_get_device(), block_init(), block_read(), block_device_t, init_main(), _start(), start_service(), copy_from_user() (+4 more)

### Community 47 - "WiFi Driver"
Cohesion: 0.21
Nodes (18): wifi_connect(), wifi_disconnect(), wifi_eeprom_init(), wifi_get_device(), wifi_hardware_init(), wifi_hw_init(), wifi_init_net_device(), wifi_irq_handler() (+10 more)

### Community 48 - "Cursor & Input Routing"
Cohesion: 0.14
Nodes (16): Compositor, InputRouter, input_router_create(), input_router_destroy(), input_router_key_press(), input_router_key_release(), input_router_mouse_absolute(), input_router_mouse_packet() (+8 more)

### Community 49 - "Tetris Game"
Cohesion: 0.18
Nodes (19): rng_next(), TetrisWindow, board_, collides, current_piece_, lines_, lock_piece, on_key_press (+11 more)

### Community 50 - "Desktop Icons & Launcher"
Cohesion: 0.19
Nodes (18): Terminal App Icon (terminal.png), Makefile (build and initrd packaging), blit_icon(), draw_desktop_icons(), draw_icon_folder(), draw_procedural(), icon_bmp_blit(), icon_bmp_load() (+10 more)

### Community 51 - "APIC Interrupts"
Cohesion: 0.16
Nodes (16): apic_init(), ioapic_route_irq(), ioapic_set_entry(), ioapic_write(), lapic_enable(), lapic_eoi(), lapic_write(), pit_handler() (+8 more)

### Community 52 - "GUI Manager Demo"
Cohesion: 0.13
Nodes (6): Widget, GUIManager, GUIManager::add_root(), GUIManager::focused(), GUIManager::remove_root(), GUIManager::set_focus()

### Community 53 - "HDA Audio & PCI"
Cohesion: 0.19
Nodes (16): hda_init(), pci_device_t, pci_check_device(), pci_check_function(), pci_config_read(), pci_config_write(), pci_enable_bus_mastering(), pci_enumerate() (+8 more)

### Community 54 - "DRM Core"
Cohesion: 0.13
Nodes (14): vmap_phys(), drm_connector_t, drm_crtc_t, drm_driver_t, drm_gem_object_t, device_t, drm_gem_create(), drm_gem_free() (+6 more)

### Community 55 - "IRQ Monitor & Audit"
Cohesion: 0.19
Nodes (18): audit_entry_t, battery_status_t, spin_lock(), spin_unlock(), irq_monitor_print_stats(), irq_monitor_record(), power_battery_get(), audit_get_entries() (+10 more)

### Community 56 - "GUI Button C Wrapper"
Cohesion: 0.15
Nodes (17): gui_button_t, gui_widget_t, gui_button_create(), gui_button_destroy(), gui_button_set_label(), gui_button_set_on_clicked(), gui_button_set_pos(), gui_button_set_size() (+9 more)

### Community 57 - "Freestanding C Runtime"
Cohesion: 0.14
Nodes (12): _align_up(), free(), malloc(), memcpy(), realloc(), _sbrk(), DirtyRegion, dirty_region_add() (+4 more)

### Community 58 - "Context Menu"
Cohesion: 0.14
Nodes (11): ContextMenu, add_item, hovered_index_, item_count_, ITEM_HEIGHT, items_, MAX_ITEMS, on_mouse_move (+3 more)

### Community 59 - "ACPI Power States"
Cohesion: 0.12
Nodes (10): acpi_fadt_t, c_state_t, acpi_fadt_checksum(), parse_acpi_tables(), power_get_c_state(), power_get_p_state(), power_shutdown(), power_subsys_init() (+2 more)

### Community 60 - "Framebuffer Double Buffer"
Cohesion: 0.23
Nodes (12): fb_double_free(), fb_double_init(), fb_console_init(), fb_console_putchar(), fb_get_addr(), fb_get_bpp(), fb_get_height(), fb_get_pitch() (+4 more)

### Community 61 - "HDMI Audio Driver"
Cohesion: 0.20
Nodes (16): pci_device_t, hdmi_audio_detect_sink(), hdmi_audio_init(), hdmi_audio_irq_handler(), hdmi_audio_register_sink(), hdmi_audio_set_mode(), hdmi_detect_sink_eld(), hdmi_pin_sense_enable() (+8 more)

### Community 62 - "GUI Compositor C Wrapper"
Cohesion: 0.18
Nodes (16): gui_comp_t, gui_widget_t, gui_compositor_add_root(), gui_compositor_create(), gui_compositor_destroy(), gui_compositor_focused(), gui_compositor_frame(), gui_compositor_remove_root() (+8 more)

### Community 63 - "Calculator Window"
Cohesion: 0.22
Nodes (16): CalculatorWindow, acc_val_, append_digit, apply_op, calculate, current_op_, current_val_, display_buffer_ (+8 more)

### Community 64 - "GUI Textbox C Wrapper"
Cohesion: 0.15
Nodes (16): gui_widget_t, gui_textbox_create(), gui_textbox_destroy(), gui_textbox_set_placeholder(), gui_textbox_set_pos(), gui_textbox_set_size(), gui_textbox_set_text(), gui_textbox_text() (+8 more)

### Community 65 - "ScrollView Widget"
Cohesion: 0.16
Nodes (14): Color, ScrollView, clamp_scroll, content_, MIN_THUMB, on_mouse_scroll, SCROLL_STEP, scroll_to (+6 more)

### Community 66 - "Snake Game"
Cohesion: 0.16
Nodes (16): rng_next(), SnakeWindow, food_x_, food_y_, GRID_SIZE, MAX_SNAKE, on_key_press, reset (+8 more)

### Community 67 - "Toggle Widget"
Cohesion: 0.18
Nodes (12): InputEvent, Toggle, INDICATOR_R, on_mouse_move, on_mouse_press, on_mouse_release, on_toggled_, set_state (+4 more)

### Community 68 - "File Manager App"
Cohesion: 0.22
Nodes (16): OS Application Icon Set, File Explorer App Icon, compare_entries(), copy_file(), create_directory(), create_file(), delete_entry(), filter_entries() (+8 more)

### Community 69 - "Intel GPU Driver"
Cohesion: 0.25
Nodes (16): fb_set_info(), pci_device_t, intel_detect_gen(), intel_forcewake_get(), intel_forcewake_put(), intel_gpu_alloc_framebuffer(), intel_gpu_flush_framebuffer(), intel_gpu_get_info() (+8 more)

### Community 70 - "ASLR & SMEP/SMAP"
Cohesion: 0.20
Nodes (13): aslr_enable_smap(), aslr_enable_smep(), aslr_get_heap_base(), aslr_get_mmap_base(), aslr_get_stack_base(), aslr_init(), aslr_random_addr(), cpu_has_smap() (+5 more)

### Community 71 - "Design Specs & Plans"
Cohesion: 0.18
Nodes (17): ShadowBox Phase 1 Foundation Plan, ShadowBox Desktop Design Spec v1.0, App-to-DE Domain Socket Protocol, App Launcher, Compositor, Dirty-Rect Tracking, File Explorer, Input Router (+9 more)

### Community 72 - "Input Event Types"
Cohesion: 0.12
Nodes (17): EventType, dispatch, InputEvent, button, key, mods, pos, scroll_delta (+9 more)

### Community 73 - "Dirty Rect Tracking"
Cohesion: 0.12
Nodes (9): DirtyList, count, MAX, rects, Rect, h, w, x (+1 more)

### Community 74 - "HAL Architecture Layer"
Cohesion: 0.17
Nodes (14): hal_arch_t, hal_status_t, hal_get_arch(), hal_init(), hal_shutdown(), hal_status_t, i2c_init(), peripheral_enumerate() (+6 more)

### Community 75 - "Virtio Block Driver"
Cohesion: 0.32
Nodes (14): pci_device_t, vblk_add_desc(), vblk_do_request(), vblk_init_block_device(), vblk_init_virtqueue(), vblk_negotiate_features(), vblk_read_reg(), vblk_reset_device() (+6 more)

### Community 76 - "HDA Audio Streams"
Cohesion: 0.18
Nodes (13): audio_device_t, audio_format_t, audio_hda_init(), hda_close_stream(), hda_open_stream(), hda_read_pcm(), hda_write_pcm(), hdmi_audio_init() (+5 more)

### Community 77 - "Device & Bus Registry"
Cohesion: 0.25
Nodes (14): bus_type_t, i2c_master_init(), bus_add_device(), bus_add_driver(), bus_register(), bus_unregister(), device_t, driver_t (+6 more)

### Community 78 - "Security Labels"
Cohesion: 0.14
Nodes (4): sec_label_init(), sec_label_transition(), strncpy(), security_label_t

### Community 79 - "GUI Widget Toolkit"
Cohesion: 0.25
Nodes (14): widget_t, window_t, gui_layout_pass(), gui_mark_damage(), gui_paint_pass(), gui_update_cursor(), gui_widget_get_focused(), gui_widget_set_focus() (+6 more)

### Community 80 - "Desktop Corruption Fallback"
Cohesion: 0.33
Nodes (13): blend_color(), window_t, draw_char(), draw_cursor(), draw_desktop(), draw_drop_shadow(), draw_number(), draw_rect() (+5 more)

### Community 81 - "Workspace Manager"
Cohesion: 0.21
Nodes (13): wm_layout_mode_t, workspace_t, xdg_toplevel_t, find_workspace_by_id(), wm_add_window_to_active(), wm_animate_window_close(), wm_animate_window_open(), wm_get_active_workspace() (+5 more)

### Community 82 - "I2S Audio Driver"
Cohesion: 0.25
Nodes (12): pci_device_t, i2s_alloc_dma_buffer(), i2s_configure(), i2s_init(), i2s_irq_handler(), i2s_read_reg(), i2s_set_volume(), i2s_start_rx() (+4 more)

### Community 83 - "Camera Subsystem"
Cohesion: 0.25
Nodes (13): cam_device_t, cam_format_t, cam_frame_t, cam_close(), cam_dequeue_frame(), cam_enqueue_frame(), cam_open(), cam_set_format() (+5 more)

### Community 84 - "Desktop ELF Server"
Cohesion: 0.23
Nodes (13): desktop.elf Userland DE Server, userland/desktop/fb.c, userland/desktop/font.c, userland/desktop/input.c, userland/desktop/main.c, desktop_thread Kernel Launch Path, userland/desktop/wm.c, Double-Buffered Compositing (+5 more)

### Community 85 - "Devfs Pseudo-Files"
Cohesion: 0.21
Nodes (12): vfs_node_t, dev_input_read(), dev_null_read(), dev_null_write(), dev_zero_read(), dev_zero_write(), devfs_finddir(), devfs_init() (+4 more)

### Community 86 - "GUI Scrollview C Wrapper"
Cohesion: 0.20
Nodes (12): gui_widget_t, gui_scrollview_create(), gui_scrollview_destroy(), gui_scrollview_set_content(), gui_scrollview_set_pos(), gui_scrollview_set_size(), gui_scrollview_create(), gui_scrollview_destroy() (+4 more)

### Community 87 - "Hex Editor Window"
Cohesion: 0.20
Nodes (12): HexEditWindow, BYTES_PER_ROW, cursor_, data_, file_size_, load_file, MAX_FILE_SIZE, on_key_press (+4 more)

### Community 88 - "Sysfs Kobject Model"
Cohesion: 0.14
Nodes (13): kset, kobj, list, parent, sysfs_attribute, mode, name, private (+5 more)

### Community 89 - "WM Focus & Decorations"
Cohesion: 0.15
Nodes (3): xdg_toplevel_t, wm_animate_window_close(), wm_animate_window_open()

### Community 90 - "Desktop Popup & Mouse"
Cohesion: 0.19
Nodes (14): close_window(), draw_popup_panel(), handle_mouse_press(), icon_at(), in_rect(), popup_close_rect(), popup_open(), popup_rect() (+6 more)

### Community 91 - "USB Audio Class"
Cohesion: 0.28
Nodes (12): audio_device_t, device_t, uac_alloc_device(), uac_create_audio_dev(), uac_parse_descriptors(), uac_probe(), uac_remove(), usb_audio_irq_handler() (+4 more)

### Community 92 - "GUI Widget C Wrapper"
Cohesion: 0.31
Nodes (11): gui_bool_t, gui_widget_t, gui_widget_add_child(), gui_widget_enabled(), gui_widget_focused(), gui_widget_hovered(), gui_widget_parent(), gui_widget_pressed() (+3 more)

### Community 93 - "Pseudo-Terminal"
Cohesion: 0.21
Nodes (8): vfs_node_t, pty_create(), pty_read(), pty_subsystem_init(), pty_vfs_read(), pty_vfs_write(), pty_write(), sys_pty_create()

### Community 94 - "Time & Clock Services"
Cohesion: 0.23
Nodes (10): get_ms_time(), get_ns_time(), ktime_get(), ktime_get_ns(), ktime_get_real_ns(), ktime_get_ts(), mdelay(), msleep() (+2 more)

### Community 95 - "README Concepts"
Cohesion: 0.26
Nodes (13): Built-in HTTP Text Browser, Custom x86_64 Kernel, Device Drivers (AHCI, USB, HDA, NIC, framebuffer), Filesystems (VFS, tarfs, devfs, tmpfs, ext2/4, fat32), Graphical Desktop (WM + 22 apps), GRUB multiboot2 ISO boot, HAL Layer (kernel/hal), Kernel input ring buffer (input_event_t) (+5 more)

### Community 96 - "CI & Build Logs"
Cohesion: 0.21
Nodes (12): CI GitHub Actions Workflow, CMake Configure/Build CI Step, build_output.txt (font8x16 Error), font8x16 Static/Extern Declaration Conflict, CMakeLists.txt Wrapper, build_all Custom Target, Default ALL Target, Top-Level Makefile (+4 more)

### Community 98 - "VFS Sysfs Attributes"
Cohesion: 0.17
Nodes (12): vfs_node_t, kobject, attr_count, attrs, bin_attrs, kset, name, parent (+4 more)

### Community 99 - "Kernel Boot & Init"
Cohesion: 0.29
Nodes (9): arch_init(), bootloader_info_parse(), initrd_mount(), kernel_main(), kernel_subsys_init(), mm_init(), smp_bringup(), syscall_task_init() (+1 more)

### Community 100 - "Root Session"
Cohesion: 0.21
Nodes (10): user_session_t, get_root_session(), root_init(), user_session_t, permission_agent_request_auth(), session_authenticate_user(), session_create(), session_destroy() (+2 more)

### Community 101 - "Hex Editor CLI"
Cohesion: 0.32
Nodes (11): display(), main(), parse_hex_byte(), parse_hex_digit(), print(), print_dec(), print_hex2(), print_hex6() (+3 more)

### Community 102 - "Media Player"
Cohesion: 0.39
Nodes (11): add_to_playlist(), next_track(), play_track(), prev_track(), print_uint(), readline(), show_playlist(), show_track_info() (+3 more)

### Community 103 - "E1000 NIC Driver"
Cohesion: 0.27
Nodes (10): net_device_t, pci_device_t, e1000_init(), e1000_irq_handler(), e1000_read(), e1000_send_packet(), e1000_write(), pci_device_t (+2 more)

### Community 104 - "PS/2 Mouse Driver"
Cohesion: 0.35
Nodes (10): mouse_handler(), mouse_init(), mouse_process_byte(), mouse_read_ack(), mouse_write(), mouse_write_with_retry(), mouse_poll_thread(), ps2_read() (+2 more)

### Community 105 - "Procfs"
Cohesion: 0.29
Nodes (10): pmm_get_info(), vfs_node_t, proc_cpuinfo_read(), proc_filesystems_read(), proc_meminfo_read(), proc_stat_read(), proc_version_read(), procfs_finddir() (+2 more)

### Community 106 - "GUI Context Menu C Wrapper"
Cohesion: 0.22
Nodes (8): gui_context_menu_t, gui_widget_t, gui_context_menu_add_item(), gui_context_menu_create(), gui_context_menu_destroy(), gui_context_menu_add_item(), gui_context_menu_create(), gui_context_menu_destroy()

### Community 107 - "Interactive Desktop"
Cohesion: 0.18
Nodes (4): Compositor, InputRouter, about_create(), init_wallpaper()

### Community 108 - "Matrix Rain App"
Cohesion: 0.24
Nodes (9): m_rand(), MatrixWindow, chars_, COLS, drops_, MatrixWindow::MatrixWindow(), ROWS, tick (+1 more)

### Community 109 - "Paint Window Logic"
Cohesion: 0.20
Nodes (11): cycle_windows(), draw_line_canvas(), handle_mouse_release(), isqrt(), paint_commit_shape(), scroll_top_window(), _start(), term_forward() (+3 more)

### Community 110 - "AHCI SATA Driver"
Cohesion: 0.38
Nodes (9): ahci_init(), ahci_read(), ahci_write(), UNUSED, check_type(), port_rebase(), start_cmd(), stop_cmd() (+1 more)

### Community 111 - "Buddy Allocator"
Cohesion: 0.27
Nodes (5): buddy_add_to_list(), buddy_alloc(), buddy_free(), buddy_init(), buddy_remove_from_list()

### Community 112 - "GUI Label C Wrapper"
Cohesion: 0.24
Nodes (8): gui_widget_t, gui_label_create(), gui_label_destroy(), gui_label_set_text(), gui_label_create(), gui_label_destroy(), gui_label_set_text(), gui_label_t

### Community 113 - "Subsystem Init Entry"
Cohesion: 0.20
Nodes (8): spinlock_init(), irq_monitor_init(), power_init(), power_suspend(), security_init(), socket_init(), power_suspend_enter(), power_suspend_exit()

### Community 114 - "Evdev Input Events"
Cohesion: 0.29
Nodes (8): input_dev_t, input_event_t, evdev_poll_event(), evdev_push(), input_allocate_device(), input_register_device(), input_report_event(), input_sync()

### Community 115 - "HID Core"
Cohesion: 0.38
Nodes (9): hid_device_t, hid_core_init(), hid_pointer_init(), hid_pointer_process_report(), hid_process_report(), hid_register_device(), hid_touchpad_init(), hid_touchpad_process_report() (+1 more)

### Community 116 - "GUI Window Dispatch"
Cohesion: 0.40
Nodes (9): libinput_event_t, widget_t, window_t, gui_create_window(), gui_dispatch_event(), gui_get_focused_widget(), gui_hit_test(), gui_hit_test_at() (+1 more)

### Community 117 - "Layout Engine"
Cohesion: 0.22
Nodes (7): widget_t, window_t, wm_layout_mode_t, workspace_t, layout_widget_recursive(), ui_layout_pass(), ui_set_layout_mode()

### Community 118 - "Ext4 Filesystem"
Cohesion: 0.36
Nodes (8): vfs_node_t, ext4_create_file(), ext4_init(), ext4_mount(), ext4_read(), ext4_readdir(), ext4_unlink(), ext4_write()

### Community 119 - "Taskbar Drawing"
Cohesion: 0.36
Nodes (6): blend_color(), draw_char(), draw_rect(), draw_rect_alpha(), draw_string(), draw_taskbar()

### Community 121 - "HID Keyboard"
Cohesion: 0.33
Nodes (7): hid_device_t, key_event_t, hid_kbd_process_report(), kbd_dispatch_event(), map_keysym_to_unicode(), process_dead_key(), main()

### Community 122 - "Notification Daemon"
Cohesion: 0.22
Nodes (7): sys_notify_t, notification_dismiss(), notification_peek(), notification_send(), sys_notify_dismiss(), sys_notify_peek(), notification_t

### Community 123 - "WiFi Network Device"
Cohesion: 0.32
Nodes (7): wifi_hw_send_packet(), net_device_t, pci_device_t, wifi_get_device(), wifi_handle_packet(), wifi_hw_init(), wifi_init()

### Community 124 - "Physical Memory Manager"
Cohesion: 0.43
Nodes (7): bitmap_clear(), bitmap_set(), bitmap_test(), pmm_free_page(), pmm_init(), vmm_destroy_address_space(), memory_free_physical()

### Community 125 - "Sysfs Filesystem"
Cohesion: 0.39
Nodes (7): vfs_node_t, sysfs_class_read(), sysfs_devices_read(), sysfs_finddir(), sysfs_init(), sysfs_power_read(), sysfs_readdir()

### Community 127 - "GUI Manager Renderer"
Cohesion: 0.32
Nodes (7): unique_ptr, release_renderer, set_renderer, RendererInfo, unique_ptr, GUIManager::release_renderer(), GUIManager::set_renderer()

### Community 128 - "Widget Hit Testing"
Cohesion: 0.25
Nodes (5): Widget, hit_test_all, Point, x, y

### Community 129 - "Keyboard Shortcuts"
Cohesion: 0.29
Nodes (7): hid_kbd_init(), keyboard_layout_init(), key_event_t, default_action_terminal(), keyboard_shortcuts_init(), keyboard_shortcuts_process_event(), keyboard_shortcuts_register()

### Community 130 - "Module System"
Cohesion: 0.29
Nodes (3): register_dummy_module(), register_kernel_module(), kernel_module_t

### Community 131 - "Test Python Scripts"
Cohesion: 0.50
Nodes (6): decode(), check_want(), main(), send(), type_cmd(), wait_sock()

### Community 132 - "Scientific Calculator"
Cohesion: 0.50
Nodes (7): parse_int(), pow_int(), print_int(), print_uint(), sqrt_int(), _start(), strcmp()

### Community 133 - "Terminal Rendering"
Cohesion: 0.43
Nodes (8): atoi64(), term_apply_sgr(), term_clear(), term_color(), term_put_cell(), term_putc(), term_ring(), term_shift_up()

### Community 134 - "Search Utility"
Cohesion: 0.43
Nodes (7): contains_substring(), main(), my_strlen(), print(), search_dir(), _start(), strcmp()

### Community 135 - "App Launcher Registry"
Cohesion: 0.38
Nodes (5): app_index_entry_t, About App Icon, app_launcher_get_recent(), app_launcher_search(), contains_substring()

### Community 136 - "Framebuffer Syscalls"
Cohesion: 0.38
Nodes (6): kernel/syscall.c syscall_table Array, fb_info_t Struct, sys_fb_info Syscall, sys_fb_mmap Syscall, sys_input_fd Syscall, Desktop Syscalls SYS_FB_MMAP=200 / SYS_INPUT_FD=201 / SYS_FB_INFO=202

### Community 137 - "ShadowFS"
Cohesion: 0.43
Nodes (6): vfs_node_t, shadowfs_finddir(), shadowfs_init(), shadowfs_read(), shadowfs_readdir(), shadowfs_write()

### Community 138 - "Screensaver Engine"
Cohesion: 0.38
Nodes (3): screensaver_engine_tick(), ss_clear(), ss_draw_ball()

### Community 139 - "System Diagnostics Apps"
Cohesion: 0.33
Nodes (7): Memory Viewer Application, Memory Viewer App Icon, System Diagnostics Tool Family, Process Monitor Application, Process Monitor BMP Icon Tile (/icons/process_monitor.bmp), Process Monitor App Icon (PNG), System Monitor App Icon (PNG)

### Community 140 - "Init Services"
Cohesion: 0.33
Nodes (3): service_manager_start_unit(), service_manager_stop_unit(), service_unit_t

### Community 141 - "Keyboard Layouts"
Cohesion: 0.33
Nodes (3): translate_scancode_to_keysym(), keyboard_layout_get_normal(), keyboard_layout_get_shift()

### Community 142 - "Input Multiplexer"
Cohesion: 0.38
Nodes (4): input_dev_t, input_multiplexer_allocate_device(), input_multiplexer_register_device(), input_multiplexer_report_event()

### Community 143 - "TarFS"
Cohesion: 0.48
Nodes (6): vfs_node_t, parse_size(), tarfs_finddir(), tarfs_init(), tarfs_read(), tarfs_readdir()

### Community 144 - "Settings Daemon"
Cohesion: 0.33
Nodes (6): power_config_t, theme_config_t, settingsd_apply_power_profile(), settingsd_apply_theme(), settingsd_init(), simple_strcpy()

### Community 145 - "Text Input Widget"
Cohesion: 0.48
Nodes (6): textinput_t, textinput_draw(), textinput_get_text(), textinput_handle_key(), textinput_init(), textinput_set_placeholder()

### Community 146 - "Browser HTTP Rendering"
Cohesion: 0.29
Nodes (7): br_htonl(), br_htons(), br_parse_ip(), browser_http_get(), browser_render_html(), memcpy(), strncmp()

### Community 148 - "Bluetooth Device Layer"
Cohesion: 0.47
Nodes (4): bt_device_t, bluetooth_receive_packet(), bluetooth_register_device(), bluetooth_send_packet()

### Community 149 - "GUI Render Headers"
Cohesion: 0.40
Nodes (3): Color, alpha_blend(), dim()

### Community 150 - "UI Font Rendering"
Cohesion: 0.53
Nodes (5): font_t, blend(), font_load(), font_render(), pixel_at()

### Community 151 - "FAT32 Filesystem"
Cohesion: 0.40
Nodes (3): vfs_node_t, fat32_read(), fat32_write()

### Community 152 - "Trackpad Input"
Cohesion: 0.33
Nodes (5): hid_kbd_repeat_tick(), input_push(), trackpad_init(), trackpad_process_report(), trackpad_report_t

### Community 154 - "Color Picker"
Cohesion: 0.47
Nodes (4): widget_t, color_picker_create(), color_picker_get_color(), color_picker_set_color()

### Community 155 - "QEMU Boot Smoke Test"
Cohesion: 0.40
Nodes (5): Headless QEMU Run Step (make run-nox), qemu_output.txt (Boot Log), GRUB Multiboot2 ISO Boot (grub.cfg), kernel_main Boot Logging, QEMU Boot Smoke Test (AHCI/XHCI/HDA)

### Community 156 - "ACPI AML Interpreter"
Cohesion: 0.50
Nodes (3): aml_context_t, aml_execute(), aml_parse()

### Community 157 - "MMap Syscalls"
Cohesion: 0.50
Nodes (4): UNUSED, mmap_init(), sys_mmap(), sys_munmap()

### Community 158 - "Build Error Logs"
Cohesion: 0.40
Nodes (5): build_output3.txt (hid_kbd Error), hid_kbd_process_report Undeclared 'ev', No Rule to Make Target '\' (os.bin), build_output4.txt (Missing Target), build_output5.txt (Missing Target)

### Community 159 - "Directory Watch"
Cohesion: 0.50
Nodes (3): dirwatch_cb_t, dirwatch_register(), dirwatch_unregister()

### Community 160 - "PCI MSI"
Cohesion: 0.60
Nodes (4): pci_msi_alloc_vectors(), pci_msi_free_vectors(), pci_msi_setup(), pci_msi_teardown()

### Community 166 - "Entropy Source"
Cohesion: 0.70
Nodes (4): entropy_get(), entropy_get_u32(), entropy_get_u64(), entropy_init()

### Community 168 - "Printk & VGA"
Cohesion: 0.60
Nodes (3): put_char(), vga_putchar(), vga_scroll()

### Community 169 - "Pipe Permissions"
Cohesion: 0.40
Nodes (5): check_permissions(), pipe_read_func(), pipe_write_func(), sb_acquire(), vfs_node_t

### Community 171 - "WM Window Core"
Cohesion: 0.60
Nodes (4): window_t, window_create(), window_destroy(), window_draw()

### Community 172 - "Ramdisk"
Cohesion: 0.67
Nodes (3): UNUSED, ramdisk_read(), ramdisk_write()

### Community 173 - "GDT Setup"
Cohesion: 1.00
Nodes (3): gdt_init(), gdt_set_gate(), gdt_set_tss()

### Community 174 - "Sys Wait4 Errors"
Cohesion: 0.83
Nodes (4): build_output2.txt (sys_wait4 Error), sys_wait4 Conflicting Types Error, include/wait.h sys_wait4 Declaration, WNOHANG Undeclared Error

### Community 175 - "Clock App Icons"
Cohesion: 0.67
Nodes (4): Clock App / Time Display, Clock App Icon (clock.png, 512x512), App Launcher Clock Entry, Clock App Icon (BMP counterpart)

### Community 177 - "Fortune App"
Cohesion: 0.67
Nodes (4): Fortune App Icon, App Index Table Entry (fortune), Desktop Fortune Window Data, Fortune CLI (_start)

### Community 178 - "Hex Viewer Icons"
Cohesion: 0.67
Nodes (4): Hex Viewer BMP Icon Tile (userland/icons/hex_viewer.bmp), Icon BMP Generation Script (gen_icons.py), Hex Viewer Application, Hex Viewer App Icon (hex_viewer.png)

### Community 184 - "Syscall Entry"
Cohesion: 0.50
Nodes (4): rdmsr(), sys_arch_prctl(), syscall_init(), wrmsr()

### Community 187 - "WiFi BSS Scan"
Cohesion: 1.00
Nodes (3): wifi_bss_alloc(), wifi_scan(), wifi_bss_t

### Community 188 - "Notepad App"
Cohesion: 0.67
Nodes (3): Notepad Text Editor App, Notepad App Icon, Icon Visual Style (unverified)

### Community 189 - "Reminders App"
Cohesion: 1.00
Nodes (3): Reminders App Icon, Reminders App Launcher Entry, Reminders Runtime Icon (BMP)

### Community 190 - "Snake Icons"
Cohesion: 0.67
Nodes (3): Snake Game App Icon, Snake Game Desktop App, Snake Icon Visual Style (pixel-art, 512x512 palette-based)

## Ambiguous Edges - Review These
- `Memory Viewer App Icon` → `System Monitor App Icon (PNG)`  [AMBIGUOUS]
  icons/system_monitor.png · relation: conceptually_related_to
- `Process Monitor App Icon (PNG)` → `System Monitor App Icon (PNG)`  [AMBIGUOUS]
  icons/system_monitor.png · relation: conceptually_related_to
- `Icon Visual Style (unverified)` → `Notepad App Icon`  [AMBIGUOUS]
  icons/notepad.png · relation: references
- `Snake Icon Visual Style (pixel-art, 512x512 palette-based)` → `Snake Game App Icon`  [AMBIGUOUS]
  icons/snake.png · relation: implements
- `Pong Video Game` → `Pong Game Icon`  [AMBIGUOUS]
  icons/pong.png · relation: conceptually_related_to
- `Settings Action (open settings UI)` → `Settings Icon`  [AMBIGUOUS]
  icons/settings.png · relation: conceptually_related_to

## Knowledge Gaps
- **318 isolated node(s):** `GUIManager`, `Compositor`, `InputRouter`, `chars_`, `COLS` (+313 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **43 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Memory Viewer App Icon` and `System Monitor App Icon (PNG)`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Process Monitor App Icon (PNG)` and `System Monitor App Icon (PNG)`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Icon Visual Style (unverified)` and `Notepad App Icon`?**
  _Edge tagged AMBIGUOUS (relation: references) - confidence is low._
- **What is the exact relationship between `Snake Icon Visual Style (pixel-art, 512x512 palette-based)` and `Snake Game App Icon`?**
  _Edge tagged AMBIGUOUS (relation: implements) - confidence is low._
- **What is the exact relationship between `Pong Video Game` and `Pong Game Icon`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `Settings Action (open settings UI)` and `Settings Icon`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Widget` connect `Widget Event System` to `ScrollView Widget`, `Toggle Widget`, `GUI C Wrappers`, `Clock Window Widget`, `Compositor & Input Router`, `Paint & 2D Drawing`, `Button Widget`, `Matrix Rain App`, `Framebuffer Drawing`, `TextBox Widget`, `GUI Framebuffer Bridge`, `Context Menu`, `Calculator Window`?**
  _High betweenness centrality (0.090) - this node is a cross-community bridge._