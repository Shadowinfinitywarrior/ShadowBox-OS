# Graph Report - OS  (2026-08-11)

## Corpus Check
- 419 files · ~230,289 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2605 nodes · 5619 edges · 158 communities (145 shown, 13 thin omitted)
- Extraction: 71% EXTRACTED · 29% INFERRED · 0% AMBIGUOUS · INFERRED: 1616 edges (avg confidence: 0.8)
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
- InputRouter.cpp
- sb_acquire
- Window
- InputRouter
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
- apic.c
- gui_wrappers.cpp
- power.c
- sysfs.c
- include/sys.h
- security.c
- device_init
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
- block.c
- tcp.c
- gui_impl.c
- freestanding.c
- ScrollView
- TetrisWindow
- task.c
- Button
- SnakeWindow
- kernel_main
- errno.h
- fb.c
- ShadowBox OS README
- hid_kbd_process_report
- socket.c
- sb_pull
- audio/hda.c
- driver.c
- Userland input_event_t (mirrors kernel)
- ahci_init
- camera.c
- bluetooth.c
- keyboard_layouts.c
- kobject
- time.c
- Build Log (kernel/syscall.o Failure)
- pty.c
- blit_to_screen
- entropy.c
- media_player_pcm.c
- Headless QEMU Launch
- devfs.c
- network_manager_cli.c
- procfs.c
- net.h
- InputRouter.hpp
- hal_init
- hid.c
- color_picker.c
- ext4.c
- taskbar.c
- init.c
- _start
- _start
- tty.c
- Makefile 'No rule to make target' Error
- hid_kbd_init
- shadowfs.c
- screensaver.c
- sys_sbrk
- service.c
- input_multiplexer.c
- kmalloc
- settingsd.c
- textinput.c
- _start
- main
- search.c
- app_launcher.c
- input_push
- numa.c
- aml.c
- dirwatch_register
- pci_msi.c
- calendar.c
- notification.c
- window.c
- xhci.c
- font_t
- ipv6.c
- Kimi K3 Agent Skill Bundle (Moonshot AI)
- sb_terminate
- test_tar.c
- gen_isr.sh
- Userland fb_info_t (mirrors kernel)
- init_script.sh
- Test File 1 (Hello World)
- CMake Thin Wrapper

## God Nodes (most connected - your core abstractions)
1. `printk()` - 145 edges
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
- `ShadowBox Desktop Design Spec v1.0` --references--> `fb_init()`  [AMBIGUOUS]
  docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md → arch/x86_64/drivers/fb.c
- `kernel_main Boot Banner` --references--> `kernel_main()`  [EXTRACTED]
  qemu_output.txt → kernel/main.c
- `sys_input_fd (SYS_INPUT_FD=201)` --calls--> `process_fd_install()`  [EXTRACTED]
  docs/superpowers/plans/2026-07-18-shadowbox-phase1-foundation.md → kernel/task.c
- `C++ Compositor Layer (gui/cpp/)` --semantically_similar_to--> `Compositor`  [INFERRED] [semantically similar]
  ROADMAP.md → docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md
- `bootloader_info_parse()` --calls--> `fb_set_info()`  [INFERRED]
  kernel/main.c → arch/x86_64/drivers/fb.c

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **DE Server Components** — docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_compositor, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_window_manager, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_input_router, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_app_launcher, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_taskbar, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_notification_manager, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_settings_backend, docs_superpowers_specs_2026_07_17_shadowbox_desktop_design_file_associations [EXTRACTED 1.00]
- **Phase 1 Kernel/Userland Boundary Syscalls** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_fb_mmap, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_input_fd, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_sys_fb_info, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_devfs_register_input [EXTRACTED 1.00]
- **Userland DE Main Event Loop (input to blit)** — docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_desktop_main, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_input_read_event, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_handle_move, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_handle_click, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_wm_render_all, docs_superpowers_plans_2026_07_18_shadowbox_phase1_foundation_fb_blit [INFERRED 0.85]

## Communities (158 total, 13 thin omitted)

### Community 0 - "sb_push"
Cohesion: 0.04
Nodes (70): main(), parse_size(), print(), print(), print_uint(), _start(), print(), print_uint() (+62 more)

