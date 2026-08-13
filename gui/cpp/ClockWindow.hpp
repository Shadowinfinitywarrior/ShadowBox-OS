#pragma once
#include "Window.hpp"
#include "Label.hpp"

class ClockWindow : public Window {
public:
    ClockWindow(Widget* parent = nullptr);
    virtual void tick(int dt_ms) override;

private:
    Label* time_label_;
    uint64_t ticks_;
};
