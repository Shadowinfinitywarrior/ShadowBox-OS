#pragma once
#include "Window.hpp"

class HexEditWindow : public Window {
public:
    HexEditWindow(Widget* parent = nullptr);
    ~HexEditWindow();
    
    virtual void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;
    virtual bool on_key_press(const InputEvent& ev) override;

    bool load_file(const char* path);

private:
    static const int MAX_FILE_SIZE = 65536;
    static const int BYTES_PER_ROW = 16;
    
    uint8_t* data_;
    size_t file_size_;
    size_t cursor_;
    size_t scroll_row_;
};
