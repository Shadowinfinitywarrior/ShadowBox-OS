// InputRouter.hpp  —  Translates raw OS input → typed InputEvents → Compositor
#pragma once
#include "Widget.hpp"
#include "Compositor.hpp"
class MouseCursor;

// ═══════════════════════════════════════════════════════════════════════════
//  InputRouter
//
//  Reads raw mouse packets (3-byte PS/2 format) and keyboard scancodes,
//  converts them to InputEvent structs, and dispatches them through the
//  compositor's widget tree via hit-testing.
//
//  One InputRouter is typically created per Compositor.
// ═══════════════════════════════════════════════════════════════════════════
class InputRouter {
public:
    // Screen bounds for clamping mouse position
    InputRouter(Compositor* comp, int32_t screen_w, int32_t screen_h);
    ~InputRouter();

    // ── Raw event injection (called from the desktop event loop) ──────────

    // 3-byte PS/2 mouse packet: buttons, dx, dy
    void inject_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy);

    // Absolute mouse position (e.g. from a tablet or QEMU USB mouse)
    void inject_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons);

    // Keyboard: codepoint (Unicode for printable, KEY_* constants for special)
    void inject_key_press  (uint32_t key, uint8_t mods);
    void inject_key_release(uint32_t key, uint8_t mods);

    // Mouse wheel: delta positive = scroll down
    void inject_scroll(int32_t delta);

    // ── Cursor position ───────────────────────────────────────────────────
    int32_t mouse_x() const { return mx_; }
    int32_t mouse_y() const { return my_; }

    // ── PS/2 scancode → Unicode codepoint table ───────────────────────────
    // Returns 0 if not a printable key, KEY_* constant if special.
    static uint32_t scancode_to_key(uint8_t scancode, bool shift);

    // Special key codepoints (matches TextBox expectations)
    static constexpr uint32_t KEY_LEFT      = 0x10000001u;
    static constexpr uint32_t KEY_RIGHT     = 0x10000002u;
    static constexpr uint32_t KEY_UP        = 0x10000003u;
    static constexpr uint32_t KEY_DOWN      = 0x10000004u;
    static constexpr uint32_t KEY_HOME      = 0x10000010u;
    static constexpr uint32_t KEY_END       = 0x10000011u;
    static constexpr uint32_t KEY_PGUP      = 0x10000012u;
    static constexpr uint32_t KEY_PGDN      = 0x10000013u;
    static constexpr uint32_t KEY_F1        = 0x10000101u;
    static constexpr uint32_t KEY_ESC       = 0x1Bu;
    static constexpr uint32_t KEY_TAB       = 0x09u;
    static constexpr uint32_t KEY_BACKSPACE = 0x08u;
    static constexpr uint32_t KEY_DELETE    = 0x7Fu;
    static constexpr uint32_t KEY_RETURN    = 0x0Du;

private:
    Compositor* comp_     = nullptr;
    int32_t     screen_w_ = 0;
    int32_t     screen_h_ = 0;
    int32_t     mx_       = 0;
    int32_t     my_       = 0;
    uint8_t     prev_btn_ = 0;   // previous button state (for press/release detection)
    MouseCursor* cursor_ = nullptr;

    void clamp_mouse();
    void dispatch(const InputEvent& ev);
    Widget* hit_test_all(Point pt) const;
};

// ─── C-callable bridge ────────────────────────────────────────────────────
extern "C" {
    InputRouter* input_router_create(Compositor* c,
                                     int32_t screen_w, int32_t screen_h);
    void input_router_destroy        (InputRouter* r);
    void input_router_mouse_packet   (InputRouter* r,
                                      uint8_t buttons, int8_t dx, int8_t dy);
    void input_router_mouse_absolute (InputRouter* r,
                                      int32_t x, int32_t y, uint8_t buttons);
    void input_router_key_press      (InputRouter* r, uint32_t key, uint8_t mods);
    void input_router_key_release    (InputRouter* r, uint32_t key, uint8_t mods);
    void input_router_scroll         (InputRouter* r, int32_t delta);
    int32_t input_router_mouse_x     (InputRouter* r);
    int32_t input_router_mouse_y     (InputRouter* r);
    uint32_t input_router_scancode   (uint8_t sc, int shift);
}
