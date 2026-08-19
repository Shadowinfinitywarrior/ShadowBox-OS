# Graph Report - OS  (2026-08-17)

## Corpus Check
- 36 files · ~266,072 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3293 nodes · 7186 edges · 233 communities (203 shown, 30 thin omitted)
- Extraction: 76% EXTRACTED · 24% INFERRED · 0% AMBIGUOUS · INFERRED: 1712 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Userland - Hexedit
- Userland - Shell
- Userland - Sys
- Kernel - Driver
- Userland - Shell
- Widget Core State
- Labels & App Windows
- Arch - Ac97
- Widget & Button Headers
- Desktop Shell (C)
- Kernel - Entropy
- Drawing Primitives & Types
- Window Class
- Kernel - Syscall
- TextBox Component
- Compositor & Display Bridge
- Desktop Shell Logic
- Arch - Fb
- Kernel - Smp
- Fs - Ext2
- Userland - Browser
- Arch - Vmm
- Kernel - Security
- Kernel - Syscall
- Kernel - Power
- Arch - Pci
- Kernel - Signal
- Kernel - Cpu
- Userland - Login
- Kernel - Sched
- Fs - Tmpfs
- Include - Sys
- Kernel - Storage
- Arch - Wifi
- GUI Manager C API
- GUI Wrappers
- Arch - Mouse
- Kernel - Memory
- Input Router Header
- Kernel - Vfs
- Drivers - Bt Core
- Input Dispatch Impl
- Arch - Smp
- Arch - Net
- Tetris Window
- Net - Tcp
- Arch - I2S
- Kernel - Security
- Freestanding Runtime & Dirty Regions
- Userland - File Manager
- Desktop Input Handling
- DRM Display Core
- Button C Wrapper
- ScrollView Component
- Include - Color Picker
- Kernel - Ipc
- Arch - Virtio Gpu
- Drivers - I2C Master
- Compositor C Wrapper
- Calculator Window
- TextBox C Wrapper
- Snake Game Window
- Include - Net
- Arch - Aslr
- Matrix Rain Window
- Kernel - Memory
- Include - Gui Internal
- Readme.Md - Readme.Md
- Net - Bluetooth
- Shadowbox Design Spec
- GUIManager (C++)
- Kernel - Peripheral
- Desktop Icons
- Arch - Virtio Blk
- Kernel - Hid
- Desktop Corruption Recovery
- Workspace Manager
- Arch - Apic
- Kernel - Camera
- Gui - Stubs
- ScrollView C Wrapper
- Hex Editor Window
- Include - Sysfs
- Kernel - Main
- UI Theme System
- UI Widget (C)
- Drivers - Usb Audio
- Widget C Wrapper
- Kernel - Pty
- Kernel - Time
- Desktop Rendering
- Cmakelists.Txt - Cmakelists.Txt
- Userland - App Launcher
- Arch - Hdmi Audio
- Include - Sysfs
- Userland - Media Player Pcm
- UI Animation
- Docs - 2026-07-18-Shadowbox-Phase1-Foundation.Md
- Docs - 2026-07-18-Shadowbox-Phase1-Foundation.Md
- Fs - Devfs
- Gui - Gui Wrapper Contextmenu
- Userland - Desktop Interactive
- Kernel - Hid Kbd
- Ui - Layout
- Userland - Network Manager Cli
- Include - Gui Internal
- Fs - Procfs
- Gui - Gui Wrapper Label
- Input - Evdev
- Ui - Gui Impl
- Arch - Buddy
- Fs - Ext4
- Taskbar (C)
- Context Menu
- Kernel - Input
- Kernel - Notification
- Userland - Calculator Scientific
- Audio - Hda
- Build_Output3.Txt - Build Output3.Txt
- Fs - Sysfs
- Gui - Gui Manager
- Include - Gui Internal
- Kernel - Keyboard Layouts
- Kernel - Module
- Net - Udp
- Tests - Gui Drive.Py
- Userland - Desktop
- Userland - Search
- Arch - Slab
- Fs - Shadowfs
- Gui - Screensaver
- Gui - Mousecursor
- Icons - Process Monitor.Png
- Init - Service
- Kernel - Input Multiplexer
- Kernel - Bcache
- Kernel - Tarfs
- Userland - Settingsd
- Ui - Textinput
- Ui - Font
- Fs - Fat32
- Gui - Demo Main
- Kernel - Calendar
- Kernel - Numa
- Kernel - Aml
- Fs - Directory Watch
- Drivers - Pci Msi
- Gui - Wallpaper Engine
- Kernel - Syscall
- Wm - Window
- Arch - Ramdisk
- Arch - Gdt
- Icons - Clock.Png
- Drivers - Xhci
- Wm - Decorations
- Icons - Fortune.Png
- Icons - Hex Viewer.Png
- Kernel - Syscall
- Net - Ipv6
- Userland - Nx Test
- Wm - Wm Animate
- Icons - Notepad.Png
- Docs - 2026-07-18-Shadowbox-Phase1-Foundation.Md
- Icons - Reminders.Png
- Icons - Snake.Png
- Test_Tar.C - Test Tar
- Userland - True
- Arch - Gen Isr.Sh
- Icons - Text Editor.Png
- Icons - Font Viewer.Png
- Icons - Mandelbrot.Png
- Icons - Paint.Png
- Icons - Pong.Png
- Icons - Settings.Png
- Init - Init Script.Sh
- Patch_Keyboard.Sh - Patch Keyboard.Sh
- Misc - Misc
- Misc - Misc
- Icons - Browser.Png
- Icons - Calculator.Png
- Icons - Image Viewer.Png
- Icons - Keyboard Power.Png
- Icons - Matrix Rain.Png
- Icons - Package Manager.Png
- Icons - Tetris.Png
- Misc - Misc
- Misc - Misc
- Misc - Misc

