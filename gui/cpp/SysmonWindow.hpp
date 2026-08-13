#ifndef SYSMON_WINDOW_HPP
#define SYSMON_WINDOW_HPP

#include "Window.hpp"
#include "Label.hpp"

class SysmonWindow : public Window {
public:
    SysmonWindow(Widget* parent);
    virtual void tick(int dt_ms) override;

private:
    Label* mem_label_;
    Label* cpu_label_;
    int tick_timer_;
};

#endif
