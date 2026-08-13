// TextBox.hpp  —  Single-line editable text field
#pragma once
#include "Widget.hpp"

// ═══════════════════════════════════════════════════════════════════════════
//  TextBox
// ═══════════════════════════════════════════════════════════════════════════
class TextBox : public Widget {
public:
    static constexpr int MAX_LEN = 4096;
    // Font dimensions (from fb_draw.c)
    static constexpr int FONT_W = 8;
    static constexpr int FONT_H = 16;

    explicit TextBox(Widget* parent = nullptr);

    void        set_text       (const char* t);
    const char* text           () const { return buf_; }
    int         text_length    () const { return len_; }
    void        set_placeholder(const char* p);
    void        set_max_length (int m) { max_len_ = (m < MAX_LEN) ? m : MAX_LEN; }
    void        enable_syntax(bool enable) { syntax_enabled_ = enable; }

    // Theme colours
    Color bg_normal       = Colors::DarkGray;
    Color bg_focused      = Colors::MidGray;
    Color fg_normal       = Colors::Text;
    Color fg_placeholder  = Colors::TextDim;
    Color cursor_col      = Colors::Accent;
    Color sel_col         = 0x446699FFu;
    Color border_normal   = Colors::Border;
    Color border_focused  = Colors::Accent;

    // Callbacks
    using ChangeFn = void(*)(TextBox*, void* ctx);
    using SubmitFn = void(*)(TextBox*, void* ctx);
    ChangeFn on_change = nullptr;
    SubmitFn on_submit = nullptr;
    void*    cb_ctx    = nullptr;

protected:
    void paint_self    (const Rect& dirty, void* fb, uint32_t stride) override;
    bool on_key_press  (const InputEvent& ev) override;
    bool on_mouse_press(const InputEvent& ev) override;
    void on_focus_gained() override { cursor_visible_ = true;  mark_dirty(); }
    void on_focus_lost  () override { cursor_visible_ = false; mark_dirty(); }

private:
    char  buf_[MAX_LEN + 1]  = {};
    char  placeholder_[128]  = {};
    int   len_           = 0;
    int   cursor_        = 0;    // byte position
    int   sel_start_     = -1;
    int   sel_end_       = -1;
    int   scroll_offset_ = 0;    // horizontal scroll in pixels
    int   max_len_       = MAX_LEN;
    bool  cursor_visible_= true;
    bool  syntax_enabled_ = false;

    void insert      (uint32_t codepoint);
    void delete_char (bool forward);
    void clear_sel   ();
    void notify_change();
    int  char_at_x   (int local_x) const;
};
