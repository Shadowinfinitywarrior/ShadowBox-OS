# Graph Report - .  (2026-08-11)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2611 nodes · 5682 edges · 180 communities (154 shown, 26 thin omitted)
- Extraction: 70% EXTRACTED · 30% INFERRED · 0% AMBIGUOUS · INFERRED: 1682 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `973cde58`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- sb_push
- Widget
- syscall.c
- userland/sys.h
- spin_lock_irqsave
- printk
- InputRouter
- sb_pull
- Window
- desktop_interactive.cpp
- shell.c
- kernel.h
- TextBox
- Rect
- inb
- smp.c
- workspace.c
- Compositor
- pmm_alloc_page
- cpu.c
- widget.c
- desktop.c
- storage.c
- vfs.c
- gui_wrappers.cpp
- power.c
- kmalloc
- include/sys.h
- security.c
- pci_config_read
- theme.c
- tmpfs.c
- ShadowBox Desktop Design Spec v1.0
- aslr.c
- kfree
- HexEditWindow
- hal/memory.c
- device.h
- Widget.hpp
- Label
- DirtyList
- tcp.c
- spin_lock
- freestanding.c
- ScrollView
- TetrisWindow
- task.c
- Button
- SnakeWindow
- device_init
- errno.h
- fb.c
- ShadowBox OS README
- hid_kbd_process_report
- sched.c
- file_manager.c
- audio/hda.c
- driver.c
- desktop main() Event Loop
- net.c
- camera.c
- gui_widget_t
- spinlock.h
- kobject
- time.c
- sys_wait4
- pty.c
- wm_render_all
- entropy.c
- media_player_pcm.c
- Headless QEMU Launch
- devfs.c
- network_manager_cli.c
- procfs.c
- net.h
- evdev.c
- peripheral.c
- hid.c
- yield
- ext4.c
- taskbar.c
- kstring.c
- _start
- idt.c
- tty_read
- Makefile 'No rule to make target' Error
- gui_input_t
- SysmonWindow
- hid_kbd_init
- module.c
- shadowfs.c
- screensaver.c
- main
- gui_window_t
- service.c
- input_multiplexer.c
- tarfs.c
- settingsd.c
- textinput.c
- _start
- main
- search.c
- app_launcher.c
- fat32.c
- gui_scrollview_t
- input_push
- numa.c
- aml.c
- swap.c
- dirwatch_register
- pci_msi.c
- calendar.c
- notification.c
- printk.c
- window.c
- xhci.c
- font_t
- process_security_check_cap
- ipv6.c
- Kimi K3 Agent Skill Bundle (Moonshot AI)
- nx_test.c
- fb_double.c
- alloc_wrapper.c
- test_tar.c
- false.c
- true.c
- gen_isr.sh
- Userland fb_info_t (mirrors kernel)
- init_script.sh
- Test File 1 (Hello World)
- pci_device_t
- CMake Thin Wrapper
- hrtimer_t
- input_event_t
- mem_zone_t
- slab_page_t
- tp_contact_t
- tp_raw_slot_t
- window_t
- sb_msg_t

## God Nodes (most connected - your core abstractions)
1. `printk()` - 143 edges
2. `sb_push()` - 112 edges
3. `Widget` - 98 edges
4. `kmalloc()` - 79 edges
5. `Window` - 76 edges
6. `get_current_process()` - 59 edges
7. `_start()` - 56 edges
8. `sb_pull()` - 54 edges
9. `sb_terminate()` - 50 edges
10. `TextBox` - 46 edges

## Surprising Connections (you probably didn't know these)
- `C++ Compositor Layer (gui/cpp/)` --semantically_similar_to--> `Compositor`  [INFERRED] [semantically similar]
  ROADMAP.md → docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md
- `sys_input_fd (SYS_INPUT_FD=201)` --calls--> `process_fd_install()`  [EXTRACTED]
  docs/superpowers/plans/2026-07-18-shadowbox-phase1-foundation.md → kernel/task.c
- `ShadowBox Desktop Design Spec v1.0` --references--> `fb_init()`  [AMBIGUOUS]
  docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md → arch/x86_64/drivers/fb.c
- `kernel_main Boot Banner` --references--> `kernel_main()`  [EXTRACTED]
  qemu_output.txt → kernel/main.c