## God Nodes (most connected - your core abstractions)
1. `printk()` - 179 edges
2. `sb_push()` - 125 edges
3. `Widget` - 96 edges
4. `_start()` - 89 edges
5. `kmalloc()` - 81 edges
6. `sb_pull()` - 73 edges
7. `Window` - 68 edges
8. `sb_release()` - 66 edges
9. `sb_acquire()` - 63 edges
10. `sb_terminate()` - 61 edges

## Surprising Connections (you probably didn't know these)
- `ext4_init()` --calls--> `printk()`  [INFERRED]
  fs/ext4.c → kernel/printk.c
- `rtc_init()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/rtc.c → kernel/printk.c
- `fb_init()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/fb.c → kernel/printk.c
- `hdmi_audio_irq_handler()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/hdmi_audio.c → kernel/printk.c
- `i2s_set_volume()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/i2s.c → kernel/printk.c

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Kernel Build -> Link -> Boot Validation Flow** — _github_workflows_ci, cmakelists_makefile, link_cmd_os_bin_link, qemu_output_qemu_smoke [INFERRED 0.85]
- **Unified Input Subsystem (PS/2/HID/trackpad -> ring buffer -> /dev/input -> userland)** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_input_ring_buffer, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_input_event_t, readme_unified_input_pipeline [INFERRED 0.85]
- **Userland Desktop Environment (desktop.elf)** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_desktop_elf, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_window_manager, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_double_buffering, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_shadowbox_desktop, readme_graphical_desktop [INFERRED 0.85]

## Communities (233 total, 30 thin omitted)

### Community 0 - "Userland - Hexedit"
Cohesion: 0.03
Nodes (79): parse_size(), print(), print(), print(), print_uint(), _start(), print(), print_uint() (+71 more)

### Community 1 - "Userland - Shell"
Cohesion: 0.06
Nodes (95): add_history(), alias_get(), alias_set(), atoi(), cmd_alias(), cmd_banner(), cmd_basename(), cmd_bench() (+87 more)

### Community 2 - "Userland - Sys"
Cohesion: 0.05
Nodes (67): sb_msg_t, sys_status_t, window_t, draw_sysmon(), get_num_processes(), atoi_octal(), main(), create_window() (+59 more)

### Community 3 - "Kernel - Driver"
Cohesion: 0.06
Nodes (55): bus_type_t, display_output_t, device_t, i2c_hid_init(), i2c_hid_probe(), i2c_hid_remove(), i2c_master_init(), device_t (+47 more)

### Community 4 - "Userland - Shell"
Cohesion: 0.09
Nodes (54): main(), print(), print_uint(), read_line(), _start(), icon_bmp_load(), maybe_load_wallpaper(), network_manager_create() (+46 more)

### Community 5 - "Widget Core State"
Cohesion: 0.06
Nodes (37): EventFn, Button::Button(), CalculatorWindow::CalculatorWindow(), ClockWindow::ClockWindow(), KernelConfigView, content_, scroll_view_, Label::Label() (+29 more)

### Community 6 - "Labels & App Windows"
Cohesion: 0.05
Nodes (33): ClockWindow, tick, ticks_, time_label_, fmt_time(), Color, Label, align_ (+25 more)

### Community 8 - "Arch - Ac97"
Cohesion: 0.08
Nodes (43): ac97_device_t, ac97_codec_reset(), ac97_configure_mixer(), ac97_create_audio_dev(), ac97_detect_codecs(), ac97_init(), ac97_irq_handler(), ac97_mixer_read() (+35 more)

### Community 9 - "Widget & Button Headers"
Cohesion: 0.07
Nodes (40): Button, bg_hover, bg_normal, bg_press, border_col, corner_radius, fg_hover, fg_normal (+32 more)

### Community 10 - "Desktop Shell (C)"
Cohesion: 0.11
Nodes (51): blend_color(), draw_app_window(), draw_bell_icon(), draw_bt_icon(), draw_button(), draw_char(), draw_circle(), draw_cursor() (+43 more)

### Community 11 - "Kernel - Entropy"
Cohesion: 0.05
Nodes (20): acpi_init(), mouse_start_poll_thread(), UNUSED, mmap_init(), sys_mmap(), sys_munmap(), UNUSED, swap_init() (+12 more)

### Community 12 - "Drawing Primitives & Types"
Cohesion: 0.08
Nodes (36): blend(), fb_draw_rect(), fb_draw_rect_round(), fb_draw_text(), fb_draw_text_wrap(), fb_fill_rect(), fb_fill_rect_round(), fb_text_width() (+28 more)

### Community 13 - "Window Class"
Cohesion: 0.05
Nodes (47): add_child, grow_children, close_btn_clicked(), Color, VoidFn, min_btn_clicked(), Window, add_client (+39 more)

### Community 14 - "Kernel - Syscall"
Cohesion: 0.05
Nodes (9): pipe_count_inc(), rtc_to_unix(), sb_pull(), sb_push(), sb_socket_recvfrom_wrapper(), sb_socket_sendto_wrapper(), sys_dup(), sys_dup2() (+1 more)

### Community 15 - "TextBox Component"
Cohesion: 0.06
Nodes (35): ChangeFn, Color, TextBox, bg_focused, bg_normal, border_focused, border_normal, buf_ (+27 more)

### Community 16 - "Compositor & Display Bridge"
Cohesion: 0.09
Nodes (30): fb_blit_rect(), gui_blit_screen(), gui_fb_flip(), sys_fb_flip(), Compositor, add_dirty, add_root, animate_roots (+22 more)

### Community 17 - "Desktop Shell Logic"
Cohesion: 0.14
Nodes (36): create_window(), editor_insert_char(), editor_load(), editor_newline(), editor_save(), fortune_next(), g2048_can_move(), g2048_init() (+28 more)

### Community 18 - "Arch - Fb"
Cohesion: 0.12
Nodes (28): fb_double_free(), fb_double_init(), fb_console_init(), fb_console_putchar(), fb_get_addr(), fb_get_bpp(), fb_get_height(), fb_get_pitch() (+20 more)

