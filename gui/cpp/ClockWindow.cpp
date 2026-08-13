#include "ClockWindow.hpp"
#include "../../userland/sys.h"

// Basic sprintf-like for integers
static void fmt_time(char* buf, int hour, int min, int sec) {
    auto fmt2 = [](char* b, int val) {
        b[0] = '0' + (val / 10);
        b[1] = '0' + (val % 10);
    };
    fmt2(buf, hour);
    buf[2] = ':';
    fmt2(buf + 3, min);
    buf[4] = ':';
    fmt2(buf + 6, sec);
    buf[8] = '\0';
}

ClockWindow::ClockWindow(Widget* parent) : Window(parent), ticks_(0) {
    set_title("Clock");
    set_size(200, 100);
    
    time_label_ = new Label(this);
    time_label_->set_pos(10, 30);
    time_label_->set_size(180, 40);
    time_label_->set_color(0x3498DB); // Blue color
}

void ClockWindow::tick(int dt_ms) {
    Window::tick(dt_ms);
    ticks_ += dt_ms;
    
    // Update every 100 ms (10 ticks roughly) to be responsive
    if (ticks_ > 100) {
        ticks_ = 0;
        
        uint64_t t = sys_times(0) / 100; // Assuming sys_times is in 10ms units based on previous code
        int sec = t % 60;
        int min = (t / 60) % 60;
        int hour = (t / 3600) % 24;
        
        char buf[16];
        fmt_time(buf, hour, min, sec);
        
        time_label_->set_text(buf);
    }
}