- `spi_master_register()` --calls--> `driver_register()`  [INFERRED]
  drivers/bus/spi_master.c → kernel/driver.c

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **DE Server Components** — docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_compositor, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_window_manager, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_input_router, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_app_launcher, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_taskbar, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_notification_manager, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_settings_backend, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_file_associations [EXTRACTED 1.00]
- **Phase 1 Kernel/Userland Boundary Syscalls** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_fb_mmap, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_input_fd, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_fb_info, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_devfs_register_input [EXTRACTED 1.00]
- **Userland DE Main Event Loop (input to blit)** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_desktop_main, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_input_read_event, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_handle_move, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_handle_click, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_render_all, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_fb_blit [INFERRED 0.85]

## Communities (180 total, 26 thin omitted)

### Community 0 - "sb_push"
Cohesion: 0.03
Nodes (68): parse_size(), print(), print(), print_uint(), _start(), print(), print_uint(), _start() (+60 more)

### Community 1 - "Widget"
Cohesion: 0.05
Nodes (53): EventFn, EventType, Button::Button(), on_mouse_move, on_mouse_press, on_mouse_release, ContextMenu, add_item (+45 more)

### Community 2 - "syscall.c"
Cohesion: 0.06
Nodes (60): is_user_range(), rdmsr(), rtc_to_unix(), sb_pull(), sb_push(), sb_release(), sb_replicate(), sb_socket_recvfrom_wrapper() (+52 more)

### Community 3 - "userland/sys.h"
Cohesion: 0.06
Nodes (51): sb_msg_t, window_t, draw_sysmon(), atoi_octal(), main(), print(), atoi(), main() (+43 more)

### Community 4 - "spin_lock_irqsave"
Cohesion: 0.06
Nodes (46): percpu_rq_steal(), buddy_add_to_list(), buddy_alloc(), buddy_free(), buddy_init(), buddy_remove_from_list(), pmm_get_info(), spin_lock_irqsave() (+38 more)

### Community 5 - "printk"
Cohesion: 0.08
Nodes (35): pci_device_t, virtio_net_init(), bt_device_t, device_t, i2c_hid_init(), i2c_hid_probe(), i2c_hid_remove(), device_t (+27 more)

### Community 6 - "InputRouter"
Cohesion: 0.06
Nodes (43): Compositor, InputRouter, input_router_create(), input_router_destroy(), input_router_key_press(), input_router_key_release(), input_router_mouse_absolute(), input_router_mouse_packet() (+35 more)

### Community 7 - "sb_pull"
Cohesion: 0.11
Nodes (48): main(), print(), print_uint(), read_line(), _start(), create_window(), _start(), create_window() (+40 more)

### Community 8 - "Window"
Cohesion: 0.05
Nodes (51): hit_test_all, Point, x, y, raise_to_top, close_btn_clicked(), Color, VoidFn (+43 more)

### Community 9 - "desktop_interactive.cpp"
Cohesion: 0.11
Nodes (22): ClockWindow::ClockWindow(), tick, fmt_time(), InputRouter, InputRouter, MouseCursor, router_, SettingsPanel::SettingsPanel() (+14 more)

### Community 10 - "shell.c"
Cohesion: 0.10
Nodes (47): add_history(), atoi(), cmd_change_dir(), cmd_create_dir(), cmd_date(), cmd_delete(), cmd_df(), cmd_dmesg() (+39 more)

### Community 12 - "kernel.h"
Cohesion: 0.05
Nodes (21): acpi_init(), UNUSED, ramdisk_init(), ramdisk_read(), ramdisk_write(), net_device_t, virtio_net_send_packet(), UNUSED (+13 more)

### Community 13 - "TextBox"
Cohesion: 0.07
Nodes (36): ChangeFn, Color, TextBox, bg_focused, bg_normal, border_focused, border_normal, buf_ (+28 more)

### Community 14 - "Rect"
Cohesion: 0.12
Nodes (33): blend(), fb_draw_rect(), fb_draw_rect_round(), fb_draw_text(), fb_draw_text_wrap(), fb_fill_rect(), fb_fill_rect_round(), fb_text_width() (+25 more)

### Community 15 - "inb"
Cohesion: 0.10
Nodes (38): keyboard_handler(), ps2_has_keyboard_byte(), mouse_handler(), mouse_init(), mouse_process_byte(), mouse_read_ack(), mouse_write(), mouse_write_with_retry() (+30 more)

### Community 16 - "smp.c"
Cohesion: 0.10
Nodes (33): apic_count_cpus(), apic_init(), ioapic_route_irq(), ioapic_set_entry(), ioapic_write(), lapic_enable(), lapic_eoi(), lapic_read() (+25 more)