### Community 1 - "Widget"
Cohesion: 0.05
Nodes (52): EventFn, EventType, Button::Button(), on_mouse_release, Label::Label(), set_text, InputRouter, InputRouter (+44 more)

### Community 2 - "syscall.c"
Cohesion: 0.06
Nodes (61): check_permissions(), is_user_range(), rdmsr(), rtc_to_unix(), sb_acquire(), sb_morph(), sb_pull(), sb_push() (+53 more)

### Community 3 - "userland/sys.h"
Cohesion: 0.07
Nodes (41): atoi_octal(), main(), print(), atoi(), main(), print(), _start(), _start() (+33 more)

### Community 4 - "spin_lock_irqsave"
Cohesion: 0.09
Nodes (42): buddy_add_to_list(), buddy_alloc(), buddy_free(), buddy_remove_from_list(), spin_lock_irqsave(), spin_unlock_irqrestore(), pipe_close(), pipe_read() (+34 more)

### Community 5 - "printk"
Cohesion: 0.06
Nodes (46): buddy_init(), slab_alloc(), slab_create_cache(), slab_free(), slab_init(), device_t, i2c_hid_init(), i2c_hid_probe() (+38 more)

### Community 6 - "InputRouter.cpp"
Cohesion: 0.14
Nodes (20): Compositor, InputRouter, input_router_create(), input_router_destroy(), input_router_key_press(), input_router_key_release(), input_router_mouse_absolute(), input_router_mouse_packet() (+12 more)

### Community 7 - "sb_acquire"
Cohesion: 0.16
Nodes (21): window_t, draw_sysmon(), create_window(), _start(), create_window(), maybe_load_wallpaper(), network_manager_create(), _start() (+13 more)

### Community 8 - "Window"
Cohesion: 0.05
Nodes (59): hit_test_all, Point, x, y, raise_to_top, close_btn_clicked(), Color, VoidFn (+51 more)

### Community 9 - "InputRouter"
Cohesion: 0.08
Nodes (22): InputRouter, comp_, cursor_, KEY_BACKSPACE, KEY_DELETE, KEY_DOWN, KEY_END, KEY_ESC (+14 more)

### Community 10 - "shell.c"
Cohesion: 0.09
Nodes (54): add_history(), atoi(), cmd_base64(), cmd_change_dir(), cmd_cp(), cmd_create_dir(), cmd_date(), cmd_delete() (+46 more)

### Community 12 - "kernel.h"
Cohesion: 0.06
Nodes (10): acpi_init(), UNUSED, mmap_init(), sys_mmap(), sys_munmap(), UNUSED, swap_init(), swap_page_in() (+2 more)

### Community 13 - "TextBox"
Cohesion: 0.05
Nodes (49): ChangeFn, ContextMenu, add_item, hovered_index_, item_count_, ITEM_HEIGHT, items_, MAX_ITEMS (+41 more)

### Community 14 - "Rect"
Cohesion: 0.08
Nodes (41): blend(), fb_draw_rect(), fb_draw_rect_round(), fb_draw_text(), fb_draw_text_wrap(), fb_fill_rect(), fb_fill_rect_round(), fb_text_width() (+33 more)

### Community 15 - "inb"
Cohesion: 0.09
Nodes (43): keyboard_getchar(), keyboard_handler(), keyboard_has_char(), keyboard_init(), ps2_has_keyboard_byte(), mouse_handler(), mouse_init(), mouse_process_byte() (+35 more)

### Community 16 - "smp.c"
Cohesion: 0.10
Nodes (30): apic_count_cpus(), lapic_read(), gdt_init(), gdt_init_ap(), gdt_set_gate(), gdt_set_tss(), tss_set_stack(), idt_init() (+22 more)

### Community 17 - "workspace.c"
Cohesion: 0.06
Nodes (22): Compositor, window_t, wm_layout_mode_t, workspace_t, ui_layout_pass(), ui_set_layout_mode(), xdg_toplevel_t, wm_animate_window_close() (+14 more)

### Community 18 - "Compositor"
Cohesion: 0.12
Nodes (25): Compositor, add_dirty, add_root, animate_roots, backbuf, collect_dirty, compositor_create(), compositor_destroy() (+17 more)

