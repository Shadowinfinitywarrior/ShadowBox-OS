// ContextMenu.hpp — Simple popup context menu widget
#pragma once

#include "Widget.hpp"

// Simple context menu used for right‑click dropdowns.
// It displays a vertically stacked list of items (label + callback).
// The menu does not manage its own visibility – callers can hide it
// by calling set_visible(false) after selecting an item.

class ContextMenu : public Widget {
public:
    // Maximum number of menu items (adjust as needed).
    static constexpr int MAX_ITEMS = 16;
    static constexpr int ITEM_HEIGHT = 24; // pixels per item

    explicit ContextMenu(Widget* parent = nullptr);
    ~ContextMenu() override = default;

    // Add an item. "label" is copied (max 127 chars). "cb" will be called with
    // the user‑provided data when the item is activated.
    void add_item(const char* label, void (*cb)(void*), void* user_data);

protected:
    void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;
    bool on_mouse_move(const InputEvent& ev) override;
    bool on_mouse_press(const InputEvent& ev) override;

private:
    struct Item {
        char label[128] = {};
        void (*callback)(void*) = nullptr;
        void* data = nullptr;
    } items_[MAX_ITEMS];
    int item_count_ = 0;
    int hovered_index_ = -1; // -1 = none
};
