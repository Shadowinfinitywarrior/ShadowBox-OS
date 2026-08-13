#pragma once
#include "Window.hpp"

class MatrixWindow : public Window {
public:
    MatrixWindow(Widget* parent = nullptr);
    
    virtual void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;
    virtual void tick(int dt_ms) override;

private:
    static const int COLS = 60;
    static const int ROWS = 25;
    
    int drops_[COLS];
    char chars_[COLS][ROWS];
    
    int tick_acc_;
};