### Community 19 - "pmm_alloc_page"
Cohesion: 0.09
Nodes (46): ahci_read(), ahci_write(), UNUSED, isr_handler(), page_fault_handler(), switch_to_user_mode (arch/x86_64/kernel/switch.c), expand_heap(), malloc_init() (+38 more)

### Community 20 - "cpu.c"
Cohesion: 0.07
Nodes (11): cpu_info_t, cpu_registers_t, hal_status_t, cpu_get_info(), cpu_get_vendor_string(), cpu_halt(), cpu_init(), cpu_restore_registers() (+3 more)

### Community 21 - "widget.c"
Cohesion: 0.24
Nodes (12): widget_set_focus() Helper, widget_t, window_t, gui_layout_pass(), gui_mark_damage(), gui_paint_pass(), widget_create(), widget_destroy() (+4 more)

### Community 22 - "desktop.c"
Cohesion: 0.15
Nodes (29): blend_color(), window_t, blend_color(), window_t, draw_char(), draw_cursor(), draw_desktop(), draw_drop_shadow() (+21 more)

### Community 23 - "storage.c"
Cohesion: 0.21
Nodes (16): block_to_storage_type(), block_device_t, hal_status_t, create_storage_device(), storage_find_device(), storage_flush(), storage_get_devices(), storage_get_info() (+8 more)

### Community 24 - "apic.c"
Cohesion: 0.18
Nodes (13): apic_init(), ioapic_route_irq(), ioapic_set_entry(), ioapic_write(), lapic_enable(), lapic_eoi(), lapic_write(), pit_handler() (+5 more)

### Community 25 - "gui_wrappers.cpp"
Cohesion: 0.06
Nodes (61): gui_button_t, gui_comp_t, gui_context_menu_t, button_click_trampoline(), gui_button_create(), gui_button_destroy(), gui_button_set_label(), gui_button_set_on_clicked() (+53 more)

### Community 26 - "power.c"
Cohesion: 0.10
Nodes (11): acpi_fadt_t, c_state_t, acpi_fadt_checksum(), power_get_c_state(), power_get_p_state(), power_subsys_init(), power_suspend(), power_thermal_init() (+3 more)

### Community 27 - "sysfs.c"
Cohesion: 0.48
Nodes (6): vfs_node_t, sysfs_class_read(), sysfs_devices_read(), sysfs_finddir(), sysfs_power_read(), sysfs_readdir()

### Community 28 - "include/sys.h"
Cohesion: 0.15
Nodes (26): sb_msg_t, sb_acquire(), sb_ipc_call(), sb_ipc_reply_wait(), sb_morph(), sb_pull(), sb_push(), sb_release() (+18 more)

### Community 29 - "security.c"
Cohesion: 0.06
Nodes (47): audit_entry_t, battery_status_t, spin_lock(), spin_unlock(), kernel_cap_t, irq_monitor_print_stats(), irq_monitor_record(), power_battery_get() (+39 more)

### Community 30 - "device_init"
Cohesion: 0.14
Nodes (24): net_device_t, pci_device_t, e1000_init(), e1000_irq_handler(), e1000_read(), e1000_send_packet(), e1000_write(), fb_init() (+16 more)

### Community 31 - "theme.c"
Cohesion: 0.10
Nodes (17): bezier_curve_t, spring_physics_t, animation_engine_init(), animation_engine_set_hz(), animation_step_bezier(), animation_step_spring(), animation_viewer_init(), animation_viewer_tick() (+9 more)

### Community 32 - "tmpfs.c"
Cohesion: 0.21
Nodes (22): vfs_node_t, tmpfs_access(), tmpfs_create_file_entry(), tmpfs_create_func(), tmpfs_find_child(), tmpfs_finddir(), tmpfs_finddir_func(), tmpfs_make_vfs_node() (+14 more)

### Community 33 - "ShadowBox Desktop Design Spec v1.0"
Cohesion: 0.11
Nodes (24): Double-Buffered Compositing (userland), 8x8 Bitmap Font (font8x8), Phase 1 Userland DE Foundation Plan, Syscall Numbering (SYS_FB_MMAP=200, SYS_INPUT_FD=201, SYS_FB_INFO=202), App Launcher, App-to-DE Socket Protocol, Compositor, ShadowBox Desktop Design Spec v1.0 (+16 more)

