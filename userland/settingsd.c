// Minimal Settings Daemon implementation for ShadowBox Desktop UI.
// Provides default configuration values and simple apply functions.
// This file is compiled into the desktop userland binary.

#include "settingsd.h"

// Global settings state – visible to other modules.
settings_daemon_t settings_daemon;

// Simple string copy utility (no standard library).
static void simple_strcpy(char *dst, const char *src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void settingsd_init(void) {
    // Theme defaults
    settings_daemon.theme.dark_mode = 0; // Light theme by default
    simple_strcpy(settings_daemon.theme.accent_color_hex, "#0077FF", sizeof(settings_daemon.theme.accent_color_hex));
    simple_strcpy(settings_daemon.theme.primary_color_hex, "#1E90FF", sizeof(settings_daemon.theme.primary_color_hex));
    simple_strcpy(settings_daemon.theme.secondary_color_hex, "#FF69B4", sizeof(settings_daemon.theme.secondary_color_hex));
    simple_strcpy(settings_daemon.theme.background_color_hex, "#FFFFFF", sizeof(settings_daemon.theme.background_color_hex));
    simple_strcpy(settings_daemon.theme.foreground_color_hex, "#000000", sizeof(settings_daemon.theme.foreground_color_hex));
    simple_strcpy(settings_daemon.theme.icon_theme, "default", sizeof(settings_daemon.theme.icon_theme));
    simple_strcpy(settings_daemon.theme.cursor_theme, "default", sizeof(settings_daemon.theme.cursor_theme));

    // Power defaults
    settings_daemon.power.screen_dim_timeout_sec = 30; // seconds until dim
    settings_daemon.power.sleep_timeout_sec = 120;   // seconds until sleep
    settings_daemon.power.power_saving_mode = 0;   // disabled

    // Input / display overrides
    simple_strcpy(settings_daemon.keymap, "us", sizeof(settings_daemon.keymap));
    settings_daemon.global_ui_scale = 1.0f; // normal UI scale

    // Accessibility
    settings_daemon.screen_reader_enabled = 0; // disabled
}

void settingsd_apply_theme(theme_config_t *theme) {
    if (!theme) return;
    settings_daemon.theme = *theme;
}

void settingsd_apply_power_profile(power_config_t *power) {
    if (!power) return;
    settings_daemon.power = *power;
}
