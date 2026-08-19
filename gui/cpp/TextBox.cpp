// TextBox.cpp  —  Single-line text field with modern styling and syntax highlighting
#include "TextBox.hpp"
#include <cstring>
#include <cmath>

// External C drawing functions from fb_draw.c
extern "C" {
    void fb_fill_rect      (void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t);
    void fb_fill_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_rect_round(void*, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, int32_t);
    void fb_draw_text      (void*, uint32_t, int32_t, int32_t, const char*, uint32_t, uint32_t);
    int  fb_text_width     (const char* s);
}

// ── System colors / theme-aware defaults ─────────────────────────────────────
static constexpr uint32_t TB_BG_NORMAL     = 0xFFFFFFFF;    // White
static constexpr uint32_t TB_BG_FOCUSED    = 0xFFE8EDF5;   // Light blue-white
static constexpr uint32_t TB_BORDER_NORMAL = 0xFF808080;   // Gray
static constexpr uint32_t TB_BORDER_FOCUSED= 0xFF0066FF;   // Blue
static constexpr uint32_t TB_FG_NORMAL     = 0xFF000000;   // Black
static constexpr uint32_t TB_FG_PLACEHOLDER= 0xFF808080;   // Gray
static constexpr uint32_t TB_SEL_COL       = 0xFF0066FF;   // Blue selection
static constexpr uint32_t TB_CURSOR_COL    = 0xFFFFFFFF;   // White cursor

// ── Syntax-highlighting keyword list ────────────────────────────────────────
static constexpr const char* KEYWORDS[] = {
    "int", "return", "if", "else", "while", "for", "break", "continue",
    "struct", "typedef", "char", "float", "double", "void", "static",
    "const", "unsigned", "signed", "long", "short", "enum", "union",
    "extern", "goto", "switch", "case", "default", "do"
};

bool is_keyword(const char* s, int len) {
    for (int i = 0; i < (int)(sizeof(KEYWORDS)/sizeof(KEYWORDS[0])); ++i) {
        if (strncmp(s, KEYWORDS[i], len) == 0 && strlen(KEYWORDS[i]) == len)
            return true;
    }
    return false;
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void TextBox::paint_self(const Rect& /*dirty*/, void* fb, uint32_t stride) {
    Rect sr = screen_rect();
    bool foc = focused();

    // Background with rounded corners - theme colors
    fb_fill_rect_round(fb, stride, sr.x, sr.y, sr.w, sr.h,
                       foc ? TB_BG_FOCUSED : TB_BG_NORMAL, 6);

    // Border with rounded corners - theme colors
    fb_draw_rect_round(fb, stride, sr.x, sr.y, sr.w, sr.h,
                       foc ? TB_BORDER_FOCUSED : TB_BORDER_NORMAL, 6);

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

        fb_fill_rect(fb, stride, sx, ty, sw, FONT_H, TB_SEL_COL);
    }

    // Text or placeholder
    const char* disp = (len_ == 0) ? placeholder_ : buf_;
    uint32_t fg = (len_ == 0) ? TB_FG_PLACEHOLDER : TB_FG_NORMAL;

    // Syntax-highlighted rendering (if enabled)
    if (syntax_enabled_) {
        int x = sr.x + pad - scroll_offset_;
        int i = 0;
        while (disp[i] && x < sr.x + sr.w) {
            // Identify token
            if ((disp[i] >= 'A' && disp[i] <= 'Z') || (disp[i] >= 'a' && disp[i] <= 'z') || disp[i] == '_') {
                // identifier or keyword
                int start = i;
                while (disp[i] && ((disp[i] >= 'A' && disp[i] <= 'Z') || (disp[i] >= 'a' && disp[i] <= 'z') || (disp[i] >= '0' && disp[i] <= '9') || disp[i] == '_')) i++;
                int tok_len = i - start;
                if (tok_len >= 64) tok_len = 63;
                char token[64];
                ::memcpy(token, disp + start, tok_len);
                token[tok_len] = '\0';
                // Check against keyword list
                Color token_color = fg;
                for (int k = 0; k < (int)(sizeof(KEYWORDS)/sizeof(KEYWORDS[0])); ++k) {
                    if (strlen(KEYWORDS[k]) == tok_len && strncmp(token, KEYWORDS[k], tok_len) == 0) {
                        token_color = Colors::Accent;  // Blue for keywords
                        break;
                    }
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
        // Simple rendering
        fb_draw_text(fb, stride, sr.x + pad - scroll_offset_, ty,
                     disp, fg, 0x00000000u);
    }

    // Cursor bar
    if (foc && cursor_visible_) {
        char tmp[MAX_LEN + 1];
        ::memcpy(tmp, buf_, (size_t)cursor_);
        tmp[cursor_] = '\0';
        int cx = sr.x + pad + fb_text_width(tmp) * FONT_W - scroll_offset_;
        fb_fill_rect(fb, stride, cx, ty, 2, FONT_H, TB_CURSOR_COL);
    }
}