### Community 34 - "aslr.c"
Cohesion: 0.20
Nodes (13): aslr_enable_smap(), aslr_enable_smep(), aslr_get_heap_base(), aslr_get_mmap_base(), aslr_get_stack_base(), aslr_init(), aslr_random_addr(), cpu_has_smap() (+5 more)

### Community 35 - "kfree"
Cohesion: 0.37
Nodes (21): kfree(), ext2_inode_t, vfs_node_t, ext2_add_dir_entry(), ext2_alloc_block_from_group(), ext2_allocate_block(), ext2_allocate_inode(), ext2_create_file() (+13 more)

### Community 36 - "HexEditWindow"
Cohesion: 0.10
Nodes (20): HexEditWindow, BYTES_PER_ROW, cursor_, data_, file_size_, load_file, MAX_FILE_SIZE, on_key_press (+12 more)

### Community 37 - "hal/memory.c"
Cohesion: 0.11
Nodes (10): dram_info_t, hal_status_t, memory_alloc_physical(), memory_get_dram_info(), memory_get_map(), memory_get_stats(), memory_set(), memory_zero() (+2 more)

### Community 38 - "device.h"
Cohesion: 0.12
Nodes (13): device_t, driver_t, i2c_master_match(), i2c_master_probe(), i2c_master_remove(), device_t, spi_master_probe(), spi_master_register() (+5 more)

### Community 39 - "Widget.hpp"
Cohesion: 0.09
Nodes (20): ClockWindow, ClockWindow::ClockWindow(), tick, ticks_, time_label_, fmt_time(), main(), _start() (+12 more)

### Community 40 - "Label"
Cohesion: 0.13
Nodes (13): Color, Label, align_, fg_, FONT_H_, FONT_W_, MAX_TEXT, scale_ (+5 more)

### Community 41 - "block.c"
Cohesion: 0.17
Nodes (13): UNUSED, ramdisk_init(), ramdisk_read(), ramdisk_write(), block_get_device(), block_read(), block_register_device(), block_write() (+5 more)

### Community 42 - "tcp.c"
Cohesion: 0.09
Nodes (42): net_device_t, pci_device_t, virtio_net_init(), virtio_net_send_packet(), arp_handle_packet(), net_device_t, icmp_handle_packet(), ip_handle_packet() (+34 more)

### Community 43 - "gui_impl.c"
Cohesion: 0.32
Nodes (11): Event Dispatch Updates, GUI Interaction Improvement Plan, Hit Testing Improvements, libinput_event_t, widget_t, window_t, gui_create_window(), gui_dispatch_event() (+3 more)

### Community 44 - "freestanding.c"
Cohesion: 0.14
Nodes (14): _align_up(), free(), malloc(), memcpy(), realloc(), _sbrk(), strcmp(), HexEditWindow::HexEditWindow() (+6 more)

### Community 45 - "ScrollView"
Cohesion: 0.12
Nodes (17): KernelConfigView, content_, scroll_view_, Color, ScrollView, clamp_scroll, content_, MIN_THUMB (+9 more)

### Community 46 - "TetrisWindow"
Cohesion: 0.18
Nodes (19): rng_next(), TetrisWindow, board_, collides, current_piece_, lines_, lock_piece, on_key_press (+11 more)

### Community 47 - "task.c"
Cohesion: 0.08
Nodes (30): percpu_rq_steal(), heapify_down(), heapify_up(), nice_to_weight(), sched_balance_runqueues(), sched_enqueue(), sched_enqueue_on(), sched_pick_next() (+22 more)

### Community 48 - "Button"
Cohesion: 0.09
Nodes (29): Button, bg_hover, bg_normal, bg_press, border_col, corner_radius, fg_hover, fg_normal (+21 more)

### Community 49 - "SnakeWindow"
Cohesion: 0.16
Nodes (16): rng_next(), SnakeWindow, food_x_, food_y_, GRID_SIZE, MAX_SNAKE, on_key_press, reset (+8 more)

### Community 50 - "kernel_main"
Cohesion: 0.22
Nodes (20): block_init(), devfs_init(), procfs_init(), sysfs_init(), boot_stage_begin(), boot_stage_end(), boot_stages_summary(), read_tick() (+12 more)

