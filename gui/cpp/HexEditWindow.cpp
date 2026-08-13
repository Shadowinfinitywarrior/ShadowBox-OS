#include "HexEditWindow.hpp"
#include "c_std.h"

extern "C" {
    #include "../../userland/sys.h"
    void fb_fill_rect(void* fb, uint32_t stride, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void fb_draw_text(void* fb, uint32_t stride, int32_t x, int32_t y, const char* s, uint32_t fg, uint32_t bg);
}

HexEditWindow::HexEditWindow(Widget* parent) 
    : Window(parent), file_size_(0), cursor_(0), scroll_row_(0) {
    set_title("Hex Editor");
    set_size(640, 480);
    data_ = (uint8_t*)malloc(MAX_FILE_SIZE);
    for (int i = 0; i < MAX_FILE_SIZE; i++) data_[i] = 0;
}

HexEditWindow::~HexEditWindow() {
    if (data_) {
        free(data_);
    }
}

bool HexEditWindow::load_file(const char* path) {
    int fd = sb_acquire(path, 0);
    if (fd < 0) return false;
    int n = sb_pull(fd, data_, MAX_FILE_SIZE);
    sb_release(fd);
    if (n < 0) n = 0;
    file_size_ = n;
    cursor_ = 0;
    scroll_row_ = 0;
    mark_dirty();
    return true;
}

bool HexEditWindow::on_key_press(const InputEvent& ev) {
    if (ev.key == 'w' || ev.key == 0x01) { // Up
        if (cursor_ >= BYTES_PER_ROW) {
            cursor_ -= BYTES_PER_ROW;
            mark_dirty();
        }
        return true;
    } else if (ev.key == 's' || ev.key == 0x02) { // Down
        if (cursor_ + BYTES_PER_ROW < file_size_) {
            cursor_ += BYTES_PER_ROW;
            mark_dirty();
        }
        return true;
    } else if (ev.key == 'a' || ev.key == 0x03) { // Left
        if (cursor_ > 0) {
            cursor_--;
            mark_dirty();
        }
        return true;
    } else if (ev.key == 'd' || ev.key == 0x04) { // Right
        if (cursor_ + 1 < file_size_) {
            cursor_++;
            mark_dirty();
        }
        return true;
    }
    return Window::on_key_press(ev);
}

static const char hex_chars[] = "0123456789ABCDEF";
static void to_hex2(uint8_t v, char* out) {
    out[0] = hex_chars[(v >> 4) & 0xF];
    out[1] = hex_chars[v & 0xF];
}
static void to_hex6(size_t v, char* out) {
    for (int i = 5; i >= 0; i--) {
        out[i] = hex_chars[v & 0xF];
        v >>= 4;
    }
}

void HexEditWindow::paint_self(const Rect& dirty, void* fb, uint32_t stride) {
    Window::paint_self(dirty, fb, stride);
    
    Rect abs_rect = screen_rect();
    // Use the inner area instead of full screen rect to avoid drawing over borders
    int area_x = abs_rect.x + 2;
    int area_y = abs_rect.y + 26; // Account for title bar
    int area_w = abs_rect.w - 4;
    int area_h = abs_rect.h - 28;
    
    // Fill background
    fb_fill_rect(fb, stride, area_x, area_y, area_w, area_h, 0xFFFFFFFF);
    
    // Calculate scroll if cursor is out of view
    int visible_rows = area_h / 16 - 2; // -1 for header
    if (visible_rows < 1) visible_rows = 1;
    
    int cursor_row = cursor_ / BYTES_PER_ROW;
    if (cursor_row < (int)scroll_row_) scroll_row_ = cursor_row;
    if (cursor_row >= (int)scroll_row_ + visible_rows) scroll_row_ = cursor_row - visible_rows + 1;
    
    int y = area_y + 4;
    int x = area_x + 4;
    
    // Header
    fb_draw_text(fb, stride, x, y, "Offset   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ASCII", 0xFF000000, 0xFFFFFFFF);
    y += 16;
    
    for (int r = 0; r < visible_rows; r++) {
        int row = scroll_row_ + r;
        size_t offset = row * BYTES_PER_ROW;
        if (offset >= file_size_ && offset > 0) break;
        
        char line[128];
        for (int i = 0; i < 128; i++) line[i] = ' ';
        line[127] = 0;
        
        to_hex6(offset, line);
        
        for (int c = 0; c < BYTES_PER_ROW; c++) {
            size_t idx = offset + c;
            int hx = 9 + c * 3;
            int ax = 9 + 48 + 2 + c;
            
            if (idx < file_size_) {
                uint8_t v = data_[idx];
                to_hex2(v, &line[hx]);
                char ch = (v >= 32 && v < 127) ? v : '.';
                line[ax] = ch;
            }
        }
        
        line[9 + 48 + 2 + BYTES_PER_ROW] = 0; // Null terminate
        
        fb_draw_text(fb, stride, x, y, line, 0xFF000000, 0xFFFFFFFF);
        
        // Highlight cursor
        if (cursor_ >= offset && cursor_ < offset + BYTES_PER_ROW) {
            int c = cursor_ - offset;
            int hx = 9 + c * 3;
            char cb[3] = { line[hx], line[hx+1], 0 };
            fb_draw_text(fb, stride, x + hx * 8, y, cb, 0xFFFFFFFF, 0xFF0000FF);
        }
        
        y += 16;
    }
}
