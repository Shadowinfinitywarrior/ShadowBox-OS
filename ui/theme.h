#ifndef SHADOWBOX_UI_THEME_H
#define SHADOWBOX_UI_THEME_H

#include <stdint.h>
#include "settingsd.h"

/* Initialise the theme engine. Must be called after settingsd_init(). */
void ui_theme_init(void);

/* Access the current theme configuration. */
const theme_config_t *ui_theme_get_current(void);

/* Set a new theme configuration. Returns 0 on success, -1 on error. */
int ui_theme_set_current(const theme_config_t *theme);
int ui_theme_apply(const theme_config_t *theme);

/* Cleanup resources (currently a no‑op). */
void ui_theme_cleanup(void);

/* Query helpers */
uint32_t ui_theme_get_accent_color(void);
uint32_t ui_theme_get_primary_color(void);
uint32_t ui_theme_get_secondary_color(void);
uint32_t ui_theme_get_background_color(void);
uint32_t ui_theme_get_foreground_color(void);
uint8_t ui_theme_is_dark(void);

#endif // SHADOWBOX_UI_THEME_H