### Community 51 - "errno.h"
Cohesion: 0.08
Nodes (21): msgget(), msgq_find(), msgrcv(), msgsnd(), sem_find(), semget(), semop(), shmat() (+13 more)

### Community 52 - "fb.c"
Cohesion: 0.07
Nodes (33): fb_double_free(), fb_double_init(), fb_console_init(), fb_console_putchar(), fb_get_addr(), fb_get_bpp(), fb_get_height(), fb_get_info() (+25 more)

### Community 53 - "ShadowBox OS README"
Cohesion: 0.14
Nodes (16): Userland desktop.elf DE Server, Custom x86_64 Kernel, Device Drivers, Filesystem Layer (VFS + tarfs/devfs/tmpfs/ext2/ext4/fat32), Graphical Desktop (18 Built-in Apps), HAL Layer (kernel/hal/), Memory Management Stack, Scheduling & IPC (+8 more)

### Community 54 - "hid_kbd_process_report"
Cohesion: 0.33
Nodes (7): hid_device_t, key_event_t, hid_kbd_process_report(), kbd_dispatch_event(), map_keysym_to_unicode(), process_dead_key(), main()

### Community 55 - "socket.c"
Cohesion: 0.18
Nodes (11): sb_socket_accept_wrapper(), sb_socket_bind_wrapper(), sb_socket_connect_wrapper(), sb_socket_listen_wrapper(), vfs_node_t, sb_socket_accept(), sb_socket_bind(), sb_socket_connect() (+3 more)

### Community 56 - "sb_pull"
Cohesion: 0.19
Nodes (20): print(), print_uint(), read_line(), _start(), compare_entries(), copy_file(), create_directory(), create_file() (+12 more)

### Community 57 - "audio/hda.c"
Cohesion: 0.20
Nodes (12): audio_format_t, audio_hda_init(), audio_device_t, hda_close_stream(), hda_open_stream(), hda_read_pcm(), hda_write_pcm(), audio_register_device() (+4 more)

### Community 58 - "driver.c"
Cohesion: 0.25
Nodes (14): bus_type_t, i2c_master_init(), bus_add_device(), bus_add_driver(), bus_register(), bus_unregister(), device_t, driver_t (+6 more)

### Community 59 - "Userland input_event_t (mirrors kernel)"
Cohesion: 0.25
Nodes (7): devfs_register_input() (/dev/input), Userland input_event_t (mirrors kernel), input_push() Ring Buffer Writer, input_read_event(), Cursor State Tracking, input_event_t Protocol, Unified Input Pipeline

### Community 60 - "ahci_init"
Cohesion: 0.62
Nodes (6): ahci_init(), check_type(), port_rebase(), start_cmd(), stop_cmd(), HBA_PORT

### Community 61 - "camera.c"
Cohesion: 0.25
Nodes (13): cam_device_t, cam_format_t, cam_frame_t, cam_close(), cam_dequeue_frame(), cam_enqueue_frame(), cam_open(), cam_set_format() (+5 more)

### Community 62 - "bluetooth.c"
Cohesion: 0.38
Nodes (6): bt_device_t, bluetooth_cleanup(), bluetooth_init(), bluetooth_receive_packet(), bluetooth_register_device(), bluetooth_send_packet()

### Community 63 - "keyboard_layouts.c"
Cohesion: 0.33
Nodes (3): translate_scancode_to_keysym(), keyboard_layout_get_normal(), keyboard_layout_get_shift()

### Community 64 - "kobject"
Cohesion: 0.08
Nodes (25): vfs_node_t, kobject, attr_count, attrs, bin_attrs, kset, name, parent (+17 more)

### Community 65 - "time.c"
Cohesion: 0.23
Nodes (10): get_ms_time(), get_ns_time(), ktime_get(), ktime_get_ns(), ktime_get_real_ns(), ktime_get_ts(), mdelay(), msleep() (+2 more)

### Community 66 - "Build Log (kernel/syscall.o Failure)"
Cohesion: 0.18
Nodes (12): Build Log (kernel/syscall.o Failure), Build Log (kernel/font.o Failure), font8x16 Excess-Initializer Failure, offsetof Macro Redefinition Warnings, Implicit sb_socket_* Declarations, sys_wait4 Conflicting Types Error, syscall_table[100] Overwrite Warning, WNOHANG Undeclared Error (+4 more)