### Community 19 - "Kernel - Smp"
Cohesion: 0.12
Nodes (30): heapify_down(), heapify_up(), percpu_rq_dequeue(), percpu_rq_enqueue(), percpu_rq_remove(), percpu_rq_steal(), swap_ptrs(), buddy_free() (+22 more)

### Community 20 - "Fs - Ext2"
Cohesion: 0.23
Nodes (29): kfree(), kmalloc(), split_block(), ext2_inode_t, vfs_node_t, ext2_add_dir_entry(), ext2_alloc_block_from_group(), ext2_allocate_block() (+21 more)

### Community 21 - "Userland - Browser"
Cohesion: 0.13
Nodes (28): http_request_t, atoi(), browser_main_loop(), browser_navigate(), draw_browser(), htonl(), htons(), http_get() (+20 more)

### Community 22 - "Arch - Vmm"
Cohesion: 0.17
Nodes (29): ahci_read(), ahci_write(), UNUSED, expand_heap(), malloc_init(), bitmap_clear(), bitmap_set(), bitmap_test() (+21 more)

### Community 23 - "Kernel - Security"
Cohesion: 0.12
Nodes (26): kernel_cap_t, cap_and(), cap_clear(), cap_isclear(), cap_isfull(), cap_lower(), cap_or(), cap_raise() (+18 more)

### Community 24 - "Kernel - Syscall"
Cohesion: 0.06
Nodes (31): is_user_range(), sb_morph(), sys_access(), sys_chdir(), sys_chmod(), sys_chown(), sys_clock_gettime(), sys_fb_info() (+23 more)

### Community 25 - "Kernel - Power"
Cohesion: 0.09
Nodes (18): acpi_fadt_t, c_state_t, acpi_fadt_checksum(), parse_acpi_tables(), power_get_c_state(), power_get_p_state(), power_init(), power_shutdown() (+10 more)

### Community 26 - "Arch - Pci"
Cohesion: 0.13
Nodes (25): ahci_init(), check_type(), port_rebase(), start_cmd(), stop_cmd(), hda_init(), pci_device_t, pci_check_device() (+17 more)

### Community 27 - "Kernel - Signal"
Cohesion: 0.12
Nodes (18): idt_init(), idt_set_gate(), isr_handler(), page_fault_handler(), find_process(), sys_sb_ipc_call(), sys_sb_ipc_reply_wait(), check_and_deliver_signals() (+10 more)

### Community 28 - "Kernel - Cpu"
Cohesion: 0.08
Nodes (9): cpu_info_t, cpu_registers_t, hal_status_t, cpu_get_info(), cpu_get_vendor_string(), cpu_init(), cpu_restore_registers(), cpu_save_registers() (+1 more)

### Community 29 - "Userland - Login"
Cohesion: 0.22
Nodes (27): u32, u64, u8, attempt_login(), blend(), draw_background(), draw_button(), draw_char() (+19 more)

### Community 30 - "Kernel - Sched"
Cohesion: 0.12
Nodes (23): tss_set_stack(), cpu_halt(), idle_loop(), idle_tasks_init(), semop(), heapify_down(), heapify_up(), nice_to_weight() (+15 more)

### Community 31 - "Fs - Tmpfs"
Cohesion: 0.19
Nodes (25): vfs_node_t, tmpfs_access(), tmpfs_create_entry(), tmpfs_create_file_entry(), tmpfs_create_func(), tmpfs_find_child(), tmpfs_finddir(), tmpfs_finddir_func() (+17 more)

### Community 32 - "Include - Sys"
Cohesion: 0.15
Nodes (26): sb_msg_t, sb_acquire(), sb_ipc_call(), sb_ipc_reply_wait(), sb_morph(), sb_pull(), sb_push(), sb_release() (+18 more)

### Community 33 - "Kernel - Storage"
Cohesion: 0.14
Nodes (23): ramdisk_init(), block_get_device(), block_init(), block_read(), block_register_device(), block_write(), block_device_t, block_to_storage_type() (+15 more)

### Community 34 - "Arch - Wifi"
Cohesion: 0.16
Nodes (23): wifi_bss_alloc(), wifi_connect(), wifi_disconnect(), wifi_eeprom_init(), wifi_get_device(), wifi_hardware_init(), wifi_hw_init(), wifi_hw_send_packet() (+15 more)

### Community 35 - "GUI Manager C API"
Cohesion: 0.13
Nodes (25): Widget, GUIManager, add_root, comp_, composite_count_, cursor_, focused, frame (+17 more)

### Community 36 - "GUI Wrappers"
Cohesion: 0.14
Nodes (24): button_click_trampoline(), gui_button_create(), gui_compositor_focused(), gui_compositor_remove_root(), gui_input_router_create(), gui_input_router_destroy(), gui_input_router_key_press(), gui_input_router_key_release() (+16 more)

### Community 37 - "Arch - Mouse"
Cohesion: 0.13
Nodes (22): kbd_translate(), keyboard_getchar(), keyboard_handle_scancode(), keyboard_handler(), keyboard_has_char(), keyboard_init(), ps2_has_keyboard_byte(), ps2_wait_write() (+14 more)

### Community 38 - "Kernel - Memory"
Cohesion: 0.09
Nodes (14): dram_info_t, hal_status_t, memory_alloc_physical(), memory_free_physical(), memory_get_dram_info(), memory_get_map(), memory_get_stats(), memory_init() (+6 more)

### Community 39 - "Input Router Header"
Cohesion: 0.08
Nodes (22): InputRouter, comp_, cursor_, KEY_BACKSPACE, KEY_DELETE, KEY_DOWN, KEY_END, KEY_ESC (+14 more)

### Community 40 - "Kernel - Vfs"
Cohesion: 0.17
Nodes (22): vfs_node_t, vfs_close(), vfs_create(), vfs_finddir(), vfs_flock(), vfs_get_inode(), vfs_get_mount(), vfs_ioctl() (+14 more)

