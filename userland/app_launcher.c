// App Launcher component implementation
// Provides a very small in‑memory index of applications for the desktop.
// This implementation is intentionally simple – it uses a static table of
// entries and does not depend on any kernel services such as kmalloc.

#include "desktop.h"


// Helper: simple substring search (case sensitive).
static int contains_substring(const char *haystack, const char *needle) {
    if (!*needle) return 1; // empty needle matches anything
    for (; *haystack; ++haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            ++h; ++n;
        }
        if (!*n) return 1; // reached end of needle -> match
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Static application index.
// Each entry mirrors the items that appear in the start menu (see desktop.c).
// The fields are filled with reasonable defaults – the DE only needs the
// display_name for searching and the exec_path for launching.
// ---------------------------------------------------------------------------

static app_index_entry_t app_index[] = {
    { "terminal",         "Terminal",         "terminal.elf",         "", 0 },
    { "file_explorer",    "File Explorer",    "file_manager.elf",    "", 0 },
    { "system_monitor",   "System Monitor",   "sys_monitor.elf",      "", 0 },
    { "image_viewer",     "Image Viewer",     "image_viewer.elf",     "", 0 },
    { "font_viewer",      "Font Viewer",      "font_viewer.elf",      "", 0 },
    { "memory_viewer",    "Memory Viewer",    "memory_viewer.elf",    "", 0 },
    { "calculator",       "Calculator",       "calculator.elf",       "", 0 },
    { "text_editor",      "Text Editor",      "text_editor.elf",      "", 0 },
    { "paint",            "Paint",            "paint.elf",            "", 0 },
    { "process_monitor",  "Process Monitor",  "process_monitor.elf",  "", 0 },
    { "hex_viewer",       "Hex Viewer",       "hex_viewer.elf",       "", 0 },
    { "snake",            "Snake",            "snake.elf",            "", 0 },
    { "tetris",           "Tetris",           "tetris.elf",           "", 0 },
    { "2048",             "2048",             "2048.elf",             "", 0 },
    { "pong",             "Pong",             "pong.elf",             "", 0 },
    { "matrix_rain",      "Matrix Rain",      "matrix_rain.elf",      "", 0 },
    { "mandelbrot",       "Mandelbrot",       "mandelbrot.elf",       "", 0 },
    { "clock",            "Clock",            "clock.elf",            "", 0 },
    { "reminders",        "Reminders",        "reminders.elf",        "", 0 },
    { "fortune",          "Fortune",          "fortune.elf",          "", 0 },
    { "about",            "About",            "about.elf",            "", 0 },
    { "settings",         "Settings",         "settings.elf",         "", 0 },
    { "file_manager",     "File Manager",     "file_manager.elf",     "", 0 },
    { "package_manager",  "Package Manager",  "package_manager.elf",  "", 0 },
    { "keyboard_power",   "Keyboard Power",   "keyboard_power.elf",   "", 0 },
    { "notepad",          "Notepad",          "notepad.elf",          "", 0 },
    // "Shutdown" is a special menu command, not an app.
};

static const uint32_t app_index_count = (uint32_t)(sizeof(app_index) / sizeof(app_index[0]));

// ---------------------------------------------------------------------------
// Simple API implementation.
// ---------------------------------------------------------------------------

void app_launcher_init(void) {
    // No dynamic initialisation required for this static implementation.
    // The function exists for API compatibility.
}

// Search for applications whose display_name contains the query string.
// Returns an array of pointers to matching entries and stores the count in
// *result_count. The returned array points to static storage that is reused on
// each call – callers must not free it.
app_index_entry_t** app_launcher_search(const char *query, uint32_t *result_count) {
    static app_index_entry_t *matches[64]; // enough for all known apps
    uint32_t cnt = 0;
    if (!query || !result_count) return 0;
    for (uint32_t i = 0; i < app_index_count && cnt < 64; ++i) {
        if (contains_substring(app_index[i].display_name, query)) {
            matches[cnt++] = &app_index[i];
        }
    }
    *result_count = cnt;
    return cnt ? matches : 0;
}

// Return the most recent applications (by last_launched). For this simple
// implementation we just return the first 'max_results' entries.
app_index_entry_t** app_launcher_get_recent(uint32_t max_results) {
    static app_index_entry_t *recent[64];
    if (max_results == 0) return 0;
    if (max_results > app_index_count) max_results = app_index_count;
    for (uint32_t i = 0; i < max_results; ++i) {
        recent[i] = &app_index[i];
    }
    return recent;
}
