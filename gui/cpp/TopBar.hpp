#ifndef TOPBAR_HPP
#define TOPBAR_HPP

#include "Widget.hpp"
#include "Label.hpp"

class TopBar : public Widget {
public:
    TopBar(Widget* parent, int screen_width);
    virtual void paint_self(const Rect& clip, void* fb, uint32_t stride) override;

private:
    Label* title_label_;
};

#endif