### Community 41 - "Drivers - Bt Core"
Cohesion: 0.20
Nodes (21): bt_hci_dev_t, bt_alloc_dma(), bt_connect_le(), bt_disconnect(), bt_get_device(), bt_handle_packet(), bt_hardware_init(), bt_hci_init() (+13 more)

### Community 42 - "Input Dispatch Impl"
Cohesion: 0.13
Nodes (19): Compositor, InputRouter, input_router_create(), input_router_destroy(), input_router_key_press(), input_router_key_release(), input_router_mouse_absolute(), input_router_mouse_packet() (+11 more)

### Community 43 - "Arch - Smp"
Cohesion: 0.17
Nodes (18): apic_count_cpus(), lapic_read(), gdt_init_ap(), idt_init_ap(), ap_trampoline_copy(), current_cpu_id(), percpu_rq_init(), read_lapic_id() (+10 more)

### Community 44 - "Arch - Net"
Cohesion: 0.17
Nodes (18): net_device_t, pci_device_t, e1000_init(), e1000_irq_handler(), e1000_read(), e1000_send_packet(), e1000_write(), net_device_t (+10 more)

### Community 45 - "Tetris Window"
Cohesion: 0.18
Nodes (19): rng_next(), TetrisWindow, board_, collides, current_piece_, lines_, lock_piece, on_key_press (+11 more)

### Community 46 - "Net - Tcp"
Cohesion: 0.27
Nodes (20): net_device_t, tcp_checksum(), tcp_congestion_control(), tcp_get_dev(), tcp_handle_packet(), tcp_send_ack(), tcp_send_data(), tcp_send_fin() (+12 more)

### Community 47 - "Arch - I2S"
Cohesion: 0.17
Nodes (17): pci_device_t, i2s_alloc_dma_buffer(), i2s_configure(), i2s_init(), i2s_irq_handler(), i2s_read_reg(), i2s_set_volume(), i2s_start_rx() (+9 more)

### Community 48 - "Kernel - Security"
Cohesion: 0.18
Nodes (19): audit_entry_t, battery_status_t, spin_lock(), spin_unlock(), irq_monitor_print_stats(), irq_monitor_record(), power_battery_get(), audit_get_entries() (+11 more)

### Community 49 - "Freestanding Runtime & Dirty Regions"
Cohesion: 0.14
Nodes (14): _align_up(), free(), malloc(), memcpy(), realloc(), _sbrk(), strcmp(), HexEditWindow::HexEditWindow() (+6 more)

### Community 50 - "Userland - File Manager"
Cohesion: 0.18
Nodes (18): OS Application Icon Set, File Explorer App Icon, compare_entries(), copy_file(), create_directory(), create_file(), delete_entry(), filter_entries() (+10 more)

### Community 51 - "Desktop Input Handling"
Cohesion: 0.22
Nodes (20): close_window(), fb_delete_entry(), fb_go_back(), fb_go_fwd(), fb_go_up(), fb_is_text_file(), fb_navigate(), fb_open_entry() (+12 more)

### Community 52 - "DRM Display Core"
Cohesion: 0.13
Nodes (14): vmap_phys(), drm_connector_t, drm_crtc_t, drm_driver_t, drm_gem_object_t, device_t, drm_gem_create(), drm_gem_free() (+6 more)

### Community 53 - "Button C Wrapper"
Cohesion: 0.15
Nodes (17): gui_button_t, gui_widget_t, gui_button_create(), gui_button_destroy(), gui_button_set_label(), gui_button_set_on_clicked(), gui_button_set_pos(), gui_button_set_size() (+9 more)

### Community 54 - "ScrollView Component"
Cohesion: 0.17
Nodes (15): Color, ScrollView, clamp_scroll, content_, draw_scrollbar_v, MIN_THUMB, on_mouse_scroll, paint_children (+7 more)

### Community 55 - "Include - Color Picker"
Cohesion: 0.12
Nodes (4): widget_t, color_picker_create(), color_picker_get_color(), color_picker_set_color()

### Community 56 - "Kernel - Ipc"
Cohesion: 0.12
Nodes (14): spinlock_init(), ipc_init(), msgget(), msgq_find(), sem_find(), semget(), shmget(), irq_monitor_init() (+6 more)

### Community 57 - "Arch - Virtio Gpu"
Cohesion: 0.25
Nodes (16): pci_device_t, virtio_gpu_add_desc(), virtio_gpu_create_resource(), virtio_gpu_flush_resource(), virtio_gpu_get_display_info(), virtio_gpu_init(), virtio_gpu_init_virtqueue(), virtio_gpu_irq_handler() (+8 more)

### Community 58 - "Drivers - I2C Master"
Cohesion: 0.16
Nodes (9): device_t, driver_t, i2c_master_match(), i2c_master_probe(), i2c_master_remove(), device_t, spi_master_probe(), spi_master_register() (+1 more)

### Community 59 - "Compositor C Wrapper"
Cohesion: 0.18
Nodes (16): gui_comp_t, gui_widget_t, gui_compositor_add_root(), gui_compositor_create(), gui_compositor_destroy(), gui_compositor_focused(), gui_compositor_frame(), gui_compositor_remove_root() (+8 more)

### Community 60 - "Calculator Window"
Cohesion: 0.22
Nodes (16): CalculatorWindow, acc_val_, append_digit, apply_op, calculate, current_op_, current_val_, display_buffer_ (+8 more)

### Community 61 - "TextBox C Wrapper"
Cohesion: 0.15
Nodes (16): gui_widget_t, gui_textbox_create(), gui_textbox_destroy(), gui_textbox_set_placeholder(), gui_textbox_set_pos(), gui_textbox_set_size(), gui_textbox_set_text(), gui_textbox_text() (+8 more)

### Community 62 - "Snake Game Window"
Cohesion: 0.16
Nodes (16): rng_next(), SnakeWindow, food_x_, food_y_, GRID_SIZE, MAX_SNAKE, on_key_press, reset (+8 more)

