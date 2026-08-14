# Graph Report - .  (2026-08-14)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 2590 nodes · 5772 edges · 146 communities (139 shown, 7 thin omitted)
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 1629 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9b551bd8`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- desktop.c
- Widget
- syscall.c
- Window
- security.c
- gui_wrappers.cpp
- types.h
- Rect
- userland/sys.h
- Compositor
- sb_push
- time.c
- spin_lock_irqsave
- InputRouter
- strlen
- sb_pull
- tcp.c
- inb
- theme.c
- shell.c
- TextBox
- smp.c
- task.c
- printk
- mark_dirty
- storage.c
- Widget.hpp
- kernel.h
- pmm_alloc_page
- driver.c
- Button
- spinlock.h
- tmpfs.c
- errno.h
- include/sys.h
- power.c
- kobject
- pci_config_read
- cpu.c
- wm.h
- kfree
- ScrollView
- widget.c
- fb.c
- device_init
- SnakeWindow
- file_manager.c
- apic.c
- kmalloc
- hal/memory.c
- ContextMenu
- gui_toolkit.h
- audio/hda.c
- hid.c
- desktop_corrupted.c
- workspace.c
- task_create_proc
- camera.c
- HexEditWindow
- pty.c
- MatrixWindow
- socket.c
- media_player_pcm.c
- devfs.c
- network_manager_cli.c
- procfs.c
- evdev.c
- hal_init
- ext4.c
- taskbar.c
- peripheral.c
- tty_read
- tarfs.c
- SysmonWindow
- module.c
- udp.c
- shadowfs.c
- screensaver.c
- service.c
- settingsd.c
- textinput.c
- _start
- main
- search.c
- app_launcher.c
- cpu_init
- ui/font.c
- fat32.c
- numa.c
- aml.c
- mmap.c
- dirwatch_register
- hal_status_t
- pci_msi.c
- entropy.c
- notification.c
- printk.c
- _start
- window.c
- xhci.c
- ipv6.c
- clipboard_test.c
- nx_test.c
- test_tar.c
- false.c
- gen_isr.sh
- init_script.sh

## God Nodes (most connected - your core abstractions)
1. `printk()` - 143 edges
2. `sb_push()` - 113 edges
3. `Widget` - 98 edges
4. `kmalloc()` - 79 edges
5. `Window` - 76 edges
6. `get_current_process()` - 59 edges
7. `_start()` - 56 edges
8. `sb_pull()` - 56 edges
9. `sb_terminate()` - 51 edges
10. `sb_release()` - 49 edges

## Surprising Connections (you probably didn't know these)
- `fb_init()` --calls--> `printk()`  [INFERRED]
  arch/x86_64/drivers/fb.c → kernel/printk.c
- `current_cpu()` --calls--> `current_cpu_id()`  [INFERRED]
  include/smp.h → arch/x86_64/kernel/smp.c
- `sched_steal_task()` --calls--> `percpu_rq_steal()`  [INFERRED]
  kernel/sched.c → arch/x86_64/kernel/smp.c
- `numa_alloc_onnode()` --calls--> `kmalloc()`  [INFERRED]
  kernel/mem/numa.c → arch/x86_64/mm/malloc.c
- `free()` --calls--> `kfree()`  [INFERRED]
  kernel/hal/alloc_wrapper.c → arch/x86_64/mm/malloc.c

## Import Cycles
- None detected.

## Communities (146 total, 7 thin omitted)

### Community 0 - "desktop.c"
Cohesion: 0.08
Nodes (83): window_t, draw_sysmon(), atoi64(), blend_color(), window_t, calc_click(), calc_input(), close_window() (+75 more)

### Community 1 - "Widget"
Cohesion: 0.05
Nodes (51): EventFn, EventType, Button::Button(), on_mouse_move, on_mouse_press, on_mouse_release, Label::Label(), InputRouter (+43 more)

### Community 2 - "syscall.c"
Cohesion: 0.06
Nodes (66): check_permissions(), is_user_range(), rdmsr(), rtc_to_unix(), sb_acquire(), sb_morph(), sb_pull(), sb_push() (+58 more)

### Community 3 - "Window"
Cohesion: 0.04
Nodes (62): hit_test_all, InputRouter, MouseCursor, router_, Point, x, y, hit_test (+54 more)

### Community 4 - "security.c"
Cohesion: 0.06
Nodes (48): audit_entry_t, battery_status_t, spin_lock(), spin_unlock(), kernel_cap_t, irq_monitor_print_stats(), irq_monitor_record(), power_battery_get() (+40 more)

### Community 5 - "gui_wrappers.cpp"
Cohesion: 0.06
Nodes (61): gui_button_t, gui_comp_t, gui_context_menu_t, button_click_trampoline(), gui_button_create(), gui_button_destroy(), gui_button_set_label(), gui_button_set_on_clicked() (+53 more)

### Community 6 - "types.h"
Cohesion: 0.04
Nodes (3): ntohl(), ntohs(), current_cpu()

### Community 7 - "Rect"
Cohesion: 0.07
Nodes (44): blend(), fb_draw_rect(), fb_draw_rect_round(), fb_draw_text(), fb_draw_text_wrap(), fb_fill_rect(), fb_fill_rect_round(), fb_text_width() (+36 more)

### Community 8 - "userland/sys.h"
Cohesion: 0.06
Nodes (47): atoi_octal(), main(), print(), atoi(), main(), print(), _start(), _start() (+39 more)

### Community 9 - "Compositor"
Cohesion: 0.06
Nodes (40): gui_clear_screen(), gui_draw_filled_rect(), gui_draw_pixel(), gui_draw_rect(), fb_blit_rect(), gui_blit_screen(), gui_fb_flip(), Compositor (+32 more)

### Community 10 - "sb_push"
Cohesion: 0.07
Nodes (43): print(), print_uint(), _start(), print(), print_uint(), _start(), parse_int(), pow_int() (+35 more)

### Community 11 - "time.c"
Cohesion: 0.05
Nodes (38): calendar_get_date(), calendar_get_time(), is_leap_year(), hid_device_t, key_event_t, hid_kbd_process_report(), hid_kbd_repeat_tick(), kbd_dispatch_event() (+30 more)

### Community 12 - "spin_lock_irqsave"
Cohesion: 0.09
Nodes (48): heapify_down(), heapify_up(), percpu_rq_dequeue(), percpu_rq_enqueue(), percpu_rq_remove(), percpu_rq_steal(), swap_ptrs(), pmm_get_info() (+40 more)

### Community 13 - "InputRouter"
Cohesion: 0.06
Nodes (43): Compositor, InputRouter, input_router_create(), input_router_destroy(), input_router_key_press(), input_router_key_release(), input_router_mouse_absolute(), input_router_mouse_packet() (+35 more)

### Community 14 - "strlen"
Cohesion: 0.04
Nodes (30): parse_size(), print(), print(), _start(), print(), print(), print(), _start() (+22 more)

### Community 15 - "sb_pull"
Cohesion: 0.12
Nodes (45): main(), create_window(), _start(), hexview_open(), maybe_load_wallpaper(), network_manager_create(), _start(), _start() (+37 more)

### Community 16 - "tcp.c"
Cohesion: 0.09
Nodes (42): net_device_t, pci_device_t, e1000_init(), e1000_irq_handler(), e1000_read(), e1000_send_packet(), e1000_write(), net_device_t (+34 more)

### Community 17 - "inb"
Cohesion: 0.09
Nodes (39): kbd_translate(), keyboard_handler(), keyboard_init(), ps2_has_keyboard_byte(), ps2_wait_write(), mouse_handler(), mouse_init(), mouse_process_byte() (+31 more)

### Community 18 - "theme.c"
Cohesion: 0.06
Nodes (33): bezier_curve_t, _align_up(), free(), malloc(), memcpy(), realloc(), _sbrk(), strcmp() (+25 more)

### Community 19 - "shell.c"
Cohesion: 0.10
Nodes (46): add_history(), atoi(), cmd_change_dir(), cmd_create_dir(), cmd_date(), cmd_df(), cmd_dmesg(), cmd_echo() (+38 more)

### Community 20 - "TextBox"
Cohesion: 0.06
Nodes (36): ChangeFn, Color, TextBox, bg_focused, bg_normal, border_focused, border_normal, buf_ (+28 more)

### Community 21 - "smp.c"
Cohesion: 0.08
Nodes (35): apic_count_cpus(), lapic_read(), gdt_init(), gdt_init_ap(), gdt_set_gate(), gdt_set_tss(), tss_set_stack(), idt_init_ap() (+27 more)

### Community 22 - "task.c"
Cohesion: 0.08
Nodes (32): pit_handler(), heapify_down(), heapify_up(), nice_to_weight(), sched_balance_runqueues(), sched_enqueue(), sched_enqueue_on(), sched_pick_next() (+24 more)

### Community 23 - "printk"
Cohesion: 0.08
Nodes (33): bt_device_t, device_t, i2c_hid_init(), i2c_hid_probe(), i2c_hid_remove(), device_t, ehci_driver_init(), ehci_probe() (+25 more)

### Community 24 - "mark_dirty"
Cohesion: 0.09
Nodes (32): Color, Label, align_, fg_, FONT_H_, FONT_W_, MAX_TEXT, scale_ (+24 more)

### Community 25 - "storage.c"
Cohesion: 0.09
Nodes (31): UNUSED, ramdisk_init(), ramdisk_read(), ramdisk_write(), block_get_device(), block_init(), block_read(), block_register_device() (+23 more)

### Community 26 - "Widget.hpp"
Cohesion: 0.09
Nodes (17): sys_sbrk(), ClockWindow, ClockWindow::ClockWindow(), tick, ticks_, time_label_, fmt_time(), main() (+9 more)

### Community 27 - "kernel.h"
Cohesion: 0.06
Nodes (7): acpi_init(), UNUSED, swap_init(), swap_page_in(), swap_page_out(), rng_init(), test_priority_scheduler()

### Community 28 - "pmm_alloc_page"
Cohesion: 0.15
Nodes (31): ahci_read(), ahci_write(), UNUSED, isr_handler(), page_fault_handler(), expand_heap(), malloc_init(), bitmap_clear() (+23 more)

### Community 29 - "driver.c"
Cohesion: 0.10
Nodes (27): bus_type_t, device_t, driver_t, i2c_master_init(), i2c_master_match(), i2c_master_probe(), i2c_master_remove(), device_t (+19 more)

### Community 30 - "Button"
Cohesion: 0.10
Nodes (28): Button, bg_hover, bg_normal, bg_press, border_col, corner_radius, fg_hover, fg_normal (+20 more)

### Community 31 - "spinlock.h"
Cohesion: 0.08
Nodes (17): buddy_add_to_list(), buddy_alloc(), buddy_free(), buddy_init(), buddy_remove_from_list(), spinlock_init(), ipc_init(), msgget() (+9 more)

### Community 32 - "tmpfs.c"
Cohesion: 0.19
Nodes (25): vfs_node_t, tmpfs_access(), tmpfs_create_entry(), tmpfs_create_file_entry(), tmpfs_create_func(), tmpfs_find_child(), tmpfs_finddir(), tmpfs_finddir_func() (+17 more)

### Community 33 - "errno.h"
Cohesion: 0.10
Nodes (16): init_main(), _start(), start_service(), copy_from_user(), copy_to_user(), validate_user_ptr(), find_process(), sys_sb_ipc_call() (+8 more)

### Community 34 - "include/sys.h"
Cohesion: 0.15
Nodes (26): sb_msg_t, sb_acquire(), sb_ipc_call(), sb_ipc_reply_wait(), sb_morph(), sb_pull(), sb_push(), sb_release() (+18 more)

### Community 35 - "power.c"
Cohesion: 0.09
Nodes (16): acpi_fadt_t, c_state_t, inl(), outw(), acpi_fadt_checksum(), parse_acpi_tables(), power_get_c_state(), power_get_p_state() (+8 more)

### Community 36 - "kobject"
Cohesion: 0.08
Nodes (25): vfs_node_t, kobject, attr_count, attrs, bin_attrs, kset, name, parent (+17 more)

### Community 37 - "pci_config_read"
Cohesion: 0.17
Nodes (20): ahci_init(), check_type(), port_rebase(), start_cmd(), stop_cmd(), hda_init(), pci_device_t, pci_check_device() (+12 more)

### Community 38 - "cpu.c"
Cohesion: 0.09
Nodes (5): cpu_registers_t, cpu_halt(), cpu_restore_registers(), cpu_save_registers(), idle_loop()

### Community 39 - "wm.h"
Cohesion: 0.09
Nodes (10): widget_t, window_t, wm_layout_mode_t, workspace_t, layout_widget_recursive(), ui_layout_pass(), ui_set_layout_mode(), xdg_toplevel_t (+2 more)

### Community 40 - "kfree"
Cohesion: 0.37
Nodes (21): kfree(), ext2_inode_t, vfs_node_t, ext2_add_dir_entry(), ext2_alloc_block_from_group(), ext2_allocate_block(), ext2_allocate_inode(), ext2_create_file() (+13 more)

### Community 41 - "ScrollView"
Cohesion: 0.13
Nodes (15): KernelConfigView, content_, scroll_view_, Color, ScrollView, clamp_scroll, content_, MIN_THUMB (+7 more)

### Community 42 - "widget.c"
Cohesion: 0.17
Nodes (19): libinput_event_t, widget_t, window_t, gui_create_window(), gui_dispatch_event(), gui_hit_test(), gui_hit_test_at(), hit_test_recursive() (+11 more)

### Community 43 - "fb.c"
Cohesion: 0.21
Nodes (13): fb_double_free(), fb_double_init(), fb_console_init(), fb_console_putchar(), fb_get_addr(), fb_get_bpp(), fb_get_height(), fb_get_phys() (+5 more)

### Community 44 - "device_init"
Cohesion: 0.30
Nodes (16): boot_stage_begin(), boot_stage_end(), boot_stages_summary(), read_tick(), arch_init(), bootloader_info_parse(), device_init(), initrd_mount() (+8 more)

### Community 45 - "SnakeWindow"
Cohesion: 0.16
Nodes (16): rng_next(), SnakeWindow, food_x_, food_y_, GRID_SIZE, MAX_SNAKE, on_key_press, reset (+8 more)

### Community 46 - "file_manager.c"
Cohesion: 0.22
Nodes (16): compare_entries(), copy_file(), create_directory(), create_file(), delete_entry(), filter_entries(), list_directory(), print() (+8 more)

### Community 47 - "apic.c"
Cohesion: 0.19
Nodes (13): apic_init(), ioapic_route_irq(), ioapic_set_entry(), ioapic_write(), lapic_enable(), lapic_eoi(), lapic_write(), hid_kbd_repeat_tick() (+5 more)

### Community 48 - "kmalloc"
Cohesion: 0.16
Nodes (15): kmalloc(), split_block(), vfs_node_t, sysfs_class_read(), sysfs_devices_read(), sysfs_finddir(), sysfs_init(), sysfs_power_read() (+7 more)

### Community 49 - "hal/memory.c"
Cohesion: 0.12
Nodes (8): memory_alloc_physical(), memory_free_physical(), memory_get_map(), memory_map_physical(), memory_set(), memory_unmap_physical(), memory_zero(), memory_map_entry_t

### Community 50 - "ContextMenu"
Cohesion: 0.17
Nodes (12): ContextMenu, add_item, hovered_index_, item_count_, ITEM_HEIGHT, items_, MAX_ITEMS, on_mouse_move (+4 more)

### Community 51 - "gui_toolkit.h"
Cohesion: 0.15
Nodes (4): widget_t, color_picker_create(), color_picker_get_color(), color_picker_set_color()

### Community 52 - "audio/hda.c"
Cohesion: 0.20
Nodes (12): audio_format_t, audio_hda_init(), audio_device_t, hda_close_stream(), hda_open_stream(), hda_read_pcm(), hda_write_pcm(), audio_register_device() (+4 more)

### Community 53 - "hid.c"
Cohesion: 0.22
Nodes (13): hid_device_t, hid_core_init(), hid_pointer_init(), hid_pointer_process_report(), hid_process_report(), hid_register_device(), hid_touchpad_init(), hid_touchpad_process_report() (+5 more)

### Community 54 - "desktop_corrupted.c"
Cohesion: 0.33
Nodes (13): blend_color(), window_t, draw_char(), draw_cursor(), draw_desktop(), draw_drop_shadow(), draw_number(), draw_rect() (+5 more)

### Community 55 - "workspace.c"
Cohesion: 0.21
Nodes (13): wm_layout_mode_t, workspace_t, xdg_toplevel_t, find_workspace_by_id(), wm_add_window_to_active(), wm_animate_window_close(), wm_animate_window_open(), wm_get_active_workspace() (+5 more)

### Community 56 - "task_create_proc"
Cohesion: 0.18
Nodes (11): mouse_start_poll_thread(), slab_alloc(), slab_create_cache(), slab_free(), slab_init(), idle_tasks_init(), kthread_create(), kthread_init() (+3 more)

### Community 57 - "camera.c"
Cohesion: 0.25
Nodes (13): cam_device_t, cam_format_t, cam_frame_t, cam_close(), cam_dequeue_frame(), cam_enqueue_frame(), cam_open(), cam_set_format() (+5 more)

### Community 58 - "HexEditWindow"
Cohesion: 0.20
Nodes (12): HexEditWindow, BYTES_PER_ROW, cursor_, data_, file_size_, load_file, MAX_FILE_SIZE, on_key_press (+4 more)

### Community 60 - "pty.c"
Cohesion: 0.21
Nodes (8): vfs_node_t, pty_create(), pty_read(), pty_subsystem_init(), pty_vfs_read(), pty_vfs_write(), pty_write(), sys_pty_create()

### Community 62 - "MatrixWindow"
Cohesion: 0.21
Nodes (9): m_rand(), MatrixWindow, chars_, COLS, drops_, MatrixWindow::MatrixWindow(), ROWS, tick (+1 more)

### Community 63 - "socket.c"
Cohesion: 0.18
Nodes (11): sb_socket_accept_wrapper(), sb_socket_bind_wrapper(), sb_socket_connect_wrapper(), sb_socket_listen_wrapper(), vfs_node_t, sb_socket_accept(), sb_socket_bind(), sb_socket_connect() (+3 more)

### Community 64 - "media_player_pcm.c"
Cohesion: 0.39
Nodes (11): add_to_playlist(), next_track(), play_track(), prev_track(), print_uint(), readline(), show_playlist(), show_track_info() (+3 more)

### Community 65 - "devfs.c"
Cohesion: 0.29
Nodes (10): vfs_node_t, dev_input_read(), dev_null_read(), dev_null_write(), dev_zero_read(), dev_zero_write(), devfs_finddir(), devfs_init() (+2 more)

### Community 66 - "network_manager_cli.c"
Cohesion: 0.35
Nodes (10): cmd_connect(), cmd_diagnose(), cmd_disconnect(), cmd_help(), cmd_list(), cmd_ping(), cmd_status(), main() (+2 more)

### Community 67 - "procfs.c"
Cohesion: 0.33
Nodes (9): vfs_node_t, proc_cpuinfo_read(), proc_filesystems_read(), proc_meminfo_read(), proc_stat_read(), proc_version_read(), procfs_finddir(), procfs_init() (+1 more)

### Community 68 - "evdev.c"
Cohesion: 0.29
Nodes (8): input_dev_t, input_event_t, evdev_poll_event(), evdev_push(), input_allocate_device(), input_register_device(), input_report_event(), input_sync()

### Community 69 - "hal_init"
Cohesion: 0.22
Nodes (8): pmm_total_pages(), hal_arch_t, hal_status_t, hal_get_arch(), hal_init(), hal_shutdown(), memory_init(), peripheral_init()

### Community 70 - "ext4.c"
Cohesion: 0.36
Nodes (8): vfs_node_t, ext4_create_file(), ext4_init(), ext4_mount(), ext4_read(), ext4_readdir(), ext4_unlink(), ext4_write()

### Community 71 - "taskbar.c"
Cohesion: 0.36
Nodes (6): blend_color(), draw_char(), draw_rect(), draw_rect_alpha(), draw_string(), draw_taskbar()

### Community 72 - "peripheral.c"
Cohesion: 0.33
Nodes (8): hal_status_t, i2c_init(), peripheral_enumerate(), peripheral_read_config(), peripheral_write_config(), spi_init(), peripheral_device_t, peripheral_list_t

### Community 73 - "tty_read"
Cohesion: 0.32
Nodes (7): keyboard_getchar(), keyboard_has_char(), vfs_node_t, tty_echo_char(), tty_init(), tty_read(), tty_write()

### Community 74 - "tarfs.c"
Cohesion: 0.48
Nodes (6): vfs_node_t, parse_size(), tarfs_finddir(), tarfs_init(), tarfs_read(), tarfs_readdir()

### Community 76 - "SysmonWindow"
Cohesion: 0.32
Nodes (7): num_to_str(), strcat_custom(), SysmonWindow, cpu_label_, mem_label_, tick, tick_timer_

### Community 78 - "module.c"
Cohesion: 0.29
Nodes (3): register_dummy_module(), register_kernel_module(), kernel_module_t

### Community 79 - "udp.c"
Cohesion: 0.39
Nodes (7): net_device_t, udp_handle_packet(), udp_socket_bind(), udp_socket_create(), udp_socket_recvfrom(), udp_socket_sendto(), udp_socket_t

### Community 80 - "shadowfs.c"
Cohesion: 0.43
Nodes (6): vfs_node_t, shadowfs_finddir(), shadowfs_init(), shadowfs_read(), shadowfs_readdir(), shadowfs_write()

### Community 81 - "screensaver.c"
Cohesion: 0.38
Nodes (3): screensaver_engine_tick(), ss_clear(), ss_draw_ball()

### Community 82 - "service.c"
Cohesion: 0.33
Nodes (3): service_manager_start_unit(), service_manager_stop_unit(), service_unit_t

### Community 83 - "settingsd.c"
Cohesion: 0.33
Nodes (6): power_config_t, theme_config_t, settingsd_apply_power_profile(), settingsd_apply_theme(), settingsd_init(), simple_strcpy()

### Community 84 - "textinput.c"
Cohesion: 0.48
Nodes (6): textinput_t, textinput_draw(), textinput_get_text(), textinput_handle_key(), textinput_init(), textinput_set_placeholder()

### Community 85 - "_start"
Cohesion: 0.43
Nodes (6): newline(), print(), print_hex64(), print_uint64(), _start(), sys_lseek()

### Community 86 - "main"
Cohesion: 0.43
Nodes (6): htonl(), htons(), main(), parse_ip(), print(), print_uint()

### Community 87 - "search.c"
Cohesion: 0.48
Nodes (6): contains_substring(), main(), my_strlen(), print(), search_dir(), strcmp()

### Community 88 - "app_launcher.c"
Cohesion: 0.47
Nodes (4): app_index_entry_t, app_launcher_get_recent(), app_launcher_search(), contains_substring()

### Community 90 - "cpu_init"
Cohesion: 0.33
Nodes (6): cpu_info_t, hal_status_t, cpu_get_info(), cpu_get_vendor_string(), cpu_init(), cpuid()

### Community 91 - "ui/font.c"
Cohesion: 0.53
Nodes (5): font_t, blend(), font_load(), font_render(), pixel_at()

### Community 92 - "fat32.c"
Cohesion: 0.40
Nodes (3): vfs_node_t, fat32_read(), fat32_write()

### Community 94 - "aml.c"
Cohesion: 0.50
Nodes (3): aml_context_t, aml_execute(), aml_parse()

### Community 95 - "mmap.c"
Cohesion: 0.50
Nodes (4): UNUSED, mmap_init(), sys_mmap(), sys_munmap()

### Community 96 - "dirwatch_register"
Cohesion: 0.50
Nodes (3): dirwatch_cb_t, dirwatch_register(), dirwatch_unregister()

### Community 97 - "hal_status_t"
Cohesion: 0.40
Nodes (5): dram_info_t, hal_status_t, memory_get_dram_info(), memory_get_stats(), memory_stats_t

### Community 98 - "pci_msi.c"
Cohesion: 0.60
Nodes (4): pci_msi_alloc_vectors(), pci_msi_free_vectors(), pci_msi_setup(), pci_msi_teardown()

### Community 103 - "entropy.c"
Cohesion: 0.70
Nodes (4): entropy_get(), entropy_get_u32(), entropy_get_u64(), entropy_init()

### Community 105 - "notification.c"
Cohesion: 0.40
Nodes (3): notification_dismiss(), notification_send(), notification_t

### Community 106 - "printk.c"
Cohesion: 0.60
Nodes (3): put_char(), vga_putchar(), vga_scroll()

### Community 108 - "_start"
Cohesion: 0.60
Nodes (4): print(), print_uint(), read_line(), _start()

### Community 109 - "window.c"
Cohesion: 0.60
Nodes (4): window_t, window_create(), window_destroy(), window_draw()

### Community 117 - "clipboard_test.c"
Cohesion: 0.67
Nodes (3): print(), print_uint(), _start()

### Community 118 - "nx_test.c"
Cohesion: 0.67
Nodes (3): main(), print(), sys_mmap_inline()

## Knowledge Gaps
- **194 isolated node(s):** `gen_isr.sh script`, `bg_normal`, `bg_hover`, `bg_press`, `fg_normal` (+189 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Widget` connect `Widget` to `Window`, `gui_wrappers.cpp`, `Rect`, `Compositor`, `ScrollView`, `InputRouter`, `ContextMenu`, `theme.c`, `TextBox`, `mark_dirty`, `MatrixWindow`, `Widget.hpp`, `Button`?**
  _High betweenness centrality (0.131) - this node is a cross-community bridge._