### Community 17 - "workspace.c"
Cohesion: 0.06
Nodes (22): Compositor, window_t, wm_layout_mode_t, workspace_t, ui_layout_pass(), ui_set_layout_mode(), xdg_toplevel_t, wm_animate_window_close() (+14 more)

### Community 18 - "Compositor"
Cohesion: 0.10
Nodes (29): fb_blit_rect(), gui_blit_screen(), gui_fb_flip(), Compositor, add_dirty, add_root, animate_roots, backbuf (+21 more)

### Community 19 - "pmm_alloc_page"
Cohesion: 0.13
Nodes (32): ahci_read(), ahci_write(), UNUSED, page_fault_handler(), switch_to_user_mode (arch/x86_64/kernel/switch.c), expand_heap(), malloc_init(), split_block() (+24 more)

### Community 20 - "cpu.c"
Cohesion: 0.07
Nodes (12): cpu_info_t, cpu_registers_t, hal_status_t, cpu_get_info(), cpu_get_vendor_string(), cpu_halt(), cpu_init(), cpu_restore_registers() (+4 more)

### Community 21 - "widget.c"
Cohesion: 0.10
Nodes (28): Cursor State Tracking, Event Dispatch Updates, GUI Interaction Improvement Plan, Hit Testing Improvements, widget_set_focus() Helper, libinput_event_t, widget_t, color_picker_create() (+20 more)

### Community 22 - "desktop.c"
Cohesion: 0.15
Nodes (29): blend_color(), blend_color(), window_t, draw_char(), draw_cursor(), draw_desktop(), draw_drop_shadow(), draw_number() (+21 more)

### Community 23 - "storage.c"
Cohesion: 0.11
Nodes (26): block_get_device(), block_init(), block_read(), block_register_device(), block_write(), block_device_t, block_to_storage_type(), block_device_t (+18 more)

### Community 24 - "vfs.c"
Cohesion: 0.14
Nodes (28): devfs_register_input() (/dev/input), sb_acquire(), sb_morph(), sys_chdir(), vfs_node_t, vfs_close(), vfs_create(), vfs_finddir() (+20 more)

### Community 25 - "gui_wrappers.cpp"
Cohesion: 0.11
Nodes (26): gui_button_t, gui_context_menu_t, button_click_trampoline(), gui_button_create(), gui_button_destroy(), gui_button_set_label(), gui_button_set_on_clicked(), gui_button_set_pos() (+18 more)

### Community 26 - "power.c"
Cohesion: 0.09
Nodes (18): acpi_fadt_t, c_state_t, acpi_fadt_checksum(), parse_acpi_tables(), power_get_c_state(), power_get_p_state(), power_init(), power_reboot() (+10 more)

### Community 27 - "kmalloc"
Cohesion: 0.15
Nodes (25): kmalloc(), slab_alloc(), slab_create_cache(), slab_free(), slab_init(), vmm_create_address_space(), vmm_fork_address_space(), vfs_node_t (+17 more)

### Community 28 - "include/sys.h"
Cohesion: 0.15
Nodes (26): sb_msg_t, sb_acquire(), sb_ipc_call(), sb_ipc_reply_wait(), sb_morph(), sb_pull(), sb_push(), sb_release() (+18 more)

### Community 29 - "security.c"
Cohesion: 0.14
Nodes (22): kernel_cap_t, cap_and(), cap_clear(), cap_isclear(), cap_isfull(), cap_lower(), cap_or(), cap_raise() (+14 more)

### Community 30 - "pci_config_read"
Cohesion: 0.13
Nodes (26): ahci_init(), check_type(), port_rebase(), start_cmd(), stop_cmd(), net_device_t, pci_device_t, e1000_init() (+18 more)

### Community 31 - "theme.c"
Cohesion: 0.10
Nodes (17): bezier_curve_t, spring_physics_t, animation_engine_init(), animation_engine_set_hz(), animation_step_bezier(), animation_step_spring(), animation_viewer_init(), animation_viewer_tick() (+9 more)

### Community 32 - "tmpfs.c"
Cohesion: 0.19
Nodes (24): vfs_node_t, tmpfs_access(), tmpfs_create_entry(), tmpfs_create_file_entry(), tmpfs_create_func(), tmpfs_find_child(), tmpfs_finddir(), tmpfs_finddir_func() (+16 more)