### Community 64 - "Arch - Aslr"
Cohesion: 0.20
Nodes (13): aslr_enable_smap(), aslr_enable_smep(), aslr_get_heap_base(), aslr_get_mmap_base(), aslr_get_stack_base(), aslr_init(), aslr_random_addr(), cpu_has_smap() (+5 more)

### Community 65 - "Matrix Rain Window"
Cohesion: 0.15
Nodes (12): Color, m_rand(), MatrixWindow, chars_, COLS, drops_, MatrixWindow::MatrixWindow(), ROWS (+4 more)

### Community 66 - "Kernel - Memory"
Cohesion: 0.15
Nodes (8): init_main(), _start(), start_service(), copy_from_user(), copy_to_user(), validate_user_ptr(), sys_wait4(), user_ptr_t

### Community 67 - "Include - Gui Internal"
Cohesion: 0.12
Nodes (9): DirtyList, count, MAX, rects, Rect, h, w, x (+1 more)

### Community 68 - "Readme.Md - Readme.Md"
Cohesion: 0.16
Nodes (14): Headless QEMU Run Step (make run-nox), make / os.iso Build Commands, ShadowBox Phase 1 Foundation Plan, qemu_output.txt (Boot Log), GRUB Multiboot2 ISO Boot (grub.cfg), kernel_main Boot Logging, QEMU Boot Smoke Test (AHCI/XHCI/HDA), make Build/Run Targets (+6 more)

### Community 69 - "Net - Bluetooth"
Cohesion: 0.18
Nodes (14): bt_connection_t, bt_device_t, bt_l2cap_channel_t, sys_sys_status(), bluetooth_device_count(), bluetooth_handle_acl_packet(), bluetooth_init(), bluetooth_receive_packet() (+6 more)

### Community 70 - "Shadowbox Design Spec"
Cohesion: 0.19
Nodes (16): ShadowBox Desktop Design Spec v1.0, App-to-DE Domain Socket Protocol, App Launcher, Compositor, Dirty-Rect Tracking, File Explorer, Input Router, Notification Manager (+8 more)

### Community 71 - "GUIManager (C++)"
Cohesion: 0.15
Nodes (5): Widget, GUIManager::add_root(), GUIManager::focused(), GUIManager::remove_root(), GUIManager::set_focus()

### Community 72 - "Kernel - Peripheral"
Cohesion: 0.17
Nodes (14): hal_arch_t, hal_status_t, hal_get_arch(), hal_init(), hal_shutdown(), hal_status_t, i2c_init(), peripheral_enumerate() (+6 more)

### Community 73 - "Desktop Icons"
Cohesion: 0.27
Nodes (14): blit_icon(), draw_desktop_icons(), draw_icon_folder(), draw_procedural(), icon_bmp_blit(), icon_calc(), icon_editor(), icon_folder() (+6 more)

### Community 74 - "Arch - Virtio Blk"
Cohesion: 0.32
Nodes (14): pci_device_t, vblk_add_desc(), vblk_do_request(), vblk_init_block_device(), vblk_init_virtqueue(), vblk_negotiate_features(), vblk_read_reg(), vblk_reset_device() (+6 more)

### Community 75 - "Kernel - Hid"
Cohesion: 0.22
Nodes (13): hid_device_t, hid_core_init(), hid_pointer_init(), hid_pointer_process_report(), hid_process_report(), hid_register_device(), hid_touchpad_init(), hid_touchpad_process_report() (+5 more)

### Community 76 - "Desktop Corruption Recovery"
Cohesion: 0.33
Nodes (13): blend_color(), window_t, draw_char(), draw_cursor(), draw_desktop(), draw_drop_shadow(), draw_number(), draw_rect() (+5 more)

### Community 77 - "Workspace Manager"
Cohesion: 0.21
Nodes (13): wm_layout_mode_t, workspace_t, xdg_toplevel_t, find_workspace_by_id(), wm_add_window_to_active(), wm_animate_window_close(), wm_animate_window_open(), wm_get_active_workspace() (+5 more)

### Community 78 - "Arch - Apic"
Cohesion: 0.24
Nodes (11): apic_init(), ioapic_route_irq(), ioapic_set_entry(), ioapic_write(), lapic_enable(), lapic_eoi(), lapic_write(), hid_kbd_repeat_tick() (+3 more)

### Community 79 - "Kernel - Camera"
Cohesion: 0.25
Nodes (13): cam_device_t, cam_format_t, cam_frame_t, cam_close(), cam_dequeue_frame(), cam_enqueue_frame(), cam_open(), cam_set_format() (+5 more)

### Community 80 - "Gui - Stubs"
Cohesion: 0.19
Nodes (8): gui_clear_screen(), gui_draw_filled_rect(), gui_draw_pixel(), gui_draw_rect(), clear_screen(), draw_hline(), draw_pixel(), draw_vline()

### Community 81 - "ScrollView C Wrapper"
Cohesion: 0.20
Nodes (12): gui_widget_t, gui_scrollview_create(), gui_scrollview_destroy(), gui_scrollview_set_content(), gui_scrollview_set_pos(), gui_scrollview_set_size(), gui_scrollview_create(), gui_scrollview_destroy() (+4 more)

### Community 82 - "Hex Editor Window"
Cohesion: 0.20
Nodes (12): HexEditWindow, BYTES_PER_ROW, cursor_, data_, file_size_, load_file, MAX_FILE_SIZE, on_key_press (+4 more)

### Community 84 - "Include - Sysfs"
Cohesion: 0.14
Nodes (13): kset, kobj, list, parent, sysfs_attribute, mode, name, private (+5 more)

### Community 85 - "Kernel - Main"
Cohesion: 0.24
Nodes (12): arch_init(), bootloader_info_parse(), device_init(), init_user_thread(), initrd_mount(), kernel_main(), kernel_subsys_init(), mm_init() (+4 more)

### Community 86 - "UI Theme System"
Cohesion: 0.21
Nodes (10): theme_config_t, hex_to_color(), ui_theme_apply(), ui_theme_get_accent_color(), ui_theme_get_background_color(), ui_theme_get_current(), ui_theme_get_foreground_color(), ui_theme_get_primary_color() (+2 more)

