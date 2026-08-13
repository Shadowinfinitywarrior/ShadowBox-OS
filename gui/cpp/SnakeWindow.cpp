#include "SnakeWindow.hpp"

extern "C" {
    void fb_fill_rect(void* fb, uint32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fb_draw_text(void* fb, uint32_t stride, int32_t x, int32_t y, const char* s, uint32_t fg, uint32_t bg);
}

// Pseudo-random for food
static uint64_t snake_rng_state = 0x123456789ULL;
static uint32_t rng_next() {
    snake_rng_state = snake_rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return snake_rng_state >> 32;
}

SnakeWindow::SnakeWindow(Widget* parent) : Window(parent), tick_acc_(0) {
    set_title("Snake");
    set_size(240, 260); // Window contents start around y+24 normally, let's leave space
    reset();
}

void SnakeWindow::reset() {
    snake_len_ = 3;
    snake_x_[0] = 10; snake_y_[0] = 10;
    snake_x_[1] = 9;  snake_y_[1] = 10;
    snake_x_[2] = 8;  snake_y_[2] = 10;
    snake_dir_ = 1;
    snake_dead_ = false;
    spawn_food();
    mark_dirty();
}

void SnakeWindow::spawn_food() {
    int max_x = (rect_.w - 4) / GRID_SIZE;
    int max_y = (rect_.h - 28) / GRID_SIZE;
    if (max_x <= 0) max_x = 1;
    if (max_y <= 0) max_y = 1;
    
    while (true) {
        food_x_ = rng_next() % max_x;
        food_y_ = rng_next() % max_y;
        bool ok = true;
        for (int i = 0; i < snake_len_; ++i) {
            if (snake_x_[i] == food_x_ && snake_y_[i] == food_y_) {
                ok = false;
                break;
            }
        }
        if (ok) break;
    }
}

void SnakeWindow::tick(int dt_ms) {
    Window::tick(dt_ms);
    if (snake_dead_) return;
    
    tick_acc_ += dt_ms;
    if (tick_acc_ < 100) return; // update every 100ms
    tick_acc_ -= 100;
    
    int max_x = (rect_.w - 4) / GRID_SIZE;
    int max_y = (rect_.h - 28) / GRID_SIZE;
    
    // Move body
    for (int j = snake_len_ - 1; j > 0; j--) {
        snake_x_[j] = snake_x_[j-1];
        snake_y_[j] = snake_y_[j-1];
    }
    
    // Move head
    if (snake_dir_ == 0) snake_y_[0]--;
    else if (snake_dir_ == 1) snake_x_[0]++;
    else if (snake_dir_ == 2) snake_y_[0]++;
    else if (snake_dir_ == 3) snake_x_[0]--;
    
    // Wall collision
    if (snake_x_[0] < 0 || snake_x_[0] >= max_x ||
        snake_y_[0] < 0 || snake_y_[0] >= max_y) {
        snake_dead_ = true;
        mark_dirty();
        return;
    }
    
    // Self collision
    for (int j = 1; j < snake_len_; j++) {
        if (snake_x_[0] == snake_x_[j] && snake_y_[0] == snake_y_[j]) {
            snake_dead_ = true;
            mark_dirty();
            return;
        }
    }
    
    // Food collision
    if (snake_x_[0] == food_x_ && snake_y_[0] == food_y_) {
        if (snake_len_ < MAX_SNAKE) snake_len_++;
        spawn_food();
    }
    
    mark_dirty();
}

bool SnakeWindow::on_key_press(const InputEvent& ev) {
    if (ev.key == 'w' || ev.key == 0x01) { // 0x01 is up arrow in our simple mapping
        if (snake_dir_ != 2) snake_dir_ = 0;
        return true;
    }
    if (ev.key == 'd' || ev.key == 0x04) { // right
        if (snake_dir_ != 3) snake_dir_ = 1;
        return true;
    }
    if (ev.key == 's' || ev.key == 0x02) { // down
        if (snake_dir_ != 0) snake_dir_ = 2;
        return true;
    }
    if (ev.key == 'a' || ev.key == 0x03) { // left
        if (snake_dir_ != 1) snake_dir_ = 3;
        return true;
    }
    if (ev.key == 'r') {
        if (snake_dead_) reset();
        return true;
    }
    return Window::on_key_press(ev);
}

void SnakeWindow::paint_self(const Rect& dirty, void* fb, uint32_t stride) {
    Window::paint_self(dirty, fb, stride);
    
    Rect abs_rect = screen_rect();
    
    // Game area
    int area_x = abs_rect.x + 2;
    int area_y = abs_rect.y + 26;
    int area_w = abs_rect.w - 4;
    int area_h = abs_rect.h - 28;
    
    fb_fill_rect(fb, stride, area_x, area_y, area_w, area_h, 0x1E1E1E);
    
    // Food
    fb_fill_rect(fb, stride, area_x + food_x_ * GRID_SIZE, area_y + food_y_ * GRID_SIZE, GRID_SIZE - 1, GRID_SIZE - 1, 0xE74C3C);
    
    // Snake
    uint32_t color = snake_dead_ ? 0x95A5A6 : 0x2ECC71;
    for (int i = 0; i < snake_len_; i++) {
        fb_fill_rect(fb, stride, area_x + snake_x_[i] * GRID_SIZE, area_y + snake_y_[i] * GRID_SIZE, GRID_SIZE - 1, GRID_SIZE - 1, color);
    }
    
    if (snake_dead_) {
        fb_draw_text(fb, stride, area_x + area_w / 2 - 40, area_y + area_h / 2, "GAME OVER", 0xE74C3C, 0x1E1E1E);
        fb_draw_text(fb, stride, area_x + area_w / 2 - 45, area_y + area_h / 2 + 16, "Press R to Restart", 0xECF0F1, 0x1E1E1E);
    }
}