### Community 33 - "ShadowBox Desktop Design Spec v1.0"
Cohesion: 0.10
Nodes (25): fb_init(), Double-Buffered Compositing (userland), 8x8 Bitmap Font (font8x8), Phase 1 Userland DE Foundation Plan, Syscall Numbering (SYS_FB_MMAP=200, SYS_INPUT_FD=201, SYS_FB_INFO=202), App Launcher, App-to-DE Socket Protocol, Compositor (+17 more)

### Community 34 - "aslr.c"
Cohesion: 0.14
Nodes (19): gdt_init(), gdt_init_ap(), gdt_set_gate(), gdt_set_tss(), tss_set_stack(), aslr_enable_smap(), aslr_enable_smep(), aslr_get_heap_base() (+11 more)

### Community 35 - "kfree"
Cohesion: 0.34
Nodes (23): kfree(), ext2_inode_t, vfs_node_t, ext2_add_dir_entry(), ext2_alloc_block_from_group(), ext2_allocate_block(), ext2_allocate_inode(), ext2_create_file() (+15 more)

### Community 36 - "HexEditWindow"
Cohesion: 0.11
Nodes (21): HexEditWindow, BYTES_PER_ROW, cursor_, data_, file_size_, load_file, MAX_FILE_SIZE, on_key_press (+13 more)

### Community 37 - "hal/memory.c"
Cohesion: 0.07
Nodes (20): pmm_total_pages(), dram_info_t, hal_arch_t, hal_status_t, hal_get_arch(), hal_init(), hal_shutdown(), hal_status_t (+12 more)

### Community 38 - "device.h"
Cohesion: 0.16
Nodes (8): device_t, spi_master_probe(), spi_master_register(), spi_master_remove(), device_t, usb_hub_driver_init(), usb_hub_probe(), usb_hub_remove()

### Community 40 - "Label"
Cohesion: 0.11
Nodes (16): ClockWindow, ticks_, time_label_, Color, Label, align_, fg_, FONT_H_ (+8 more)

### Community 41 - "DirtyList"
Cohesion: 0.20
Nodes (7): alpha_blend(), dim(), DirtyList, count, MAX, rects, Color

### Community 42 - "tcp.c"
Cohesion: 0.27
Nodes (20): net_device_t, tcp_checksum(), tcp_congestion_control(), tcp_get_dev(), tcp_handle_packet(), tcp_send_ack(), tcp_send_data(), tcp_send_fin() (+12 more)

### Community 43 - "spin_lock"
Cohesion: 0.17
Nodes (19): audit_entry_t, battery_status_t, spin_lock(), spin_unlock(), irq_monitor_init(), irq_monitor_print_stats(), irq_monitor_record(), power_battery_get() (+11 more)

### Community 44 - "freestanding.c"
Cohesion: 0.14
Nodes (14): _align_up(), free(), malloc(), memcpy(), realloc(), _sbrk(), strcmp(), HexEditWindow::HexEditWindow() (+6 more)

### Community 45 - "ScrollView"
Cohesion: 0.14
Nodes (17): KernelConfigView, content_, scroll_view_, Color, ScrollView, clamp_scroll, content_, MIN_THUMB (+9 more)

### Community 46 - "TetrisWindow"
Cohesion: 0.18
Nodes (19): rng_next(), TetrisWindow, board_, collides, current_piece_, lines_, lock_piece, on_key_press (+11 more)

### Community 47 - "task.c"
Cohesion: 0.12
Nodes (13): check_and_deliver_signals(), send_signal(), sig_default_term(), signal_init(), sys_sigaction(), sys_sigprocmask(), sb_terminate(), sb_terminate_group() (+5 more)

### Community 48 - "Button"
Cohesion: 0.08
Nodes (32): Button, bg_hover, bg_normal, bg_press, border_col, corner_radius, fg_hover, fg_normal (+24 more)

### Community 49 - "SnakeWindow"
Cohesion: 0.15
Nodes (17): rng_next(), SnakeWindow, food_x_, food_y_, GRID_SIZE, MAX_SNAKE, on_key_press, reset (+9 more)

### Community 50 - "device_init"
Cohesion: 0.40
Nodes (14): boot_stage_begin(), boot_stage_end(), boot_stages_summary(), read_tick(), arch_init(), bootloader_info_parse(), device_init(), initrd_mount() (+6 more)

