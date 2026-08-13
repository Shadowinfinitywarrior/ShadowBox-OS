#include "TetrisWindow.hpp"

extern "C" {
    void fb_fill_rect(void* fb, uint32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fb_draw_text(void* fb, uint32_t stride, int32_t x, int32_t y, const char* s, uint32_t fg, uint32_t bg);
}

static const uint16_t tetris_shapes[7] = {
    0x0F00, 0x6600, 0x4E00, 0x6C00, 0xC600, 0x44C0, 0x88C0
};
static const uint32_t tetris_colors[7] = {
    0x00BCD4, 0xFFEB3B, 0x9C27B0, 0x4CAF50, 0xF44336, 0x2196F3, 0xFF9800
};

static uint64_t tetris_rng_state = 0x87654321ULL;
static uint32_t rng_next() {
    tetris_rng_state = tetris_rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return tetris_rng_state >> 32;
}

TetrisWindow::TetrisWindow(Widget* parent) : Window(parent), tick_acc_(0) {
    set_title("Tetris");
    set_size(160, 300); 
    reset();
}

void TetrisWindow::reset() {
    for (int i = 0; i < 22 * 10; i++) board_[i] = 0;
    score_ = 0;
    lines_ = 0;
    tetris_over_ = false;
    tetris_next_ = (int)(rng_next() % 7);
    spawn_piece();
    mark_dirty();
}

uint16_t TetrisWindow::rotate_piece(uint16_t m) {
    uint16_t r = 0;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (m & (1 << (y * 4 + x)))
                r |= (uint16_t)(1 << (x * 4 + (3 - y)));
    return r;
}

bool TetrisWindow::collides(int px, int py, uint16_t m) {
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (m & (1 << (y * 4 + x))) {
                int bx = px + x, by = py + y;
                if (bx < 0 || bx >= 10 || by >= 22) return true;
                if (by >= 0 && board_[by * 10 + bx]) return true;
            }
    return false;
}

void TetrisWindow::spawn_piece() {
    tetris_type_ = tetris_next_;
    tetris_next_ = (int)(rng_next() % 7);
    tetris_px_ = 3;
    tetris_py_ = 0;
    current_piece_ = tetris_shapes[tetris_type_];
    if (collides(tetris_px_, tetris_py_, current_piece_))
        tetris_over_ = true;
}

void TetrisWindow::lock_piece() {
    uint16_t m = current_piece_;
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            if (m & (1 << (y * 4 + x))) {
                int bx = tetris_px_ + x, by = tetris_py_ + y;
                if (by >= 0 && by < 22) board_[by * 10 + bx] = (signed char)(tetris_type_ + 1);
            }
    int cleared_lines = 0;
    for (int y = 21; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < 10; x++) if (!board_[y * 10 + x]) { full = false; break; }
        if (full) {
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < 10; x++)
                    board_[yy * 10 + x] = board_[(yy - 1) * 10 + x];
            for (int x = 0; x < 10; x++) board_[x] = 0;
            cleared_lines++;
            y++;
        }
    }
    lines_ += cleared_lines;
    if (cleared_lines == 1) score_ += 100;
    else if (cleared_lines == 2) score_ += 300;
    else if (cleared_lines == 3) score_ += 500;
    else if (cleared_lines >= 4) score_ += 800;
    spawn_piece();
}

void TetrisWindow::tick(int dt_ms) {
    Window::tick(dt_ms);
    if (tetris_over_) return;
    
    tick_acc_ += dt_ms;
    if (tick_acc_ < 500) return; // Drop every 500ms
    tick_acc_ -= 500;
    
    uint16_t m = current_piece_;
    if (!collides(tetris_px_, tetris_py_ + 1, m)) {
        tetris_py_++;
    } else {
        lock_piece();
    }
    mark_dirty();
}

bool TetrisWindow::on_key_press(const InputEvent& ev) {
    if (tetris_over_) {
        if (ev.key == 'r' || ev.key == 'R') {
            reset();
        }
        return true;
    }
    
    uint16_t m = current_piece_;
    if ((ev.key == 'a' || ev.key == 0x03) && !collides(tetris_px_ - 1, tetris_py_, m)) {
        tetris_px_--;
        mark_dirty();
        return true;
    }
    else if ((ev.key == 'd' || ev.key == 0x04) && !collides(tetris_px_ + 1, tetris_py_, m)) {
        tetris_px_++;
        mark_dirty();
        return true;
    }
    else if (ev.key == 'w' || ev.key == 0x01) {
        uint16_t r = rotate_piece(m);
        int tries[3] = {0, -1, 1};
        for (int i = 0; i < 3; i++) {
            if (!collides(tetris_px_ + tries[i], tetris_py_, r)) {
                tetris_px_ += tries[i];
                current_piece_ = r;
                break;
            }
        }
        mark_dirty();
        return true;
    } 
    else if (ev.key == 's' || ev.key == 0x02) {
        while (!collides(tetris_px_, tetris_py_ + 1, m)) tetris_py_++;
        score_ += 2;
        lock_piece();
        mark_dirty();
        return true;
    }
    
    return Window::on_key_press(ev);
}

void TetrisWindow::paint_self(const Rect& dirty, void* fb, uint32_t stride) {
    Window::paint_self(dirty, fb, stride);
    
    Rect abs_rect = screen_rect();
    int area_x = abs_rect.x + 2;
    int area_y = abs_rect.y + 26;
    int area_w = abs_rect.w - 4;
    int area_h = abs_rect.h - 28;
    
    fb_fill_rect(fb, stride, area_x, area_y, area_w, area_h, 0x0F0F23);
    
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 10; x++) {
            signed char v = board_[y * 10 + x];
            fb_fill_rect(fb, stride, area_x + 4 + x * 12, area_y + 4 + y * 12, 11, 11, v ? tetris_colors[v - 1] : 0x1A1A3E);
        }
    }
    
    if (!tetris_over_) {
        uint16_t m = current_piece_;
        
        int ghost_y = tetris_py_;
        while (!collides(tetris_px_, ghost_y + 1, m)) ghost_y++;
        
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (m & (1 << (y * 4 + x))) {
                    int bx = tetris_px_ + x, by = ghost_y + y;
                    if (bx >= 0 && bx < 10 && by >= 0 && by < 20)
                        fb_fill_rect(fb, stride, area_x + 4 + bx * 12, area_y + 4 + by * 12, 11, 11, dim(tetris_colors[tetris_type_], 60));
                }
            }
        }
        
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (m & (1 << (y * 4 + x))) {
                    int bx = tetris_px_ + x, by = tetris_py_ + y;
                    if (bx >= 0 && bx < 10 && by >= 0 && by < 20)
                        fb_fill_rect(fb, stride, area_x + 4 + bx * 12, area_y + 4 + by * 12, 11, 11, tetris_colors[tetris_type_]);
                }
            }
        }
    } else {
        fb_draw_text(fb, stride, area_x + 40, area_y + 100, "GAME OVER", 0xE74C3C, 0x0F0F23);
    }
}
