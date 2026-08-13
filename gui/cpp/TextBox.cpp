// TextBox.cpp  —  Single-line text field implementation
#include "TextBox.hpp"
#include <cstring>

extern "C" {
    void fb_fill_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text      (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width     (const char* s);
}

// Key codepoints (reuse same encoding as InputRouter)
static constexpr uint32_t KEY_LEFT      = 0x10000001u;
static constexpr uint32_t KEY_RIGHT     = 0x10000002u;
static constexpr uint32_t KEY_HOME      = 0x10000010u;
static constexpr uint32_t KEY_END       = 0x10000011u;
static constexpr uint32_t KEY_BACKSPACE = 0x08u;
static constexpr uint32_t KEY_DELETE    = 0x7Fu;
static constexpr uint32_t KEY_RETURN    = 0x0Du;

// ─────────────────────────────────────────────────────────────────────────

TextBox::TextBox(Widget* parent) : Widget(parent) {
    set_tag("TextBox");
    set_flag(WF_FOCUSABLE, true);
}

void TextBox::set_text(const char* t) {
    len_ = 0;
    while (t[len_] && len_ < max_len_) {
        buf_[len_] = t[len_];
        ++len_;
    }
    buf_[len_] = '\0';
    cursor_       = len_;
    scroll_offset_= 0;
    mark_dirty();
}

void TextBox::set_placeholder(const char* p) {
    int i = 0;
    while (p[i] && i < 127) { placeholder_[i] = p[i]; ++i; }
    placeholder_[i] = '\0';
    mark_dirty();
}

// ─── Paint ────────────────────────────────────────────────────────────────

void TextBox::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr  = screen_rect();
    bool foc = focused();

    // Background
    fb_fill_rect_round(fb, stride, sr.x, sr.y, sr.w, sr.h,
                       foc ? bg_focused : bg_normal, 6);
    // Border
    fb_draw_rect_round(fb, stride, sr.x, sr.y, sr.w, sr.h,
                       foc ? border_focused : border_normal, 6);

    constexpr int pad = 8;
    int ty = sr.y + (sr.h - FONT_H) / 2;

    // Selection highlight
    if (foc && sel_start_ >= 0 && sel_end_ > sel_start_) {
        char tmp[MAX_LEN + 1];
        ::memcpy(tmp, buf_, (size_t)sel_start_);
        tmp[sel_start_] = '\0';
        int sx = sr.x + pad + fb_text_width(tmp) * FONT_W - scroll_offset_;

        ::memcpy(tmp, buf_ + sel_start_, (size_t)(sel_end_ - sel_start_));
        tmp[sel_end_ - sel_start_] = '\0';
        int sw = fb_text_width(tmp) * FONT_W;

        fb_fill_rect(fb, stride, sx, ty, sw, FONT_H, sel_col);
    }

    // Text or placeholder
    const char* disp = (len_ == 0) ? placeholder_ : buf_;
    Color       fg   = (len_ == 0) ? fg_placeholder : fg_normal;
    if (syntax_enabled_) {
        int x = sr.x + pad - scroll_offset_;
        int i = 0;
        while (disp[i] && x < sr.x + sr.w) {
            // Identify token
            if ((disp[i] >= 'A' && disp[i] <= 'Z') || (disp[i] >= 'a' && disp[i] <= 'z') || disp[i] == '_') {
                // identifier or keyword
                int start = i;
                while (disp[i] && ((disp[i] >= 'A' && disp[i] <= 'Z') || (disp[i] >= 'a' && disp[i] <= 'z') || (disp[i] >= '0' && disp[i] <= '9') || disp[i] == '_')) i++;
                char token[64];
                int len = i - start;
                if (len >= (int)sizeof(token)) len = sizeof(token)-1;
                ::memcpy(token, disp + start, len);
                token[len] = '\0';
                // keyword list
                static const char* keywords[] = {"int","return","if","else","while","for","break","continue","struct","typedef","char","float","double","void","static","const","unsigned","signed","long","short","enum","union","extern","goto","switch","case","default","do"};
                Color token_color = fg;
                for (const char* kw : keywords) {
                    if (strcmp(token, kw) == 0) { token_color = Colors::Accent; break; }
                }
                fb_draw_text(fb, stride, x, ty, token, token_color, 0x00000000u);
                x += fb_text_width(token) * FONT_W;
            } else {
                // Other character (including space, punctuation)
                char chbuf[2] = { disp[i], '\0' };
                fb_draw_text(fb, stride, x, ty, chbuf, fg, 0x00000000u);
                x += fb_text_width(chbuf) * FONT_W;
                i++;
            }
        }
    } else {
        fb_draw_text(fb, stride, sr.x + pad - scroll_offset_, ty,
                     disp, fg, 0x00000000u);
    }

    // Cursor bar
    if (foc && cursor_visible_) {
        char tmp[MAX_LEN + 1];
        ::memcpy(tmp, buf_, (size_t)cursor_);
        tmp[cursor_] = '\0';
        int cx = sr.x + pad + fb_text_width(tmp) * FONT_W - scroll_offset_;
        fb_fill_rect(fb, stride, cx, ty, 2, FONT_H, cursor_col);
    }
}