### Community 51 - "errno.h"
Cohesion: 0.15
Nodes (8): init_main(), _start(), start_service(), copy_from_user(), copy_to_user(), validate_user_ptr(), sys_wait4(), user_ptr_t

### Community 52 - "fb.c"
Cohesion: 0.30
Nodes (14): fb_double_init(), fb_console_init(), fb_console_putchar(), fb_get_addr(), fb_get_bpp(), fb_get_height(), fb_get_info(), fb_get_phys() (+6 more)

### Community 53 - "ShadowBox OS README"
Cohesion: 0.14
Nodes (16): Userland desktop.elf DE Server, Custom x86_64 Kernel, Device Drivers, Filesystem Layer (VFS + tarfs/devfs/tmpfs/ext2/ext4/fat32), Graphical Desktop (18 Built-in Apps), HAL Layer (kernel/hal/), Memory Management Stack, Scheduling & IPC (+8 more)

### Community 54 - "hid_kbd_process_report"
Cohesion: 0.18
Nodes (10): hid_device_t, key_event_t, hid_kbd_process_report(), kbd_dispatch_event(), map_keysym_to_unicode(), process_dead_key(), translate_scancode_to_keysym(), keyboard_layout_get_normal() (+2 more)

### Community 55 - "sched.c"
Cohesion: 0.20
Nodes (14): heapify_down(), heapify_up(), nice_to_weight(), sched_balance_runqueues(), sched_enqueue_on(), sched_pick_next(), sched_remove(), sched_steal_task() (+6 more)

### Community 56 - "file_manager.c"
Cohesion: 0.27
Nodes (14): compare_entries(), copy_file(), create_directory(), create_file(), delete_entry(), filter_entries(), list_directory(), readline() (+6 more)

### Community 57 - "audio/hda.c"
Cohesion: 0.20
Nodes (12): audio_format_t, audio_hda_init(), audio_device_t, hda_close_stream(), hda_open_stream(), hda_read_pcm(), hda_write_pcm(), audio_register_device() (+4 more)

### Community 58 - "driver.c"
Cohesion: 0.16
Nodes (19): bus_type_t, device_t, driver_t, i2c_master_init(), i2c_master_match(), i2c_master_probe(), i2c_master_remove(), bus_add_device() (+11 more)

### Community 59 - "desktop main() Event Loop"
Cohesion: 0.13
Nodes (7): desktop main() Event Loop, Userland input_event_t (mirrors kernel), input_init(), input_push() Ring Buffer Writer, input_read_event(), sys_input_fd (SYS_INPUT_FD=201), wm_handle_click()

### Community 60 - "net.c"
Cohesion: 0.26
Nodes (13): arp_handle_packet(), net_device_t, icmp_handle_packet(), ip_handle_packet(), net_checksum(), net_handle_packet(), net_device_t, udp_handle_packet() (+5 more)

### Community 61 - "camera.c"
Cohesion: 0.25
Nodes (13): cam_device_t, cam_format_t, cam_frame_t, cam_close(), cam_dequeue_frame(), cam_enqueue_frame(), cam_open(), cam_set_format() (+5 more)

### Community 62 - "gui_widget_t"
Cohesion: 0.18
Nodes (14): gui_comp_t, gui_compositor_add_root(), gui_compositor_create(), gui_compositor_destroy(), gui_compositor_focused(), gui_compositor_frame(), gui_compositor_remove_root(), gui_compositor_set_backbuf() (+6 more)

### Community 64 - "kobject"
Cohesion: 0.08
Nodes (25): vfs_node_t, kobject, attr_count, attrs, bin_attrs, kset, name, parent (+17 more)

### Community 65 - "time.c"
Cohesion: 0.21
Nodes (11): hid_kbd_repeat_tick(), get_ms_time(), get_ns_time(), ktime_get(), ktime_get_ns(), ktime_get_real_ns(), ktime_get_ts(), mdelay() (+3 more)

### Community 66 - "sys_wait4"
Cohesion: 0.18
Nodes (13): Build Log (kernel/syscall.o Failure), Build Log (kernel/font.o Failure), font8x16 Excess-Initializer Failure, offsetof Macro Redefinition Warnings, Implicit sb_socket_* Declarations, sys_wait4 Conflicting Types Error, syscall_table[100] Overwrite Warning, WNOHANG Undeclared Error (+5 more)

### Community 69 - "pty.c"
Cohesion: 0.21
Nodes (8): vfs_node_t, pty_create(), pty_read(), pty_subsystem_init(), pty_vfs_read(), pty_vfs_write(), pty_write(), sys_pty_create()

