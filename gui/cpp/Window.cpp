// Window.cpp  —  Decorated window implementation
#include "Window.hpp"
#include <cstdlib>
#include <cstdint>

extern "C" {
    void fb_fill_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_draw_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text      (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
}

// ─── Helper: close button lambda via a plain static trampoline ────────────
static void close_btn_clicked(Widget* w) {
    Window* win = static_cast<Window*>(w->user_data);
    if (win->on_close) win->on_close(win);
    else               win->start_close_animation();
}

static void min_btn_clicked(Widget* w) {
    Window* win = static_cast<Window*>(w->user_data);
    if (win->on_minimize) win->on_minimize(win);
}

// ─────────────────────────────────────────────────────────────────────────
//  Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────

Window::Window(Widget* parent) : Widget(parent) {
    set_tag("Window");
    set_flag(WF_CLIP, true);

    // Title label (spans full title bar; painted behind buttons)
    title_label_ = new Label(nullptr);
    title_label_->set_color(Colors::Text);
    title_label_->set_align(TextAlign::Center);
    Widget::add_child(title_label_);

    // Close button
    if (closeable) {
        close_btn_ = new Button(nullptr);
        close_btn_->set_label("x");
        close_btn_->bg_normal     = Colors::Transparent;
        close_btn_->bg_hover      = Colors::Danger;
        close_btn_->bg_press      = dim(Colors::Danger, 180);
        close_btn_->fg_normal     = Colors::Text;
        close_btn_->fg_hover      = Colors::White;
        close_btn_->corner_radius = 4;
        close_btn_->on_clicked    = close_btn_clicked;
        close_btn_->user_data     = this;
        Widget::add_child(close_btn_);
    }

    // Minimise button
    if (minimizable) {
        min_btn_ = new Button(nullptr);
        min_btn_->set_label("-");
        min_btn_->bg_normal     = Colors::Transparent;
        min_btn_->bg_hover      = Colors::Warning;
        min_btn_->bg_press      = dim(Colors::Warning, 180);
        min_btn_->fg_normal     = Colors::Text;
        min_btn_->fg_hover      = Colors::Black;
        min_btn_->corner_radius = 4;
        min_btn_->on_clicked    = min_btn_clicked;
        min_btn_->user_data     = this;
        Widget::add_child(min_btn_);
    }

    layout_controls();
    start_anim_open();
}

Window::~Window() {
    // Children are freed manually since we allocated them
    delete title_label_;
    delete close_btn_;
    delete min_btn_;
}

// ─── Title ────────────────────────────────────────────────────────────────

void Window::set_title(const char* t) {
    int i = 0;
    while (t[i] && i < 127) { title_[i] = t[i]; ++i; }
    title_[i] = '\0';
    if (title_label_) title_label_->set_text(t);
}

// ─── Geometry ─────────────────────────────────────────────────────────────

Rect Window::client_rect() const {
    Point p = screen_pos();
    return {
        p.x + BORDER_W,
        p.y + TITLEBAR_H,
        rect_.w - BORDER_W * 2,
        rect_.h - TITLEBAR_H - BORDER_W
    };
}

void Window::add_client(Widget* child) {
    Widget::add_child(child);
    Rect r = child->rect();
    // Offset relative to window so it lands in the client area
    child->set_rect({ r.x + BORDER_W, r.y + TITLEBAR_H, r.w, r.h });
}

void Window::layout_controls() {
    int y = (TITLEBAR_H - 20) / 2;
    int x = rect_.w;

    if (close_btn_) {
        x -= 28;
        close_btn_->set_rect({ x, y, 24, 20 });
    }
    if (min_btn_) {
        x -= 28;
        min_btn_->set_rect({ x, y, 24, 20 });
    }
    if (title_label_) {
        title_label_->set_rect({ 0, 0, rect_.w, TITLEBAR_H });
    }
}

void Window::on_resize() {
    layout_controls();
}

// ─── Paint ────────────────────────────────────────────────────────────────

void Window::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();
    // Apply pop-in animation scaling and alpha blending
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
        uint32_t a = (c >> 24) & 0xFF;
        uint32_t na = static_cast<uint32_t>(a * anim_alpha);
        return (c & 0x00FFFFFFu) | (na << 24);
    };

    // Drop shadow (semi-transparent dark fill offset by 4px)
    fb_fill_rect_round(fb, stride,
                       sr.x + 4, sr.y + 4, sr.w, sr.h,
                       blend_alpha(0x44000000u), 10);

    // Window body
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, sr.h,
                       blend_alpha(window_bg), 10);

    // Title bar (rounded top, squared-off bottom half)
    fb_fill_rect_round(fb, stride,
                       sr.x, sr.y, sr.w, TITLEBAR_H,
                       blend_alpha(title_bg), 10);
    fb_fill_rect(fb, stride,
                 sr.x, sr.y + TITLEBAR_H / 2,
                 sr.w, TITLEBAR_H / 2,
                 blend_alpha(title_bg));

    // Border
    fb_draw_rect(fb, stride, sr.x, sr.y, sr.w, sr.h, blend_alpha(border_col));

    // Separator line below title bar
    fb_fill_rect(fb, stride,
                 sr.x, sr.y + TITLEBAR_H,
                 sr.w, 1,
                 blend_alpha(border_col));
}

// ─── Input — drag & resize ────────────────────────────────────────────────

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