### Community 87 - "UI Widget (C)"
Cohesion: 0.26
Nodes (13): widget_t, window_t, gui_mark_damage(), gui_paint_pass(), gui_update_cursor(), gui_widget_get_focused(), gui_widget_set_focus(), widget_create() (+5 more)

### Community 88 - "Drivers - Usb Audio"
Cohesion: 0.28
Nodes (12): audio_device_t, device_t, uac_alloc_device(), uac_create_audio_dev(), uac_parse_descriptors(), uac_probe(), uac_remove(), usb_audio_irq_handler() (+4 more)

### Community 89 - "Widget C Wrapper"
Cohesion: 0.31
Nodes (11): gui_bool_t, gui_widget_t, gui_widget_add_child(), gui_widget_enabled(), gui_widget_focused(), gui_widget_hovered(), gui_widget_parent(), gui_widget_pressed() (+3 more)

### Community 90 - "Kernel - Pty"
Cohesion: 0.21
Nodes (8): vfs_node_t, pty_create(), pty_read(), pty_subsystem_init(), pty_vfs_read(), pty_vfs_write(), pty_write(), sys_pty_create()

### Community 91 - "Kernel - Time"
Cohesion: 0.23
Nodes (10): get_ms_time(), get_ns_time(), ktime_get(), ktime_get_ns(), ktime_get_real_ns(), ktime_get_ts(), mdelay(), msleep() (+2 more)

### Community 92 - "Desktop Rendering"
Cohesion: 0.23
Nodes (13): atoi64(), calc_click(), calc_input(), memset(), num_to_str(), paint_save_bmp(), term_apply_sgr(), term_clear() (+5 more)

### Community 93 - "Cmakelists.Txt - Cmakelists.Txt"
Cohesion: 0.21
Nodes (12): CI GitHub Actions Workflow, CMake Configure/Build CI Step, build_output.txt (font8x16 Error), font8x16 Static/Extern Declaration Conflict, CMakeLists.txt Wrapper, build_all Custom Target, Default ALL Target, Top-Level Makefile (+4 more)

### Community 94 - "Userland - App Launcher"
Cohesion: 0.20
Nodes (8): app_index_entry_t, About App Icon, Terminal App Icon (terminal.png), Makefile (build and initrd packaging), app_launcher_get_recent(), app_launcher_search(), contains_substring(), Desktop Icons (desktop_icons.c)

### Community 95 - "Arch - Hdmi Audio"
Cohesion: 0.35
Nodes (11): pci_device_t, hdmi_audio_detect_sink(), hdmi_audio_init(), hdmi_audio_irq_handler(), hdmi_audio_register_sink(), hdmi_audio_set_mode(), hdmi_detect_sink_eld(), hdmi_pin_sense_enable() (+3 more)

### Community 97 - "Include - Sysfs"
Cohesion: 0.17
Nodes (12): vfs_node_t, kobject, attr_count, attrs, bin_attrs, kset, name, parent (+4 more)

### Community 98 - "Userland - Media Player Pcm"
Cohesion: 0.39
Nodes (11): add_to_playlist(), next_track(), play_track(), prev_track(), print_uint(), readline(), show_playlist(), show_track_info() (+3 more)

### Community 99 - "UI Animation"
Cohesion: 0.24
Nodes (9): bezier_curve_t, spring_physics_t, animation_engine_init(), animation_engine_set_hz(), animation_step_bezier(), animation_step_spring(), animation_viewer_init(), animation_viewer_tick() (+1 more)

### Community 100 - "Docs - 2026-07-18-Shadowbox-Phase1-Foundation.Md"
Cohesion: 0.25
Nodes (10): build_output2.txt (sys_wait4 Error), sys_wait4 Conflicting Types Error, kernel/syscall.c syscall_table Array, include/wait.h sys_wait4 Declaration, WNOHANG Undeclared Error, fb_info_t Struct, sys_fb_info Syscall, sys_fb_mmap Syscall (+2 more)

### Community 101 - "Docs - 2026-07-18-Shadowbox-Phase1-Foundation.Md"
Cohesion: 0.33
Nodes (11): desktop.elf Userland DE Server, userland/desktop/fb.c, userland/desktop/font.c, userland/desktop/input.c, userland/desktop/main.c, desktop_thread Kernel Launch Path, userland/desktop/wm.c, Double-Buffered Compositing (+3 more)

### Community 102 - "Fs - Devfs"
Cohesion: 0.29
Nodes (10): vfs_node_t, dev_input_read(), dev_null_read(), dev_null_write(), dev_zero_read(), dev_zero_write(), devfs_finddir(), devfs_init() (+2 more)

### Community 103 - "Gui - Gui Wrapper Contextmenu"
Cohesion: 0.22
Nodes (8): gui_context_menu_t, gui_widget_t, gui_context_menu_add_item(), gui_context_menu_create(), gui_context_menu_destroy(), gui_context_menu_add_item(), gui_context_menu_create(), gui_context_menu_destroy()

### Community 104 - "Userland - Desktop Interactive"
Cohesion: 0.18
Nodes (4): Compositor, InputRouter, about_create(), init_wallpaper()

### Community 105 - "Kernel - Hid Kbd"
Cohesion: 0.25
Nodes (9): hid_device_t, key_event_t, hid_kbd_process_report(), kbd_dispatch_event(), map_keysym_to_unicode(), process_dead_key(), key_event_t, keyboard_shortcuts_process_event() (+1 more)

### Community 107 - "Ui - Layout"
Cohesion: 0.20
Nodes (8): widget_t, window_t, wm_layout_mode_t, workspace_t, layout_widget_recursive(), ui_layout_pass(), ui_set_layout_mode(), gui_layout_pass()

### Community 108 - "Userland - Network Manager Cli"
Cohesion: 0.35
Nodes (10): cmd_connect(), cmd_diagnose(), cmd_disconnect(), cmd_help(), cmd_list(), cmd_ping(), cmd_status(), main() (+2 more)

