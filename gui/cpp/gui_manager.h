#pragma once

#include <memory>
#include <stdint.h>

struct RendererInfo; // forward

class GUIManager {
public:
    GUIManager();
    ~GUIManager();

    // Initialize the GUI system with framebuffer
    void init(void* fb, uint32_t stride, uint32_t w, uint32_t h);

    // Shut down
    void shutdown();

    // Frame advance
    void frame(int dt_ms = 16);

    // Window management
    void add_root(Widget* w, bool raise = true);
    void remove_root(Widget* w);
    Widget* focused() const;
    void set_focus(Widget* w);

    // Input delegation (pass-through to InputRouter)
    void inject_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy);
    void inject_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons);
    void inject_key_press(uint32_t key, uint8_t mods);
    void inject_key_release(uint32_t key, uint8_t mods);
    void inject_scroll(int32_t delta);

    // Mouse position
    int32_t mouse_x() const;
    int32_t mouse_y() const;

    // Renderer control
    void set_renderer(std::unique_ptr<RendererInfo> r);
    std::unique_ptr<RendererInfo> release_renderer();

private:
    std::unique_ptr<Compositor> comp_;
    std::unique_ptr<InputRouter> router_;
    std::unique_ptr<MouseCursor> cursor_;
    std::unique_ptr<RendererInfo> renderer_;
    int composite_count_;
};
