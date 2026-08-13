// KernelConfigView.hpp — UI component to display kernel configuration values
#pragma once

#include "Window.hpp"
#include "ScrollView.hpp"
#include "Label.hpp"

// Component that shows compile‑time kernel configuration constants in a scrollable view.
class KernelConfigView : public Window {
public:
    explicit KernelConfigView(Widget* parent = nullptr);
    ~KernelConfigView() override = default;

private:
    ScrollView* scroll_view_ = nullptr;   // Holds the scrollable content area
    Widget* content_ = nullptr;            // Container widget for all labels
};
