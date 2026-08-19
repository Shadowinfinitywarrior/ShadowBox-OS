#pragma once

#include "gui_internal.h"

struct RendererInfo {
    virtual ~RendererInfo() {}
    virtual void init(void* fb, uint32_t stride, uint32_t w, uint32_t h) = 0;
    virtual void shutdown() = 0;
    virtual void paint_widget(Widget* w, const Rect& dirty, void* fb, uint32_t stride) = 0;
    virtual void present() = 0;
    virtual void resize(uint32_t w, uint32_t h) = 0;
};