### Community 70 - "wm_render_all"
Cohesion: 0.23
Nodes (11): draw_desktop(), draw_rect(), draw_rect_outline(), draw_taskbar(), fb_get_info() (kernel fb.c), fb_init() (userland), font_draw_char(), font_draw_string() (+3 more)

### Community 71 - "entropy.c"
Cohesion: 0.70
Nodes (4): entropy_get(), entropy_get_u32(), entropy_get_u64(), entropy_init()

### Community 72 - "media_player_pcm.c"
Cohesion: 0.39
Nodes (11): add_to_playlist(), next_track(), play_track(), prev_track(), print_uint(), readline(), show_playlist(), show_track_info() (+3 more)

### Community 73 - "Headless QEMU Launch"
Cohesion: 0.22
Nodes (11): CI Build Job, build_all Target, default Target, run_all Target, Building ShadowBox OS Guide, Make Targets (build/run/clean, os.iso), QEMU Boot Log, GRUB Multiboot2 ISO Layout (grub.cfg) (+3 more)

### Community 74 - "devfs.c"
Cohesion: 0.29
Nodes (10): vfs_node_t, dev_input_read(), dev_null_read(), dev_null_write(), dev_zero_read(), dev_zero_write(), devfs_finddir(), devfs_init() (+2 more)

### Community 75 - "network_manager_cli.c"
Cohesion: 0.35
Nodes (10): cmd_connect(), cmd_diagnose(), cmd_disconnect(), cmd_help(), cmd_list(), cmd_ping(), cmd_status(), main() (+2 more)

### Community 76 - "procfs.c"
Cohesion: 0.33
Nodes (9): vfs_node_t, proc_cpuinfo_read(), proc_filesystems_read(), proc_meminfo_read(), proc_stat_read(), proc_version_read(), procfs_finddir(), procfs_init() (+1 more)

### Community 78 - "evdev.c"
Cohesion: 0.29
Nodes (8): input_dev_t, input_event_t, input_allocate_device(), input_poll_event(), input_push(), input_register_device(), input_report_event(), input_sync()

### Community 79 - "peripheral.c"
Cohesion: 0.31
Nodes (9): hal_status_t, i2c_init(), peripheral_enumerate(), peripheral_init(), peripheral_read_config(), peripheral_write_config(), spi_init(), peripheral_device_t (+1 more)

### Community 80 - "hid.c"
Cohesion: 0.38
Nodes (9): hid_device_t, hid_core_init(), hid_pointer_init(), hid_pointer_process_report(), hid_process_report(), hid_register_device(), hid_touchpad_init(), hid_touchpad_process_report() (+1 more)

### Community 81 - "yield"
Cohesion: 0.29
Nodes (9): find_process(), sys_sb_ipc_call(), sys_sb_ipc_reply_wait(), vfs_node_t, check_permissions(), pipe_read_func(), pipe_write_func(), sys_sched_yield() (+1 more)

### Community 82 - "ext4.c"
Cohesion: 0.36
Nodes (8): vfs_node_t, ext4_create_file(), ext4_init(), ext4_mount(), ext4_read(), ext4_readdir(), ext4_unlink(), ext4_write()

### Community 83 - "taskbar.c"
Cohesion: 0.36
Nodes (6): blend_color(), draw_char(), draw_rect(), draw_rect_alpha(), draw_string(), draw_taskbar()

### Community 85 - "_start"
Cohesion: 0.42
Nodes (8): parse_int(), pow_int(), print(), print_int(), print_uint(), sqrt_int(), _start(), strcmp()

### Community 86 - "idt.c"
Cohesion: 0.67
Nodes (3): idt_init(), idt_init_ap(), idt_set_gate()

### Community 87 - "tty_read"
Cohesion: 0.36
Nodes (7): serial_read_char(), serial_received(), vfs_node_t, tty_echo_char(), tty_init(), tty_read(), tty_write()

### Community 88 - "Makefile 'No rule to make target' Error"
Cohesion: 0.29
Nodes (8): Build Log (hid_kbd.o + Makefile Rule Error), Build Log (Makefile Rule Error), Build Log (Makefile Rule Error), 'ev' Undeclared in hid_kbd_process_report, Makefile 'No rule to make target' Error, Kernel Link Command Log, os.bin Link Command, x86_64 Freestanding Toolchain Flags