// ─── Input ────────────────────────────────────────────────────────────────

bool TextBox::on_mouse_press(const InputEvent& ev) {
    if (!screen_rect().contains(ev.pos)) return false;
    int local_x = ev.pos.x - screen_pos().x - 8 + scroll_offset_;
    cursor_  = char_at_x(local_x);
    sel_start_ = sel_end_ = -1;
    mark_dirty();
    return true;
}

bool TextBox::on_key_press(const InputEvent& ev) {
    switch (ev.key) {
        case KEY_LEFT:
            if (cursor_ > 0) --cursor_;
            clear_sel();  mark_dirty();  return true;

        case KEY_RIGHT:
            if (cursor_ < len_) ++cursor_;
            clear_sel();  mark_dirty();  return true;

        case KEY_HOME:
            cursor_ = 0;  mark_dirty();  return true;

        case KEY_END:
            cursor_ = len_;  mark_dirty();  return true;

        case KEY_BACKSPACE:
            if (cursor_ > 0) { --cursor_;  delete_char(false); }
            return true;

        case KEY_DELETE:
            if (cursor_ < len_) delete_char(true);
            return true;

        case KEY_RETURN:
            if (on_submit) on_submit(this, cb_ctx);
            return true;

        default:
            // Accept printable ASCII
            if (ev.key >= 0x20u && ev.key <= 0x7Eu) {
                insert(ev.key);
                return true;
            }
            return false;
    }
}

// ─── Private helpers ──────────────────────────────────────────────────────

void TextBox::insert(uint32_t ch) {
    if (len_ >= max_len_) return;
    ::memmove(&buf_[cursor_ + 1], &buf_[cursor_],
              (size_t)(len_ - cursor_ + 1));
    buf_[cursor_++] = (char)ch;
    ++len_;

    // Scroll right if cursor escaped the visible area
    char tmp[MAX_LEN + 1];
    ::memcpy(tmp, buf_, (size_t)cursor_);
    tmp[cursor_] = '\0';
    int text_x    = fb_text_width(tmp) * FONT_W;
    int visible_w = rect_.w - 16;
    if (text_x - scroll_offset_ > visible_w)
        scroll_offset_ = text_x - visible_w;

    notify_change();
    mark_dirty();
}

void TextBox::delete_char(bool forward) {
    if (!forward && cursor_ > 0) {
        ::memmove(&buf_[cursor_ - 1], &buf_[cursor_],
                  (size_t)(len_ - cursor_ + 1));
        --len_;
        --cursor_;
    } else if (forward && cursor_ < len_) {
        ::memmove(&buf_[cursor_], &buf_[cursor_ + 1],
                  (size_t)(len_ - cursor_));
        --len_;
        buf_[len_] = '\0';
    }
    notify_change();
    mark_dirty();
}

void TextBox::clear_sel() {
    sel_start_ = sel_end_ = -1;
}

void TextBox::notify_change() {
    if (on_change) on_change(this, cb_ctx);
}

int TextBox::char_at_x(int local_x) const {
    if (local_x <= 0) return 0;
    char tmp[MAX_LEN + 1];
    for (int i = 0; i <= len_; ++i) {
        ::memcpy(tmp, buf_, (size_t)i);
        tmp[i] = '\0';
        if (fb_text_width(tmp) * FONT_W >= local_x) return i;
    }
    return len_;
}
