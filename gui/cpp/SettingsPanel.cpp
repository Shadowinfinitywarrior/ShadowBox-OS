// SettingsPanel.cpp — Implementation of the simple SettingsPanel widget
#include "SettingsPanel.hpp"
#include <cstdio>

SettingsPanel::SettingsPanel(Widget* parent)
    : Window(parent)
{
    // Configure the window
    set_title("Settings");
    set_pos(200, 150);
    set_size(300, 250);

    // Create a button to toggle dark mode
    toggle_dark_btn_ = new Button(this);
    toggle_dark_btn_->set_label("Toggle Dark");
    toggle_dark_btn_->set_pos(10, 50);
    toggle_dark_btn_->set_size(120, 30);
    // Store pointer to this panel for the callback
    toggle_dark_btn_->user_data = this;
    toggle_dark_btn_->on_clicked = SettingsPanel::on_toggle_dark;
}

SettingsPanel::~SettingsPanel() {
    // Child widgets are owned by Window and will be deleted automatically.
}

void SettingsPanel::on_toggle_dark(Widget* w) {
    // The button passes itself as the argument; retrieve the panel via user_data.
    SettingsPanel* self = static_cast<SettingsPanel*>(w->user_data);
    if (!self) return;
    self->dark_mode_ = !self->dark_mode_;
    // For demonstration, update the window title to reflect the mode.
    const char* mode = self->dark_mode_ ? "Settings (Dark)" : "Settings (Light)";
    self->set_title(mode);
    // Optionally, you could request a repaint or other UI updates here.
    // For now we simply mark the window dirty to repaint.
    self->mark_dirty();
}