- **Why does `Window` connect `Window` to `Widget`, `HexEditWindow`, `Rect`, `ScrollView`, `SysmonWindow`, `SnakeWindow`, `sb_pull`, `ContextMenu`, `Button`, `mark_dirty`, `Widget.hpp`, `MatrixWindow`?**
  _High betweenness centrality (0.065) - this node is a cross-community bridge._
- **Why does `printk()` connect `printk` to `syscall.c`, `security.c`, `tcp.c`, `inb`, `smp.c`, `task.c`, `storage.c`, `kernel.h`, `pmm_alloc_page`, `driver.c`, `spinlock.h`, `tmpfs.c`, `power.c`, `pci_config_read`, `kfree`, `fb.c`, `device_init`, `apic.c`, `kmalloc`, `audio/hda.c`, `hid.c`, `task_create_proc`, `camera.c`, `pty.c`, `devfs.c`, `procfs.c`, `hal_init`, `ext4.c`, `tty_read`, `tarfs.c`, `module.c`, `shadowfs.c`, `cpu_init`, `mmap.c`, `entropy.c`, `printk.c`?**
  _High betweenness centrality (0.038) - this node is a cross-community bridge._
- **Are the 140 inferred relationships involving `printk()` (e.g. with `acpi_init()` and `ahci_init()`) actually correct?**
  _`printk()` has 140 INFERRED edges - model-reasoned connections that need verification._
- **Are the 111 inferred relationships involving `sb_push()` (e.g. with `print()` and `print()`) actually correct?**
  _`sb_push()` has 111 INFERRED edges - model-reasoned connections that need verification._
- **Are the 76 inferred relationships involving `kmalloc()` (e.g. with `fb_double_init()` and `ramdisk_init()`) actually correct?**
  _`kmalloc()` has 76 INFERRED edges - model-reasoned connections that need verification._
- **Are the 7 inferred relationships involving `Window` (e.g. with `about_create()` and `filebrowser_create()`) actually correct?**
  _`Window` has 7 INFERRED edges - model-reasoned connections that need verification._