### Community 109 - "Include - Gui Internal"
Cohesion: 0.20
Nodes (10): EventType, dispatch, InputEvent, button, key, mods, pos, scroll_delta (+2 more)

### Community 110 - "Fs - Procfs"
Cohesion: 0.33
Nodes (9): vfs_node_t, proc_cpuinfo_read(), proc_filesystems_read(), proc_meminfo_read(), proc_stat_read(), proc_version_read(), procfs_finddir(), procfs_init() (+1 more)

### Community 111 - "Gui - Gui Wrapper Label"
Cohesion: 0.24
Nodes (8): gui_widget_t, gui_label_create(), gui_label_destroy(), gui_label_set_text(), gui_label_create(), gui_label_destroy(), gui_label_set_text(), gui_label_t

### Community 113 - "Input - Evdev"
Cohesion: 0.29
Nodes (8): input_dev_t, input_event_t, evdev_poll_event(), evdev_push(), input_allocate_device(), input_register_device(), input_report_event(), input_sync()

### Community 114 - "Ui - Gui Impl"
Cohesion: 0.40
Nodes (9): libinput_event_t, widget_t, window_t, gui_create_window(), gui_dispatch_event(), gui_get_focused_widget(), gui_hit_test(), gui_hit_test_at() (+1 more)

### Community 115 - "Arch - Buddy"
Cohesion: 0.28
Nodes (4): buddy_add_to_list(), buddy_alloc(), buddy_init(), buddy_remove_from_list()

### Community 116 - "Fs - Ext4"
Cohesion: 0.36
Nodes (8): vfs_node_t, ext4_create_file(), ext4_init(), ext4_mount(), ext4_read(), ext4_readdir(), ext4_unlink(), ext4_write()

### Community 117 - "Taskbar (C)"
Cohesion: 0.36
Nodes (6): blend_color(), draw_char(), draw_rect(), draw_rect_alpha(), draw_string(), draw_taskbar()

### Community 118 - "Context Menu"
Cohesion: 0.25
Nodes (7): ContextMenu, add_item, hovered_index_, item_count_, ITEM_HEIGHT, items_, MAX_ITEMS

### Community 119 - "Kernel - Input"
Cohesion: 0.22
Nodes (7): hid_kbd_repeat_tick(), input_event_t, input_poll_event(), input_push(), trackpad_init(), trackpad_process_report(), trackpad_report_t

### Community 120 - "Kernel - Notification"
Cohesion: 0.22
Nodes (8): sys_notify_t, notification_daemon_init(), notification_dismiss(), notification_peek(), notification_send(), sys_notify_dismiss(), sys_notify_peek(), notification_t

### Community 121 - "Userland - Calculator Scientific"
Cohesion: 0.42
Nodes (8): parse_int(), pow_int(), print(), print_int(), print_uint(), sqrt_int(), _start(), strcmp()

### Community 122 - "Audio - Hda"
Cohesion: 0.39
Nodes (7): audio_format_t, audio_device_t, hda_close_stream(), hda_open_stream(), hda_read_pcm(), hda_write_pcm(), audio_stream_dir_t

### Community 123 - "Build_Output3.Txt - Build Output3.Txt"
Cohesion: 0.25
Nodes (8): build_output3.txt (hid_kbd Error), hid_kbd_process_report Undeclared 'ev', No Rule to Make Target '\' (os.bin), build_output4.txt (Missing Target), build_output5.txt (Missing Target), 18 Built-In Desktop Apps, Graphical Desktop (userland/desktop.c), Unified Input Pipeline

### Community 124 - "Fs - Sysfs"
Cohesion: 0.39
Nodes (7): vfs_node_t, sysfs_class_read(), sysfs_devices_read(), sysfs_finddir(), sysfs_init(), sysfs_power_read(), sysfs_readdir()

### Community 126 - "Gui - Gui Manager"
Cohesion: 0.32
Nodes (7): unique_ptr, release_renderer, set_renderer, RendererInfo, unique_ptr, GUIManager::release_renderer(), GUIManager::set_renderer()

### Community 127 - "Include - Gui Internal"
Cohesion: 0.25
Nodes (5): Widget, hit_test_all, Point, x, y

### Community 128 - "Kernel - Keyboard Layouts"
Cohesion: 0.29
Nodes (4): translate_scancode_to_keysym(), keyboard_layout_get_normal(), keyboard_layout_get_shift(), keyboard_layout_init()

### Community 129 - "Kernel - Module"
Cohesion: 0.29
Nodes (3): register_dummy_module(), register_kernel_module(), kernel_module_t

### Community 130 - "Net - Udp"
Cohesion: 0.39
Nodes (7): net_device_t, udp_handle_packet(), udp_socket_bind(), udp_socket_create(), udp_socket_recvfrom(), udp_socket_sendto(), udp_socket_t

### Community 131 - "Tests - Gui Drive.Py"
Cohesion: 0.50
Nodes (6): decode(), check_want(), main(), send(), type_cmd(), wait_sock()

### Community 132 - "Userland - Desktop"
Cohesion: 0.29
Nodes (8): cycle_windows(), paint_dot(), paint_fill(), scroll_top_window(), _start(), top_window(), update_terminal(), update_terminals()

### Community 133 - "Userland - Search"
Cohesion: 0.43
Nodes (7): contains_substring(), main(), my_strlen(), print(), search_dir(), _start(), strcmp()

### Community 134 - "Arch - Slab"
Cohesion: 0.43
Nodes (6): slab_alloc(), slab_create_cache(), slab_free(), slab_init(), task_init(), slab_cache_t

### Community 135 - "Fs - Shadowfs"
Cohesion: 0.43
Nodes (6): vfs_node_t, shadowfs_finddir(), shadowfs_init(), shadowfs_read(), shadowfs_readdir(), shadowfs_write()

