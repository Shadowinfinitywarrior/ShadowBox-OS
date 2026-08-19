// Window.cpp  —  Decorated window implementation with modern visual depth
#include "Window.hpp"
#include <cstdlib>
#include <cstdint>
#include <cmath>

// External C drawing functions from fb_draw.c
extern "C" {
    void fb_fill_rect       (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect       (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_draw_rect_round (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text       (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    void gui_fb_flip        (uint32_t y_offset);
    void* gui_fb_backbuf    ();
    uint32_t gui_fb_stride  ();
}

// ── Helper: close button lambda via a plain static trampoline ────────────
static void close_btn_clicked(Widget* w) {
    Window* win = static_cast<Window*>(w->user_data);
    if (win->on_close) win->on_close(win);
    else               win->start_close_animation();
}

static void min_btn_clicked(Widget* w) {
    Window* win = static_cast<Window*>(w->user_data);
    if (win->on_minimize) win->on_minimize(win);
}

// ── Color blending helpers ─────────────────────────────────────────────────

static uint32_t blend_alpha(uint32_t color, float alpha) {
    uint32_t a = (color >> 24) & 0xFF;
    uint32_t na = static_cast<uint32_t>(a * alpha);
    return (color & 0x00FFFFFFu) | (na << 24);
}

static uint32_t blend(uint32_t fg, uint32_t bg, float alpha) {
    uint32_t fa = static_cast<uint32_t>(alpha * 255);
    uint32_t fb = 255 - fa;
    uint32_t r = ((fg & 0xFF0000u) * fa + (bg & 0xFF0000u) * fb) / 255;
    uint32_t g = ((fg & 0x00FF00u) * fa + (bg & 0x00FF00u) * fb) / 255;
    uint32_t b = ((fg & 0x0000FFu) * fa + (bg & 0x0000FFu) * fb) / 255;
    uint32_t a_out = ((fg >> 24) * fa + (bg >> 24) * fb) / 255;
    return (a_out << 24) | (r << 16) | (g << 8) | b;
}

// ── Window constants ───────────────────────────────────────────────────────

static constexpr int BORDER_W       = 2;       // Window border width
static constexpr int TITLEBAR_H     = 32;      // Title bar height
static constexpr int RESIZE_ZONE    = 8;       // Resize grip zone size
static constexpr float ANIM_START_SCALE = 0.8f;  // Pop-in start scale
static constexpr float ANIM_START_ALPHA = 0.3f;   // Pop-in start alpha
static constexpr int   ANIM_DURATION_MS   = 150;   // Animation duration

// ── Focus/blur animation state ──────────────────────────────────────────────

static int focus_anim_progress = 0;
static bool focus_animating = false;

// ── Paint ──────────────────────────────────────────────────────────────────────

void Window::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();
    
    // Clamp alpha to valid range
    float anim_alpha = 1.0f;
    if (animating_) {
        float t = anim_progress_;
        // ease-out quadratic
        float eased = t * (2.0f - t);
        // scale factor from start to 1.0
        float scale = ANIM_START_SCALE + (1.0f - ANIM_START_SCALE) * eased;
        // compute centered scaled rect
        int cx = sr.x + sr.w / 2;
        int cy = sr.y + sr.h / 2;
        int w = static_cast<int>(sr.w * scale);
        int h = static_cast<int>(sr.h * scale);
        sr = { cx - w/2, cy - h/2, w, h };
        // alpha blending factor
        anim_alpha = ANIM_START_ALPHA + (1.0f - ANIM_START_ALPHA) * eased;
    }

    // Helper to blend alpha into a colour
    auto blend_alpha = [&](Color c) -> Color {
        uint32_t ac = (c >> 24) & 0xFF;
        uint32_t na = static_cast<uint32_t>(ac * anim_alpha);
        return (c & 0x00FFFFFFu) | (na << 24);
    };

    // Color constants from theme
    uint32_t window_bg = LightTheme::WindowBg;  // Could be theme-aware
    uint32_t title_bg  = LightTheme::Accent;
    uint32_t border_col = LightTheme::Border;
    uint32_t text_col  = LightTheme::Text;

    // ── Drop shadow (semi-transparent dark fill offset by 4px) ────────────
    // Draw shadow first, behind the window
    Rect shadow_rect = sr;
    shadow_rect.x -= 4;
    shadow_rect.y -= 4;
    shadow_rect.w += 8;
    shadow_rect.h += 8;
    fb_fill_rect_round(fb, stride,
                       shadow_rect.x, shadow_rect.y, shadow_rect.w, shadow_rect.h,
                       LightTheme::Shadow, 12);

    // ── Window body with rounded corners ────────────────────────────────────
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, sr.h,
                       blend_alpha(window_bg), 12);

    // ── Title bar: rounded top, squared-off bottom half ─────────────────────
    // Top rounded part
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, TITLEBAR_H,
                       blend_alpha(title_bg), 10);
    // Bottom squared part
    fb_fill_rect(fb, stride,
                 sr.x, sr.y + TITLEBAR_H / 2,
                 sr.w, TITLEBAR_H / 2,
                 blend_alpha(title_bg));

    // ── Border ───────────────────────────────────────────────────────────────
    fb_draw_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, blend_alpha(border_col));

    // ── Separator line below title bar ───────────────────────────────────────
    fb_fill_rect(fb, stride,
                 sr.x, sr.y + TITLEBAR_H,
                 sr.w, 1,
                 blend_alpha(border_col));

    // ── Draw close/min buttons if present ────────────────────────────────────
    // (Already drawn by their paint_self which uses theme colors)
}

