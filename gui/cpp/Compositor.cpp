// Compositor.cpp — Incremental repaint engine
#include "Compositor.hpp"
#include <cstdlib>

// Freestanding placement new (no <new> header available)
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }

extern "C" {
void fb_fill_rect(void* fb, uint32_t stride,
int32_t x, int32_t y, int32_t w, int32_t h,
uint32_t color);
void fb_blit_rect(void* dst, uint32_t dst_stride,
const void* src, uint32_t src_stride,
int32_t dx, int32_t dy, int32_t w, int32_t h);
void gui_fb_flip(uint32_t y_offset);
}

// ─────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────

Compositor::Compositor(void* fb, uint32_t stride,
uint32_t width, uint32_t height)
: fb_(fb), stride_(stride), width_(width), height_(height)
{
full_repaint_ = true;
}

// ─── Root management ──────────────────────────────────────────────────────

void Compositor::add_root(Widget* w, bool raise) {
if (!w || root_count_ >= MAX_ROOTS) return;
for (int i = 0; i < root_count_; ++i)
if (roots_[i] == w) return;
roots_[root_count_++] = w;
if (raise) {
if (focused_) {
InputEvent ev;
ev.type = EventType::FocusLost;
focused_->on_event(ev);
}
set_focus(w);
}
invalidate_all();
}

void Compositor::remove_root(Widget* w) {
for (int i = 0; i < root_count_; ++i) {
if (roots_[i] == w) {
for (int j = i; j < root_count_ - 1; ++j)
roots_[j] = roots_[j + 1];
--root_count_;
invalidate_all();
return;
}
}
}

// ─── Focus ────────────────────────────────────────────────────────────────

void Compositor::set_focus(Widget* w) {
if (focused_ == w) return;

if (focused_) {
InputEvent ev;
ev.type = EventType::FocusLost;
focused_->on_event(ev);
}
focused_ = w;
if (focused_) {
InputEvent ev;
ev.type = EventType::FocusGained;
focused_->set_flag(WF_FOCUSED, true);
focused_->on_event(ev);
}
}

// ─── Dirty tracking ───────────────────────────────────────────────────────

void Compositor::add_dirty(const Rect& r) {
if (r.w <= 0 || r.h <= 0) return;
if (dirty_count_ < MAX_DIRTY) {
dirty_[dirty_count_++] = r;
} else {
dirty_[0] = { 0, 0, (int32_t)width_, (int32_t)height_ };
dirty_count_ = 1;
}
}

void Compositor::invalidate_all() {
full_repaint_ = true;
}

// ─── Frame loop ───────────────────────────────────────────────────────────

void Compositor::frame(int dt_ms) {
dirty_count_ = 0;

if (full_repaint_) {
full_repaint_ = false;
add_dirty({ 0, 0, (int32_t)width_, (int32_t)height_ });
} else {
collect_dirty();
}

if (dirty_count_ == 0) {
animate_roots(dt_ms);
return;
}

repaint();
blit_to_screen();

// Clear dirty lists on all roots
for (int i = 0; i < root_count_; ++i)
roots_[i]->dirty().clear();

animate_roots(dt_ms);
}

void Compositor::animate_roots(int dt_ms) {
    for (int i = 0; i < root_count_; ++i)
roots_[i]->tick(dt_ms);
}

void Compositor::collect_dirty() {
for (int i = 0; i < root_count_; ++i) {
DirtyList& dl = roots_[i]->dirty();
for (int j = 0; j < dl.count; ++j)
add_dirty(dl.rects[j]);
}
}

void Compositor::repaint() {
// Determine paint target: back-buffer if available, else direct fb
void* target = backbuf ? backbuf : fb_;
uint32_t tstride = stride_;

// Clear each dirty rect to the desktop background
for (int d = 0; d < dirty_count_; ++d) {
const Rect& r = dirty_[d];
fb_fill_rect(target, tstride,
r.x, r.y, r.w, r.h,
Colors::WindowBg);
}

// Repaint all roots (back-to-front) for each dirty region
	// Draw drop shadows for each root window (visual depth)
	const int SHADOW_OFFSET = 4;
	for (int d = 0; d < dirty_count_; ++d) {
	    const Rect& dr = dirty_[d];
	    for (int i = 0; i < root_count_; ++i) {
	        Rect sr = roots_[i]->screen_rect();
	        Rect shadow = { sr.x + SHADOW_OFFSET, sr.y + SHADOW_OFFSET, sr.w, sr.h };
	        if (shadow.intersects(dr)) {
	            Rect draw = shadow.intersection(dr);
	            fb_fill_rect(target, tstride,
	                         draw.x, draw.y, draw.w, draw.h,
	                         Colors::Shadow);
	        }
	    }
	}

for (int d = 0; d < dirty_count_; ++d) {
const Rect& dr = dirty_[d];
for (int i = 0; i < root_count_; ++i)
roots_[i]->paint(dr, target, tstride);
}
}

void Compositor::blit_to_screen() {
    if (!backbuf || !fb_) return;
    
    // We assume fb_ points to the start of VRAM (mapped to 2x height).
    // We implement kernel-level double buffering using sys_fb_flip.
    static int active_buffer = 0;
    int next_buffer = active_buffer ^ 1;
    
    uint8_t* hidden_fb = (uint8_t*)fb_ + (next_buffer ? height_ * stride_ : 0);
    
    // Copy the dirty regions from backbuf to the hidden hardware buffer
    for (int d = 0; d < dirty_count_; ++d) {
        const Rect& r = dirty_[d];
        fb_blit_rect(hidden_fb, stride_,
                     backbuf, stride_,
                     r.x, r.y, r.w, r.h);
    }
    
    // Issue page flip
    gui_fb_flip(next_buffer ? height_ : 0);
    
    // Now that we flipped, the old visible buffer is hidden.
    // We must ALSO apply the same dirty rectangles to the newly hidden buffer
    // so it stays up-to-date for the NEXT frame.
    uint8_t* new_hidden_fb = (uint8_t*)fb_ + (active_buffer ? height_ * stride_ : 0);
    for (int d = 0; d < dirty_count_; ++d) {
        const Rect& r = dirty_[d];
        fb_blit_rect(new_hidden_fb, stride_,
                     backbuf, stride_,
                     r.x, r.y, r.w, r.h);
    }
    
    active_buffer = next_buffer;
}

// ─── C bridge ─────────────────────────────────────────────────────────────

extern "C" {

Compositor* compositor_create(void* fb, uint32_t stride,
uint32_t w, uint32_t h)
{
void* mem = ::malloc(sizeof(Compositor));
if (!mem) return nullptr;
return new(mem) Compositor(fb, stride, w, h);
}

void compositor_destroy(Compositor* c) {
if (c) { c->~Compositor(); ::free(c); }
}

void compositor_add_root(Compositor* c, Widget* w, int raise) {
if (c) c->add_root(w, raise != 0);
}

void compositor_remove_root(Compositor* c, Widget* w) {
if (c) c->remove_root(w);
}

void compositor_frame(Compositor* c) {
if (c) c->frame();
}

void compositor_invalidate(Compositor* c) {
if (c) c->invalidate_all();
}

void compositor_set_focus(Compositor* c, Widget* w) {
if (c) c->set_focus(w);
}

Widget* compositor_focused(Compositor* c) {
return c ? c->focused() : nullptr;
}

void compositor_set_backbuf(Compositor* c, void* buf) {
if (c) c->backbuf = buf;
}

} // extern "C"
