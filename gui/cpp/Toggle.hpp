// Toggle.hpp  —  Checkbox/Toggle widget with modern styling
#pragma once

#include "Widget.hpp"
#include "Colors.hpp"

// ── Toggle states ──────────────────────────────────────────────────────────
enum ToggleState {
    TOGGLE_OFF = 0,
    TOGGLE_ON  = 1,
};

// ── Callback type ──────────────────────────────────────────────────────────
using ToggleFn = void(*)(Widget* self, ToggleState state);

// ── Toggle widget ──────────────────────────────────────────────────────────
class Toggle : public Widget {
public:
    explicit Toggle(Widget* parent = nullptr);
    virtual ~Toggle();

    // Non-copyable
    Toggle(const Toggle&)            = delete;
    Toggle& operator=(const Toggle&) = delete;

    // ── Appearance ──────────────────────────────────────────────────────────
    void set_state(ToggleState s);
    ToggleState state() const { return state_; }

    // ── Callbacks ───────────────────────────────────────────────────────────
    void set_on_toggled(ToggleFn fn) { on_toggled_ = fn; }

    // ── Painting ───────────────────────────────────────────────────────────
    void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;

    // ── Input ──────────────────────────────────────────────────────────────
    bool on_mouse_press(const InputEvent& ev) override;
    bool on_mouse_move(const InputEvent& ev) override;
    bool on_mouse_release(const InputEvent& ev) override;

    // ── Debug tag ──────────────────────────────────────────────────────────
    char tag[32] = {};
    void set_tag(const char* t) {
        int i = 0;
        while (t[i] && i < 31) { tag[i] = t[i]; ++i; }
        tag[i] = '\0';
    }

private:
    ToggleState state_    = TOGGLE_OFF;
    ToggleFn  on_toggled_ = nullptr;

    // ── Visual constants ──────────────────────────────────────────────────
    static constexpr int TOGGLE_SIZE = 20;    // Checkbox size in pixels
    static constexpr int INDICATOR_R = 3;     // Inner indicator radius
};