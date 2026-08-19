// Colors.hpp — Modern color theme system for ShadowBox GUI
// Freestanding C++17 compatible — no STL, no exceptions
#pragma once

#include <cstdint>

// ── Default / Built-in Themes ──────────────────────────────────────────────

// Light theme colors (ARGB format, premultiplied alpha)
struct LightTheme {
    static constexpr uint32_t WindowBg   = 0xFF1E1E1E;  // Dark gray background
    static constexpr uint32_t Text       = 0xFFFFFFFF;  // White text
    static constexpr uint32_t TextDim    = 0xFFB0B0B0;  // Dimmed text
    static constexpr uint32_t Shadow     = 0x44000000;  // Semi-transparent shadow
    static constexpr uint32_t Border     = 0xFF808080;  // Gray border
    static constexpr uint32_t Accent     = 0xFF0066FF;  // Blue accent
    static constexpr uint32_t AccentHover= 0xFF0055DD;  // Blue hover
    static constexpr uint32_t AccentPress= 0xFF0044CC;  // Blue press
    static constexpr uint32_t Warning    = 0xFFCC6600;  // Orange warning
    static constexpr uint32_t Danger     = 0xFFCC0000;  // Red danger
    static constexpr uint32_t Success    = 0xFF009900;  // Green success
    static constexpr uint32_t Background = 0xFFFAFAFA;  // Light gray bg
    static constexpr uint32_t Surface    = 0xFFFFFFFF;  // White surface
};

// Dark theme colors (ARGB format, premultiplied auto)
struct DarkTheme {
    static constexpr uint32_t WindowBg   = 0xFF121212;  // Dark background
    static constexpr uint32_t Text       = 0xFFE0E0E0;  // Light text
    static constexpr uint32_t TextDim    = 0xFF808080;  // Dimmed text
    static constexpr uint32_t Shadow     = 0x66000000;  // Slightly opaque shadow
    static constexpr uint32_t Border     = 0xFF505050;  // Medium gray border
    static constexpr uint32_t Accent     = 0xFF0088FF;  // Bright blue accent
    static constexpr uint32_t AccentHover= 0xFF0077DD;  // Blue hover
    static constexpr uint32_t AccentPress= 0xFF0066CC;  // Blue press
    static constexpr uint32_t Warning    = 0xFFE57373;  // Red warning
    static constexpr uint32_t Danger     = 0xFFFFCC80;  // Amber danger
    static constexpr uint32_t Success    = 0xFF81C784;  // Light green success
    static constexpr uint32_t Background = 0xFF212121;  // Dark gray bg
    static constexpr uint32_t Surface    = 0xFF2B2B2B;  // Card surface
};

// ── Color utility functions ────────────────────────────────────────────────

// Dim a color by multiplying its alpha (0-255 range)
inline uint32_t dim(uint32_t color, int factor) {
    // Factor: 100 = full, 180 = 80%, 200 = 90%
    // Extract alpha channel (bits 24-31) and scale it
    uint32_t alpha = (color >> 24) & 0xFF;
    alpha = static_cast<uint32_t>(alpha * factor / 200);
    return (color & 0x00FFFFFFu) | (alpha << 24);
}

// Lighten a color slightly
inline uint32_t lighten(uint32_t color, int factor) {
    // Factor: positive = lighten, negative = darken
    uint32_t alpha = (color >> 24) & 0xFF;
    alpha = static_cast<uint32_t>(alpha + factor);
    if (alpha > 255) alpha = 255;
    return (color & 0x00FFFFFFu) | (alpha << 24);
}

// Blend two colors with alpha
inline uint32_t blend(uint32_t fg, uint32_t bg, int alpha) {
    // alpha: 0 = bg, 255 = fg
    int a = alpha;
    if (a <= 0) return bg;
    if (a >= 255) return fg;
    uint32_t fa = static_cast<uint32_t>(a);
    uint32_t fb = 255 - fa;
    uint32_t r = ((fg & 0xFF0000u) * fa + (bg & 0xFF0000u) * fb) / 255;
    uint32_t g = ((fg & 0x00FF00u) * fa + (bg & 0x00FF00u) * fb) / 255;
    uint32_t b = ((fg & 0x0000FFu) * fa + (bg & 0x0000FFu) * fb) / 255;
    uint32_t a_out = ((fg >> 24) * fa + (bg >> 24) * fb) / 255;
    return (a_out << 24) | (r << 16) | (g << 8) | b;
}

// Get system color from theme
inline uint32_t sys_color(const char* name) {
    // Placeholder - will be overridden by active theme
    return 0xFFFFFFFF;
}

// Active theme pointer - set by GUI manager
extern const LightTheme light_theme;
extern const DarkTheme dark_theme;

// Convenience macros for theme colors
#define COLOR_THEME(theme, color) theme##_color##color
#define THEME_COLOR(c) COLOR_THEME((active_theme == "dark" ? dark : light), c)

// Active theme selection
extern const char* active_theme;

// Inline theme color accessors
inline uint32_t window_bg() { return active_theme && strcmp(active_theme, "dark") ? DarkTheme::WindowBg : LightTheme::WindowBg; }
inline uint32_t text_color()  { return active_theme && strcmp(active_theme, "dark") ? DarkTheme::Text : LightTheme::Text; }
inline uint32_t accent_color() { return active_theme && strcmp(active_theme, "dark") ? DarkTheme::Accent : LightTheme::Accent; }
inline uint32_t shadow_color() { return active_theme && strcmp(active_theme, "dark") ? DarkTheme::Shadow : LightTheme::Shadow; }
inline uint32_t border_color() { return active_theme && strcmp(active_theme, "dark") ? DarkTheme::Border : LightTheme::Border; }