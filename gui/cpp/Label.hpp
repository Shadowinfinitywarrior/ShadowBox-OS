// Label.hpp  —  Static text widget
#pragma once
#include "Widget.hpp"

enum class TextAlign : uint8_t { Left, Center, Right };
enum class WrapMode  : uint8_t { NoWrap, WordWrap };

// ═══════════════════════════════════════════════════════════════════════════
//  Label  —  Non-interactive text display
// ═══════════════════════════════════════════════════════════════════════════
class Label : public Widget {
public:
    explicit Label(Widget* parent = nullptr);

    void set_text      (const char* text);
    void set_color     (Color c)       { fg_ = c;     mark_dirty(); }
    void set_align     (TextAlign a)   { align_ = a;  mark_dirty(); }
    void set_wrap      (WrapMode  w)   { wrap_  = w;  mark_dirty(); }
    void set_font_scale(int s)         { scale_ = s;  mark_dirty(); }

    const char* text() const { return text_; }

protected:
    void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;

private:
    static constexpr int MAX_TEXT = 512;
    char      text_[MAX_TEXT] = {};
    Color     fg_    = Colors::Text;
    TextAlign align_ = TextAlign::Left;
    WrapMode  wrap_  = WrapMode::NoWrap;
    int       scale_ = 1;

    // Font dimensions
    static constexpr int FONT_W_ = 8;
    static constexpr int FONT_H_ = 16;
};