### Community 69 - "pty.c"
Cohesion: 0.06
Nodes (22): vfs_node_t, fat32_read(), fat32_write(), input_dev_t, input_event_t, input_allocate_device(), input_poll_event(), input_push() (+14 more)

### Community 70 - "blit_to_screen"
Cohesion: 0.40
Nodes (4): fb_blit_rect(), gui_blit_screen(), gui_fb_flip(), blit_to_screen

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
Cohesion: 0.39
Nodes (8): vfs_node_t, dev_input_read(), dev_null_read(), dev_null_write(), dev_zero_read(), dev_zero_write(), devfs_finddir(), devfs_readdir()

### Community 75 - "network_manager_cli.c"
Cohesion: 0.35
Nodes (10): cmd_connect(), cmd_diagnose(), cmd_disconnect(), cmd_help(), cmd_list(), cmd_ping(), cmd_status(), main() (+2 more)

### Community 76 - "procfs.c"
Cohesion: 0.39
Nodes (8): vfs_node_t, proc_cpuinfo_read(), proc_filesystems_read(), proc_meminfo_read(), proc_stat_read(), proc_version_read(), procfs_finddir(), procfs_readdir()

### Community 79 - "hal_init"
Cohesion: 0.17
Nodes (14): hal_arch_t, hal_status_t, hal_get_arch(), hal_init(), hal_shutdown(), hal_status_t, i2c_init(), peripheral_enumerate() (+6 more)

### Community 80 - "hid.c"
Cohesion: 0.38
Nodes (9): hid_device_t, hid_core_init(), hid_pointer_init(), hid_pointer_process_report(), hid_process_report(), hid_register_device(), hid_touchpad_init(), hid_touchpad_process_report() (+1 more)

### Community 81 - "color_picker.c"
Cohesion: 0.47
Nodes (4): widget_t, color_picker_create(), color_picker_get_color(), color_picker_set_color()

### Community 82 - "ext4.c"
Cohesion: 0.36
Nodes (8): vfs_node_t, ext4_create_file(), ext4_init(), ext4_mount(), ext4_read(), ext4_readdir(), ext4_unlink(), ext4_write()

### Community 83 - "taskbar.c"
Cohesion: 0.36
Nodes (6): blend_color(), draw_char(), draw_rect(), draw_rect_alpha(), draw_string(), draw_taskbar()

### Community 84 - "init.c"
Cohesion: 0.83
Nodes (3): init_main(), _start(), start_service()

### Community 85 - "_start"
Cohesion: 0.42
Nodes (8): parse_int(), pow_int(), print(), print_int(), print_uint(), sqrt_int(), _start(), strcmp()

### Community 87 - "tty.c"
Cohesion: 0.40
Nodes (4): vfs_node_t, tty_echo_char(), tty_init(), tty_write()

### Community 88 - "Makefile 'No rule to make target' Error"
Cohesion: 0.29
Nodes (8): Build Log (hid_kbd.o + Makefile Rule Error), Build Log (Makefile Rule Error), Build Log (Makefile Rule Error), 'ev' Undeclared in hid_kbd_process_report, Makefile 'No rule to make target' Error, Kernel Link Command Log, os.bin Link Command, x86_64 Freestanding Toolchain Flags

### Community 92 - "hid_kbd_init"
Cohesion: 0.29
Nodes (7): hid_kbd_init(), keyboard_layout_init(), key_event_t, default_action_terminal(), keyboard_shortcuts_init(), keyboard_shortcuts_process_event(), keyboard_shortcuts_register()

### Community 94 - "shadowfs.c"
Cohesion: 0.43
Nodes (6): vfs_node_t, shadowfs_finddir(), shadowfs_init(), shadowfs_read(), shadowfs_readdir(), shadowfs_write()

### Community 96 - "screensaver.c"
Cohesion: 0.38
Nodes (3): screensaver_engine_tick(), ss_clear(), ss_draw_ball()

### Community 99 - "service.c"
Cohesion: 0.33
Nodes (3): service_manager_start_unit(), service_manager_stop_unit(), service_unit_t

