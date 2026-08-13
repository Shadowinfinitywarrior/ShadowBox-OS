#pragma once
#include "Window.hpp"

class SnakeWindow : public Window {
public:
    SnakeWindow(Widget* parent = nullptr);
    
    virtual void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;
    virtual void tick(int dt_ms) override;
    virtual bool on_key_press(const InputEvent& ev) override;

private:
    void reset();
    void spawn_food();

    static constexpr int MAX_SNAKE = 64;
    static constexpr int GRID_SIZE = 10;
    
    int snake_x_[MAX_SNAKE];
    int snake_y_[MAX_SNAKE];
    int snake_len_;
    int snake_dir_; // 0=Up, 1=Right, 2=Down, 3=Left
    bool snake_dead_;
    
    int food_x_;
    int food_y_;
    
    int tick_acc_;
};