### Community 90 - "gui_input_t"
Cohesion: 0.25
Nodes (8): gui_input_router_create(), gui_input_router_destroy(), gui_input_router_key_press(), gui_input_router_key_release(), gui_input_router_mouse_absolute(), gui_input_router_mouse_packet(), gui_input_router_scroll(), gui_input_t

### Community 91 - "SysmonWindow"
Cohesion: 0.32
Nodes (7): num_to_str(), strcat_custom(), SysmonWindow, cpu_label_, mem_label_, tick, tick_timer_

### Community 92 - "hid_kbd_init"
Cohesion: 0.29
Nodes (7): hid_kbd_init(), keyboard_layout_init(), key_event_t, default_action_terminal(), keyboard_shortcuts_init(), keyboard_shortcuts_process_event(), keyboard_shortcuts_register()

### Community 93 - "module.c"
Cohesion: 0.29
Nodes (3): register_dummy_module(), register_kernel_module(), kernel_module_t

### Community 94 - "shadowfs.c"
Cohesion: 0.43
Nodes (6): vfs_node_t, shadowfs_finddir(), shadowfs_init(), shadowfs_read(), shadowfs_readdir(), shadowfs_write()

### Community 96 - "screensaver.c"
Cohesion: 0.38
Nodes (3): screensaver_engine_tick(), ss_clear(), ss_draw_ball()

### Community 97 - "main"
Cohesion: 0.29
Nodes (4): sys_sbrk(), main(), _start(), wallpaper_engine_init()

### Community 98 - "gui_window_t"
Cohesion: 0.29
Nodes (7): gui_window_add_client(), gui_window_create(), gui_window_destroy(), gui_window_set_pos(), gui_window_set_size(), gui_window_set_title(), gui_window_t

### Community 99 - "service.c"
Cohesion: 0.33
Nodes (3): service_manager_start_unit(), service_manager_stop_unit(), service_unit_t

### Community 100 - "input_multiplexer.c"
Cohesion: 0.38
Nodes (4): input_dev_t, input_multiplexer_allocate_device(), input_multiplexer_register_device(), input_multiplexer_report_event()

### Community 101 - "tarfs.c"
Cohesion: 0.48
Nodes (6): vfs_node_t, parse_size(), tarfs_finddir(), tarfs_init(), tarfs_read(), tarfs_readdir()

### Community 102 - "settingsd.c"
Cohesion: 0.33
Nodes (6): power_config_t, theme_config_t, settingsd_apply_power_profile(), settingsd_apply_theme(), settingsd_init(), simple_strcpy()

### Community 103 - "textinput.c"
Cohesion: 0.48
Nodes (6): textinput_t, textinput_draw(), textinput_get_text(), textinput_handle_key(), textinput_init(), textinput_set_placeholder()

### Community 104 - "_start"
Cohesion: 0.43
Nodes (6): newline(), print(), print_hex64(), print_uint64(), _start(), sys_lseek()

### Community 105 - "main"
Cohesion: 0.43
Nodes (6): htonl(), htons(), main(), parse_ip(), print(), print_uint()

### Community 106 - "search.c"
Cohesion: 0.48
Nodes (6): contains_substring(), main(), my_strlen(), print(), search_dir(), strcmp()

### Community 107 - "app_launcher.c"
Cohesion: 0.47
Nodes (4): app_index_entry_t, app_launcher_get_recent(), app_launcher_search(), contains_substring()

### Community 109 - "fat32.c"
Cohesion: 0.40
Nodes (3): vfs_node_t, fat32_read(), fat32_write()

### Community 110 - "gui_scrollview_t"
Cohesion: 0.33
Nodes (6): gui_scrollview_create(), gui_scrollview_destroy(), gui_scrollview_set_content(), gui_scrollview_set_pos(), gui_scrollview_set_size(), gui_scrollview_t

### Community 111 - "input_push"
Cohesion: 0.33
Nodes (5): input_event_t, input_poll_event(), input_push(), trackpad_process_report(), trackpad_report_t

### Community 113 - "aml.c"
Cohesion: 0.50
Nodes (3): aml_context_t, aml_execute(), aml_parse()

### Community 115 - "swap.c"
Cohesion: 0.50
Nodes (4): UNUSED, swap_init(), swap_page_in(), swap_page_out()

### Community 116 - "dirwatch_register"
Cohesion: 0.50
Nodes (3): dirwatch_cb_t, dirwatch_register(), dirwatch_unregister()

