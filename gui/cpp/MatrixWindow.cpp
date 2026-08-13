#include "MatrixWindow.hpp"
#include "c_std.h"

static uint64_t m_rand_state = 1234567;
static uint32_t m_rand() {
    m_rand_state = m_rand_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return m_rand_state >> 32;
}

MatrixWindow::MatrixWindow(Widget* parent) 
    : Window(parent), tick_acc_(0) {
    set_title("Matrix");
    set_size(COLS * 8 + 4, ROWS * 16 + 24);
    for (int i = 0; i < COLS; i++) {
        drops_[i] = -(m_rand() % 50);
        for (int j = 0; j < ROWS; j++) {
            chars_[i][j] = ' ';
        }
    }
}

void MatrixWindow::tick(int dt_ms) {
    Window::tick(dt_ms);
    tick_acc_ += dt_ms;
    if (tick_acc_ >= 50) {
        tick_acc_ -= 50;
        
        for (int i = 0; i < COLS; i++) {
            if (drops_[i] > 0 && drops_[i] < ROWS) {
                int y = drops_[i];
                chars_[i][y] = 33 + (m_rand() % 94);
            }
            drops_[i]++;
            if (drops_[i] > ROWS && (m_rand() % 10 == 0)) {
                drops_[i] = 0;
            }
        }
        mark_dirty();
    }
}

extern "C" {
    void fb_fill_rect(void* fb, uint32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fb_draw_text(void* fb, uint32_t stride, int32_t x, int32_t y, const char* s, uint32_t fg, uint32_t bg);
}

void MatrixWindow::paint_self(const Rect& dirty, void* fb, uint32_t stride) {
    Window::paint_self(dirty, fb, stride);
    
    Rect abs_rect = screen_rect();
    int area_x = abs_rect.x + 2;
    int area_y = abs_rect.y + 26;
    int area_w = abs_rect.w - 4;
    int area_h = abs_rect.h - 28;
    
    fb_fill_rect(fb, stride, area_x, area_y, area_w, area_h, 0xFF000000);
    
    // Draw characters
    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < ROWS; j++) {
            if (chars_[i][j] != ' ') {
                uint32_t color = 0xFF00FF00;
                if (drops_[i] - 1 == j) {
                    color = 0xFFFFFFFF; // Head of the drop is white
                } else if (drops_[i] > j) {
                    // Fade out
                    int dist = drops_[i] - j;
                    int g = 255 - (dist * 15);
                    if (g < 0) g = 0;
                    color = 0xFF000000 | (g << 8);
                }
                char str[2] = { chars_[i][j], 0 };
                fb_draw_text(fb, stride, area_x + i * 8, area_y + j * 16, str, color, 0xFF000000);
            }
        }
    }
}