### Community 100 - "input_multiplexer.c"
Cohesion: 0.38
Nodes (4): input_dev_t, input_multiplexer_allocate_device(), input_multiplexer_register_device(), input_multiplexer_report_event()

### Community 101 - "kmalloc"
Cohesion: 0.16
Nodes (15): kmalloc(), tmpfs_create_entry(), tmpfs_init(), tmpfs_write(), free(), malloc(), vfs_node_t, pipe_create() (+7 more)

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

### Community 111 - "input_push"
Cohesion: 0.25
Nodes (6): hid_kbd_repeat_tick(), input_event_t, input_poll_event(), input_push(), trackpad_process_report(), trackpad_report_t

### Community 113 - "aml.c"
Cohesion: 0.50
Nodes (3): aml_context_t, aml_execute(), aml_parse()

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

### Community 127 - "window.c"
Cohesion: 0.60
Nodes (4): window_t, window_create(), window_destroy(), window_draw()

### Community 129 - "font_t"
Cohesion: 0.67
Nodes (3): font_t, font_load(), font_render()

### Community 137 - "Kimi K3 Agent Skill Bundle (Moonshot AI)"
Cohesion: 0.50
Nodes (4): Distinctive Frontend Design Guidance, Interactive React Prototype Guidance, Kimi K3 Agent Skill Bundle (Moonshot AI), 3D Object Modeling Workflow (three.js + three_d_stage)

### Community 138 - "sb_terminate"
Cohesion: 0.07
Nodes (27): _start(), print(), print_uint(), _start(), main(), _start(), print(), _start() (+19 more)

## Ambiguous Edges - Review These
- `fb_init()` → `ShadowBox Desktop Design Spec v1.0`  [AMBIGUOUS]
  docs/superpowers/specs/2026-07-17-shadowbox-desktop-design.md · relation: references
- `Unified Input Pipeline` → `Cursor State Tracking`  [AMBIGUOUS]
  gui_improvement_plan.md · relation: conceptually_related_to

## Knowledge Gaps
- **228 isolated node(s):** `gen_isr.sh script`, `bg_normal`, `bg_hover`, `bg_press`, `fg_normal` (+223 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **What is the exact relationship between `fb_init()` and `ShadowBox Desktop Design Spec v1.0`?**
  _Edge tagged AMBIGUOUS (relation: references) - confidence is low._
- **What is the exact relationship between `Unified Input Pipeline` and `Cursor State Tracking`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **Why does `Widget` connect `Widget` to `HexEditWindow`, `InputRouter.cpp`, `Widget.hpp`, `Window`, `Label`, `freestanding.c`, `TextBox`, `ScrollView`, `Rect`, `Button`, `Compositor`, `gui_wrappers.cpp`?**
  _High betweenness centrality (0.126) - this node is a cross-community bridge._
- **Why does `Window` connect `Window` to `Widget`, `HexEditWindow`, `Widget.hpp`, `Label`, `sb_acquire`, `ScrollView`, `TetrisWindow`, `Rect`, `Button`, `SnakeWindow`, `TextBox`?**
  _High betweenness centrality (0.077) - this node is a cross-community bridge._
- **Why does `printk()` connect `printk` to `syscall.c`, `kernel.h`, `inb`, `smp.c`, `pmm_alloc_page`, `cpu.c`, `storage.c`, `apic.c`, `power.c`, `security.c`, `device_init`, `aslr.c`, `kfree`, `device.h`, `block.c`, `tcp.c`, `task.c`, `kernel_main`, `fb.c`, `audio/hda.c`, `driver.c`, `ahci_init`, `camera.c`, `bluetooth.c`, `pty.c`, `entropy.c`, `hal_init`, `hid.c`, `ext4.c`, `tty.c`, `hid_kbd_init`, `shadowfs.c`, `kmalloc`?**
  _High betweenness centrality (0.057) - this node is a cross-community bridge._
- **Are the 142 inferred relationships involving `printk()` (e.g. with `acpi_init()` and `ahci_init()`) actually correct?**
  _`printk()` has 142 INFERRED edges - model-reasoned connections that need verification._
- **Are the 110 inferred relationships involving `sb_push()` (e.g. with `print()` and `print()`) actually correct?**
  _`sb_push()` has 110 INFERRED edges - model-reasoned connections that need verification._