### Community 136 - "Gui - Screensaver"
Cohesion: 0.38
Nodes (3): screensaver_engine_tick(), ss_clear(), ss_draw_ball()

### Community 137 - "Gui - Mousecursor"
Cohesion: 0.33
Nodes (4): InputRouter, MouseCursor, paint_self, router_

### Community 138 - "Icons - Process Monitor.Png"
Cohesion: 0.33
Nodes (7): Memory Viewer Application, Memory Viewer App Icon, System Diagnostics Tool Family, Process Monitor Application, Process Monitor BMP Icon Tile (/icons/process_monitor.bmp), Process Monitor App Icon (PNG), System Monitor App Icon (PNG)

### Community 139 - "Init - Service"
Cohesion: 0.33
Nodes (3): service_manager_start_unit(), service_manager_stop_unit(), service_unit_t

### Community 140 - "Kernel - Input Multiplexer"
Cohesion: 0.38
Nodes (4): input_dev_t, input_multiplexer_allocate_device(), input_multiplexer_register_device(), input_multiplexer_report_event()

### Community 141 - "Kernel - Bcache"
Cohesion: 0.38
Nodes (5): bcache_flush(), bcache_read(), bcache_write(), block_device_t, block_flush()

### Community 142 - "Kernel - Tarfs"
Cohesion: 0.48
Nodes (6): vfs_node_t, parse_size(), tarfs_finddir(), tarfs_init(), tarfs_read(), tarfs_readdir()

### Community 143 - "Userland - Settingsd"
Cohesion: 0.33
Nodes (6): power_config_t, theme_config_t, settingsd_apply_power_profile(), settingsd_apply_theme(), settingsd_init(), simple_strcpy()

### Community 144 - "Ui - Textinput"
Cohesion: 0.48
Nodes (6): textinput_t, textinput_draw(), textinput_get_text(), textinput_handle_key(), textinput_init(), textinput_set_placeholder()

### Community 146 - "Ui - Font"
Cohesion: 0.53
Nodes (5): font_t, blend(), font_load(), font_render(), pixel_at()

### Community 147 - "Fs - Fat32"
Cohesion: 0.40
Nodes (3): vfs_node_t, fat32_read(), fat32_write()

### Community 148 - "Gui - Demo Main"
Cohesion: 0.47
Nodes (3): main(), _start(), GUIManager

### Community 149 - "Kernel - Calendar"
Cohesion: 0.53
Nodes (5): calendar_get_date(), calendar_get_time(), is_leap_year(), clock_gettime(), get_s_time()

### Community 151 - "Kernel - Aml"
Cohesion: 0.50
Nodes (3): aml_context_t, aml_execute(), aml_parse()

### Community 152 - "Fs - Directory Watch"
Cohesion: 0.50
Nodes (3): dirwatch_cb_t, dirwatch_register(), dirwatch_unregister()

### Community 153 - "Drivers - Pci Msi"
Cohesion: 0.60
Nodes (4): pci_msi_alloc_vectors(), pci_msi_free_vectors(), pci_msi_setup(), pci_msi_teardown()

### Community 160 - "Kernel - Syscall"
Cohesion: 0.40
Nodes (5): check_permissions(), pipe_read_func(), pipe_write_func(), sb_acquire(), vfs_node_t

### Community 162 - "Wm - Window"
Cohesion: 0.60
Nodes (4): window_t, window_create(), window_destroy(), window_draw()

### Community 163 - "Arch - Ramdisk"
Cohesion: 0.67
Nodes (3): UNUSED, ramdisk_read(), ramdisk_write()

### Community 164 - "Arch - Gdt"
Cohesion: 1.00
Nodes (3): gdt_init(), gdt_set_gate(), gdt_set_tss()

### Community 165 - "Icons - Clock.Png"
Cohesion: 0.67
Nodes (4): Clock App / Time Display, Clock App Icon (clock.png, 512x512), App Launcher Clock Entry, Clock App Icon (BMP counterpart)

### Community 168 - "Icons - Fortune.Png"
Cohesion: 0.67
Nodes (4): Fortune App Icon, App Index Table Entry (fortune), Desktop Fortune Window Data, Fortune CLI (_start)

### Community 169 - "Icons - Hex Viewer.Png"
Cohesion: 0.67
Nodes (4): Hex Viewer BMP Icon Tile (userland/icons/hex_viewer.bmp), Icon BMP Generation Script (gen_icons.py), Hex Viewer Application, Hex Viewer App Icon (hex_viewer.png)

### Community 174 - "Kernel - Syscall"
Cohesion: 0.50
Nodes (4): rdmsr(), sys_arch_prctl(), syscall_init(), wrmsr()

### Community 177 - "Userland - Nx Test"
Cohesion: 0.67
Nodes (3): main(), print(), sys_mmap_inline()

### Community 178 - "Wm - Wm Animate"
Cohesion: 0.67
Nodes (3): xdg_toplevel_t, wm_animate_window_close(), wm_animate_window_open()

### Community 179 - "Icons - Notepad.Png"
Cohesion: 0.67
Nodes (3): Notepad Text Editor App, Notepad App Icon, Icon Visual Style (unverified)

### Community 181 - "Icons - Reminders.Png"
Cohesion: 1.00
Nodes (3): Reminders App Icon, Reminders App Launcher Entry, Reminders Runtime Icon (BMP)

### Community 182 - "Icons - Snake.Png"
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
- **262 isolated node(s):** `Compositor`, `InputRouter`, `backbuf`, `dirty_`, `dirty_count_` (+257 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **30 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

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
- **Why does `Widget` connect `Widget Core State` to `Matrix Rain Window`, `GUI Wrappers`, `Labels & App Windows`, `Widget & Button Headers`, `Gui - Mousecursor`, `Drawing Primitives & Types`, `Window Class`, `TextBox Component`, `Compositor & Display Bridge`, `Freestanding Runtime & Dirty Regions`, `Context Menu`, `ScrollView Component`, `Calculator Window`?**
  _High betweenness centrality (0.073) - this node is a cross-community bridge._