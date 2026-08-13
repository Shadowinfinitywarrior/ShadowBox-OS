static void draw_sysmon(window_t *w) {
    // Update system monitor data periodically (≈0.5s)
    uint64_t now = sys_times(0);
    if (now - w->last_update > 50) {
        w->last_update = now;
        sys_mem_info((uint64_t*)w->text);
        uint64_t tot = ((uint64_t*)w->text)[0];
        uint64_t usd = ((uint64_t*)w->text)[1];
        uint8_t pct = (tot > 0) ? (usd * 100 / tot) : 0;
        
        // Randomly fluctuate CPU based on num_windows
        uint8_t cpu_pct = 5 + num_windows * 3 + (sys_times(0) % 15);
        if (cpu_pct > 100) cpu_pct = 100;
        
        uint32_t idx = *(uint32_t*)(w->text + 16);
        uint8_t *history = (uint8_t*)(w->text + 20);
        uint8_t *cpu_history = (uint8_t*)(w->text + 220);
        history[idx] = pct;
        cpu_history[idx] = cpu_pct;
        idx = (idx + 1) % 200;
        *(uint32_t*)(w->text + 16) = idx;
    }
    
    draw_string(w->x + 10, w->y + 34, "ShadowBox Advanced System Monitor", 0x2C3E50);
    
    // General Stats column
    draw_icon_cpu(w->x + 10, w->y + 55, 0x8E44AD);
    draw_string(w->x + 30, w->y + 60, "CPU: 1 Core AMD/Intel @ 2.5 GHz", 0x34495E);

    uint64_t total_mb = ((uint64_t*)w->text)[0] / (1024 * 1024);
    uint64_t used_mb  = ((uint64_t*)w->text)[1] / (1024 * 1024);
    draw_icon_ram(w->x + 10, w->y + 80, 0x27AE60);
    draw_string(w->x + 30, w->y + 85, "RAM: ", 0x34495E);
    draw_number(w->x + 70, w->y + 85, used_mb, 0x2C3E50);
    draw_string(w->x + 100, w->y + 85, "/", 0x34495E);
    draw_number(w->x + 110, w->y + 85, total_mb, 0x2C3E50);
    draw_string(w->x + 140, w->y + 85, "MB", 0x34495E);

    draw_icon_disk(w->x + 10, w->y + 105, 0x7F8C8D);
    draw_string(w->x + 30, w->y + 110, "Drive: AHCI SATA 0 (Active)", 0x34495E);

    draw_icon_folder(w->x + 10, w->y + 130, 0x2980B9);
    draw_string(w->x + 30, w->y + 135, "Uptime: ", 0x34495E);
    uint32_t uptime_s = sys_times(0) / 100;
    draw_number(w->x + 90, w->y + 135, uptime_s, 0x2C3E50);
    draw_string(w->x + 120, w->y + 135, "s", 0x34495E);
    
    draw_icon_file(w->x + 180, w->y + 130, 0xE67E22);
    draw_string(w->x + 200, w->y + 135, "Tasks: ", 0x34495E);
    draw_number(w->x + 256, w->y + 135, num_windows + 3, 0x2C3E50); // Approximated
    
    // Driver list (right side column)
    draw_string(w->x + 240, w->y + 55, "Drivers Loaded:", 0x2980B9);
    draw_string(w->x + 240, w->y + 70, "- e1000 & rtl8139 (Net)", 0x34495E);
    draw_string(w->x + 240, w->y + 85, "- Universal HID", 0x34495E);
    draw_string(w->x + 240, w->y + 100, "- ACPI/APIC/IOAPIC", 0x34495E);
    draw_string(w->x + 240, w->y + 115, "- VESA FB & Intel HDA", 0x34495E);
    
    // Memory Usage Graph Box
    draw_string(w->x + 10, w->y + 165, "Memory Usage History:", 0x2980B9);
    draw_rect(w->x + 10, w->y + 185, 200, 50, 0xBDC3C7);
    draw_rect(w->x + 12, w->y + 187, 196, 46, 0xECF0F1); // inner area
    
    // CPU Usage Graph Box
    draw_string(w->x + 220, w->y + 165, "CPU Load History:", 0x8E44AD);
    draw_rect(w->x + 220, w->y + 185, 200, 50, 0xBDC3C7);
    draw_rect(w->x + 222, w->y + 187, 196, 46, 0xECF0F1); // inner area
    
    uint32_t idx = *(uint32_t*)(w->text + 16);
    uint8_t *history = (uint8_t*)(w->text + 20);
    uint8_t *cpu_history = (uint8_t*)(w->text + 220);
    for (int i = 0; i < 196; i++) {
        int hist_pos = (idx + i + 4) % 200;
        uint8_t pct = history[hist_pos];
        if (pct > 100) pct = 100;
        int h = (pct * 46) / 100;
        if (h > 0) {
            uint32_t color = (pct > 80) ? 0xE74C3C : (pct > 50 ? 0xF39C12 : 0x27AE60);
            for (int y = 0; y < h; y++) {
                backbuffer[(w->y + 187 + 46 - 1 - y) * SCREEN_WIDTH + (w->x + 12 + i)] = color;
            }
        }
        
        uint8_t cpu_pct = cpu_history[hist_pos];
        if (cpu_pct > 100) cpu_pct = 100;
        int cpu_h = (cpu_pct * 46) / 100;
        if (cpu_h > 0) {
            uint32_t color = (cpu_pct > 80) ? 0xE74C3C : (cpu_pct > 50 ? 0xF39C12 : 0x8E44AD);
            for (int y = 0; y < cpu_h; y++) {
                backbuffer[(w->y + 187 + 46 - 1 - y) * SCREEN_WIDTH + (w->x + 222 + i)] = color;
            }
        }
    }
}
