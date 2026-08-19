#include "GUIManager.hpp"
#include "Compositor.hpp"
#include "InputRouter.hpp"
#include "MouseCursor.hpp"
#include "gui_public.h"

// GUIManager — central owner of compositor, input router, and widget tree
// Reduces boilerplate in demo applications and future apps.

GUIManager::GUIManager() : renderer_(std::make_unique<SoftwareRenderer>()), composite_count_(0) {
    // Set up global GUI manager instance
    gui_manager_ptr_ = this;
}

GUIManager::~GUIManager() {
    shutdown();
}

void GUIManager::init(void* fb, uint32_t stride, uint32_t w, uint32_t h) {
    // Create compositor
    comp_ = std::make_unique<Compositor>(fb, stride, w, h);
    comp_->set_renderer(renderer_.get());
    
    // Create input router
    router_ = std::make_unique<InputRouter>(comp_.get(), w, h);
    
    // Create default mouse cursor (auto-added as root by InputRouter)
    cursor_ = std::make_unique<MouseCursor>(router_.get());
    router_->set_cursor(cursor_.get());
    
    composite_count_ = 0;
}

void GUIManager::shutdown() {
    if (cursor_) { cursor_.reset(); }
    if (router_) { router_.reset(); }
    if (comp_) { comp_.reset(); }
    renderer_.reset();
    composite_count_ = 0;
}

void GUIManager::frame(int dt_ms) {
    if (!comp_) return;
    // Advance animations on all root widgets
    animate_roots(dt_ms);
    // Frame the compositor
    comp_->frame(dt_ms);
}

void GUIManager::add_root(Widget* w, bool raise) {
    if (!comp_) return;
    if (raise) comp_->raise_root(w);
    composite_count_++;
}

void GUIManager::remove_root(Widget* w) {
    if (!comp_) return;
    comp_->remove_root(w);
    composite_count_--;
}

Widget* GUIManager::focused() const {
    return comp_ ? comp_->focused() : nullptr;
}

void GUIManager::set_focus(Widget* w) {
    if (comp_) comp_->set_focus(w);
}

// Input delegation (pass-through to InputRouter)
void GUIManager::inject_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy) {
    if (router_) router_->inject_mouse_packet(buttons, dx, dy);
}

void GUIManager::inject_mouse_absolute(int32_t abs_x, int32_t abs_y, uint8_t buttons) {
    if (router_) router_->inject_mouse_absolute(abs_x, abs_y, buttons);
}

void GUIManager::inject_key_press(uint32_t key, uint8_t mods) {
    if (router_) router_->inject_key_press(key, mods);
}

void GUIManager::inject_key_release(uint32_t key, uint8_t mods) {
    if (router_) router_->inject_key_release(key, mods);
}

void GUIManager::inject_scroll(int32_t delta) {
    if (router_) router_->inject_scroll(delta);
}

int32_t GUIManager::mouse_x() const { return router_ ? router_->mouse_x() : 0; }
int32_t GUIManager::mouse_y() const { return router_ ? router_->mouse_y() : 0; }

void GUIManager::set_renderer(std::unique_ptr<RendererInfo> r) {
    renderer_ = std::move(r);
    if (comp_) comp_->set_renderer(renderer_.get());
}
std::unique_ptr<RendererInfo> GUIManager::release_renderer() {
    return std::move(renderer_);
}