### Community 117 - "pci_msi.c"
Cohesion: 0.60
Nodes (4): pci_msi_alloc_vectors(), pci_msi_free_vectors(), pci_msi_setup(), pci_msi_teardown()

### Community 122 - "calendar.c"
Cohesion: 0.60
Nodes (4): calendar_get_date(), calendar_get_time(), is_leap_year(), get_s_time()

### Community 124 - "notification.c"
Cohesion: 0.40
Nodes (3): notification_dismiss(), notification_send(), notification_t

### Community 125 - "printk.c"
Cohesion: 0.60
Nodes (3): put_char(), vga_putchar(), vga_scroll()

### Community 127 - "window.c"
Cohesion: 0.60
Nodes (4): window_t, window_create(), window_destroy(), window_draw()

### Community 129 - "font_t"
Cohesion: 0.67
Nodes (3): font_t, font_load(), font_render()

### Community 134 - "process_security_check_cap"
Cohesion: 0.50
Nodes (4): capable(), ns_capable(), process_security_check_cap(), process_security_check_kill()

### Community 137 - "Kimi K3 Agent Skill Bundle (Moonshot AI)"
Cohesion: 0.50
Nodes (4): Distinctive Frontend Design Guidance, Interactive React Prototype Guidance, Kimi K3 Agent Skill Bundle (Moonshot AI), 3D Object Modeling Workflow (three.js + three_d_stage)

### Community 138 - "nx_test.c"
Cohesion: 0.67
Nodes (3): main(), print(), sys_mmap_inline()

## Ambiguous Edges - Review These
- `Unified Input Pipeline` → `Cursor State Tracking`  [AMBIGUOUS]
  gui_improvement_plan.md · relation: conceptually_related_to
- `ShadowBox Desktop Design Spec v1.0` → `fb_init()`  [AMBIGUOUS]
  docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md · relation: references

## Knowledge Gaps
- **228 isolated node(s):** `gen_isr.sh script`, `bg_normal`, `bg_hover`, `bg_press`, `fg_normal` (+223 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **26 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `Unified Input Pipeline` and `Cursor State Tracking`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `ShadowBox Desktop Design Spec v1.0` and `fb_init()`?**
  _Edge tagged AMBIGUOUS (relation: references) - confidence is low._
- **Why does `Widget` connect `Widget` to `HexEditWindow`, `InputRouter`, `Widget.hpp`, `Window`, `desktop_interactive.cpp`, `Label`, `DirtyList`, `freestanding.c`, `ScrollView`, `TextBox`, `Rect`, `Button`, `SnakeWindow`, `Compositor`, `gui_wrappers.cpp`?**
  _High betweenness centrality (0.133) - this node is a cross-community bridge._
- **Why does `printk()` connect `printk` to `syscall.c`, `spin_lock_irqsave`, `kernel.h`, `inb`, `smp.c`, `pmm_alloc_page`, `cpu.c`, `storage.c`, `vfs.c`, `power.c`, `kmalloc`, `pci_config_read`, `tmpfs.c`, `ShadowBox Desktop Design Spec v1.0`, `aslr.c`, `kfree`, `hal/memory.c`, `device.h`, `spin_lock`, `task.c`, `device_init`, `fb.c`, `audio/hda.c`, `driver.c`, `camera.c`, `pty.c`, `entropy.c`, `devfs.c`, `procfs.c`, `hid.c`, `ext4.c`, `tty_read`, `hid_kbd_init`, `module.c`, `shadowfs.c`, `tarfs.c`, `swap.c`, `printk.c`?**
  _High betweenness centrality (0.045) - this node is a cross-community bridge._
- **Why does `Window` connect `Window` to `Widget`, `HexEditWindow`, `Widget.hpp`, `Label`, `desktop_interactive.cpp`, `sb_pull`, `ScrollView`, `TetrisWindow`, `Rect`, `Button`, `SnakeWindow`, `SysmonWindow`?**
  _High betweenness centrality (0.038) - this node is a cross-community bridge._
- **Are the 140 inferred relationships involving `printk()` (e.g. with `acpi_init()` and `ahci_init()`) actually correct?**
  _`printk()` has 140 INFERRED edges - model-reasoned connections that need verification._
- **Are the 110 inferred relationships involving `sb_push()` (e.g. with `print()` and `print()`) actually correct?**
  _`sb_push()` has 110 INFERRED edges - model-reasoned connections that need verification._