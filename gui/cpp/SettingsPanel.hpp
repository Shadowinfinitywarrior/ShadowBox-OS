// SettingsPanel.hpp — Simple settings panel UI component
#pragma once

#include "Window.hpp"
#include "Button.hpp"
#include "Label.hpp"

// Simple settings panel showing a toggle for dark mode.
// In a real OS this would interface with the settings daemon;
// for now it only toggles an internal flag and updates the title.

class SettingsPanel : public Window {
public:
    explicit SettingsPanel(Widget* parent = nullptr);
    ~SettingsPanel() override;

private:
    Button* toggle_dark_btn_ = nullptr;
    bool dark_mode_ = false;

    // Callback for the toggle button – static member matches VoidFn signature.
    static void on_toggle_dark(Widget* w);
};
