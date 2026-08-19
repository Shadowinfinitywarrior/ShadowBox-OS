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
    void set_color     (Color c);
    void set_align     (TextAlign a);
    void set_wrap      (WrapMode  w);
    void set_font_scale(int s);

    const char* text() const { return text_; }

protected:
    void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;

private:
    static constexpr int MAX_TEXT = 512;
    char      text_[MAX_TEXT] = {};
    Color     fg_    = Colors::Text;
    Color     bg_;  // Background color - initialized in constructor
    TextAlign align_ = TextAlign::Left;
    WrapMode  wrap_  = WrapMode::NoWrap;
    int       scale_ = 1;

    // Font dimensions
    static constexpr int FONT_W_ = 8;
    static constexpr int FONT_H_ = 16;
};
