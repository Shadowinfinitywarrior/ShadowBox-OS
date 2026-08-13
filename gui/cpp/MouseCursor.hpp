#pragma once

#include "Widget.hpp"
#include "InputRouter.hpp"
#include "cursor.hpp"

// MouseCursor widget – displays the standard arrow cursor at the current mouse position.
// It is automatically added as a root widget by InputRouter.
class MouseCursor : public Widget {
public:
    explicit MouseCursor(InputRouter* router);
    ~MouseCursor() override = default;

    // Paint the cursor bitmap within the dirty region.
    void paint_self(const Rect& dirty, void* fb, uint32_t stride) override;

private:
    InputRouter* router_ = nullptr;
};
