#pragma once
#include "Window.hpp"

class TetrisWindow : public Window {
public:
    TetrisWindow(Widget* parent = nullptr);
    
    virtual void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;
    virtual void tick(int dt_ms) override;
    virtual bool on_key_press(const InputEvent& ev) override;

private:
    void reset();
    void spawn_piece();
    void lock_piece();
    bool collides(int px, int py, uint16_t m);
    uint16_t rotate_piece(uint16_t m);

    signed char board_[22 * 10]; // 22 rows, 10 columns (top 2 rows are hidden normally but useful)
    
    int tetris_type_;
    int tetris_next_;
    int tetris_px_;
    int tetris_py_;
    uint16_t current_piece_;
    bool tetris_over_;
    
    int score_;
    int lines_;
    
    int tick_acc_;
};
