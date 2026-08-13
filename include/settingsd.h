#ifndef SHADOWBOX_SETTINGSD_H
#define SHADOWBOX_SETTINGSD_H

#include "types.h"

// Theme Engine Config
typedef struct theme_config {
    uint8_t dark_mode;
    char accent_color_hex[8];
    char icon_theme[64];
    char cursor_theme[64];
    char primary_color_hex[8];
    char secondary_color_hex[8];
    char background_color_hex[8];
    char foreground_color_hex[8];
} theme_config_t;

// Power Config
typedef struct power_config {
    uint32_t screen_dim_timeout_sec;
    uint32_t sleep_timeout_sec;
    uint8_t power_saving_mode; // Triggers CPU underclock & reduces animations
} power_config_t;

// Settings Daemon State
typedef struct settings_daemon {
    theme_config_t theme;
    power_config_t power;
    
    // Input / Display overrides mapped via libinput/drm config files
    char keymap[16]; // e.g. "us", "uk", "dvorak"
    float global_ui_scale;
    
    // Accessibility is mapped to a11y_settings_t in accessibility.h
    uint8_t screen_reader_enabled;
} settings_daemon_t;

void settingsd_init(void);
void settingsd_apply_theme(theme_config_t *theme);
void settingsd_apply_power_profile(power_config_t *power);

#endif
