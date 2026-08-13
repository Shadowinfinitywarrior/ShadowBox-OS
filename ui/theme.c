// Theme Engine implementation for ShadowBox OS.
// Provides runtime theming based on settings daemon configuration.
// The API mirrors the original stub for compatibility.

#include <stdint.h>
#include "settingsd.h"

// Expose the global settings daemon variable defined in settingsd.c.
extern settings_daemon_t settings_daemon;

// Internal copy of the current theme configuration.
static theme_config_t current_theme;

// Forward declaration for internal helper.
static uint32_t hex_to_color(const char *hex);

// Initialise the theme subsystem using the current settings daemon theme.
void ui_theme_init(void) {
    // Copy the theme from the settings daemon (which should be initialised already).
    current_theme = settings_daemon.theme;
}

// Retrieve the current theme configuration.
const theme_config_t *ui_theme_get_current(void) {
    return &current_theme;
}

// Set a new theme configuration. Returns 0 on success, -1 on error.
int ui_theme_set_current(const theme_config_t *theme) {
    if (!theme) return -1;
    current_theme = *theme;
    // Also propagate to the global settings daemon for consistency.
    settings_daemon.theme = *theme;
    return 0;
}

int ui_theme_apply(const theme_config_t *theme) {
    return ui_theme_set_current(theme);
}

// Cleanup placeholder (no resources allocated).
void ui_theme_cleanup(void) {
    // No dynamic resources to release.
}

// Convert a "#RRGGBB" hex string (or "RRGGBB") to a 0xRRGGBB color value.
static uint32_t hex_to_color(const char *hex) {
    // Skip optional leading '#'.
    if (hex && hex[0] == '#') {
        hex++;
    }
    uint32_t color = 0;
    for (int i = 0; i < 6; ++i) {
        char c = hex[i];
        uint8_t digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            digit = 0; // Invalid character, treat as zero.
        }
        color = (color << 4) | digit;
    }
    return color;
}

// Helper to fetch the accent color defined in the current theme.
uint32_t ui_theme_get_accent_color(void) {
    return hex_to_color(current_theme.accent_color_hex);
}
uint32_t ui_theme_get_primary_color(void) { return hex_to_color(current_theme.primary_color_hex); }
uint32_t ui_theme_get_secondary_color(void) { return hex_to_color(current_theme.secondary_color_hex); }

// Helper to fetch background color based on dark mode flag.
uint32_t ui_theme_get_background_color(void) {
    // Light mode uses white background, dark mode uses dark gray.
    return current_theme.dark_mode ? 0x1E1E1E : 0xFFFFFF;
}

// Helper to fetch default foreground (text) color based on dark mode.
uint32_t ui_theme_get_foreground_color(void) {
    return current_theme.dark_mode ? 0xCCCCCC : 0x333333;
}

// Helper to query whether dark mode is active.
uint8_t ui_theme_is_dark(void) {
    return current_theme.dark_mode;
}