// ── Input — drag & resize ────────────────────────────────────────────────────

bool Window::in_titlebar(Point pt) const {
    Rect sr = screen_rect();
    return (pt.x >= sr.x && pt.x < sr.x + sr.w &&
            pt.y >= sr.y && pt.y < sr.y + TITLEBAR_H);
}

bool Window::in_resize_zone(Point pt) const {
    if (!resizable) return false;
    Rect sr = screen_rect();
    Rect zone { sr.x + sr.w - RESIZE_ZONE,
                sr.y + sr.h - RESIZE_ZONE,
                RESIZE_ZONE, RESIZE_ZONE };
    return zone.contains(pt);
}

bool Window::on_mouse_press(const InputEvent& ev) {
    if (!screen_rect().contains(ev.pos)) return false;
    if (ev.button != MouseButton::Left) return false;

    raise_to_top();

    if (in_resize_zone(ev.pos)) {
        resizing_           = true;
        resize_origin_      = ev.pos;
        rect_before_resize_ = rect_;
        return true;
    }

    if (in_titlebar(ev.pos)) {
        // Don't drag if the click is on one of the chrome buttons
        Point local { ev.pos.x - screen_pos().x, ev.pos.y - screen_pos().y };
        if (close_btn_ && close_btn_->rect().contains(local)) return false;
        if (min_btn_   && min_btn_->rect().contains(local))   return false;

        dragging_    = true;
        drag_origin_ = ev.pos - screen_pos();
        return true;
    }

    return true;  // Consume click (bring to front) without dragging
}

bool Window::on_mouse_move(const InputEvent& ev) {
    if (dragging_) {
        Point new_pos = ev.pos - drag_origin_;
        set_pos(new_pos.x, new_pos.y);
        return true;
    }
    if (resizing_) {
        Point delta = ev.pos - resize_origin_;
        int new_w = rect_before_resize_.w + delta.x;
        int new_h = rect_before_resize_.h + delta.y;
        if (new_w < 200) new_w = 200;
        if (new_h < 120) new_h = 120;
        set_size(new_w, new_h);
        return true;
    }
    return false;
}

bool Window::on_mouse_release(const InputEvent& ev) {
    (void)ev;
    bool was = dragging_ || resizing_;
    dragging_ = resizing_ = false;
    return was;
}

// ─── Animation control implementations ───────────────────────────────────────

void Window::paint(const Rect& dirty_screen, void* fb, uint32_t stride) {
    if (animating_ && anim_progress_ < 1.0f) {
        // Draw window with animation (children omitted during pop-in/out)
        paint_self(dirty_screen, fb, stride);
    } else {
        // Normal painting (including children)
        Widget::paint(dirty_screen, fb, stride);
    }
}

void Window::start_anim_open() {
    animating_ = true;
    anim_elapsed_ms_ = 0;
    anim_open_ = true;
    anim_target_rect_ = rect_;
    anim_progress_ = 0.0f;
}

void Window::start_anim_close() {
    animating_ = true;
    anim_elapsed_ms_ = 0;
    anim_open_ = false;
    anim_target_rect_ = rect_;
    anim_progress_ = 0.0f;
}

void Window::start_close_animation() {
    start_anim_close();
}

bool Window::tick_anim(int dt_ms) {
    if (!animating_) return false;
    anim_elapsed_ms_ += dt_ms;
    if (anim_elapsed_ms_ >= ANIM_DURATION_MS) {
        anim_elapsed_ms_ = ANIM_DURATION_MS;
        anim_progress_ = 1.0f;
        animating_ = false;
    } else {
        anim_progress_ = static_cast<float>(anim_elapsed_ms_) / static_cast<float>(ANIM_DURATION_MS);
    }
    // Request repaint for animation progress
    mark_dirty();
    return animating_;
}

void Window::tick(int dt_ms) {
    // Update animation; hide after close animation finishes
    if (tick_anim(dt_ms)) {
        // animation still active
        return;
    }
    // No animation active; if close animation just finished, hide window
    if (!anim_open_) {
        set_visible(false);
    }
}

// ── C bridge ─────────────────────────────────────────────────────────────

extern "C" {