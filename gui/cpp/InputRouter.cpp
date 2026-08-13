// InputRouter.cpp  —  Input translation and dispatch
#include "InputRouter.hpp"
#include "MouseCursor.hpp"
#include "cursor.hpp"
#include <cstdlib>

// Freestanding placement new (no <new> header available)
inline void* operator new(decltype(sizeof(0)), void* p) noexcept { return p; }



// ─── PS/2 US-QWERTY scancode tables ───────────────────────────────────────
// Index = scancode (set 1); value = ASCII codepoint (0 = unmapped).
// Only the first 89 scancodes are covered; extended codes go via special-key path.

static const uint8_t sc_normal[89] = {
/*00*/  0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', 8,
/*0F*/  9,  'q','w','e','r','t','y','u','i','o','p','[',']', 13,
/*1D*/  0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
/*2A*/  0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
/*37*/  '*', 0,  ' ', 0,
/*3B*/  0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  /* F1-F10 */
/*45*/  0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*52*/  0,   0,   0
};

static const uint8_t sc_shift[89] = {
/*00*/  0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', 8,
/*0F*/  9,  'Q','W','E','R','T','Y','U','I','O','P','{','}', 13,
/*1D*/  0,  'A','S','D','F','G','H','J','K','L',':','"','~',
/*2A*/  0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
/*37*/  '*', 0,  ' ', 0,
/*3B*/  0,   0,   0,  0,  0,  0,  0,  0,  0,  0,
/*45*/  0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*52*/  0,   0,   0
};

// ─────────────────────────────────────────────────────────────────────────

uint32_t InputRouter::scancode_to_key(uint8_t sc, bool shift) {
    // Extended / special scancodes handled here
    switch (sc) {
        case 0x48: return KEY_UP;
        case 0x50: return KEY_DOWN;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        case 0x47: return KEY_HOME;
        case 0x4F: return KEY_END;
        case 0x49: return KEY_PGUP;
        case 0x51: return KEY_PGDN;
        case 0x53: return KEY_DELETE;
        case 0x3B: return KEY_F1;
        case 0x01: return KEY_ESC;
        default:   break;
    }

    if (sc >= 89) return 0;
    uint8_t ch = shift ? sc_shift[sc] : sc_normal[sc];
    return (uint32_t)ch;
}

// ─────────────────────────────────────────────────────────────────────────

InputRouter::InputRouter(Compositor* comp, int32_t sw, int32_t sh)
    : comp_(comp), screen_w_(sw), screen_h_(sh)
{
    mx_ = sw / 2;
    my_ = sh / 2;
    // Allocate and add the cursor widget as a top‑level root.
    if (comp_) {
        void* memc = ::malloc(sizeof(MouseCursor));
        if (memc) {
            cursor_ = new(memc) MouseCursor(this);
            comp_->add_root(cursor_, true);
        }
    }
}

void InputRouter::clamp_mouse() {
    if (mx_ < 0) mx_ = 0;
    if (my_ < 0) my_ = 0;
    if (mx_ >= screen_w_) mx_ = screen_w_ - 1;
    if (my_ >= screen_h_) my_ = screen_h_ - 1;
}

// ─── Hit-test across all compositor roots (topmost first) ─────────────────

Widget* InputRouter::hit_test_all(Point pt) const {
    if (!comp_) return nullptr;
    int n = 0;
    // Count roots (we need to iterate in reverse for topmost-first)
    // Compositor makes roots_ private, so we use the focused() accessor
    // and rely on the compositor's frame() to order them. For hit-testing
    // we expose nothing from Compositor — so we store a shadow root list here.
    // Instead, traverse roots via the compositor bridge (add_root adds to comp).
    // Simple approach: compositor exposes hit-test via the C bridge.
    (void)n; (void)pt;
    return nullptr;   // Overridden in dispatch() with comp's own tree walk
}

// Destructor – clean up the cursor widget.
InputRouter::~InputRouter() {
    if (cursor_) {
        cursor_->~MouseCursor();
        ::free(cursor_);
        cursor_ = nullptr;
    }
}

// ─── Dispatch helpers ─────────────────────────────────────────────────────

void InputRouter::dispatch(const InputEvent& ev) {
    if (!comp_) return;

    // For mouse events, find the widget under the cursor via compositor's roots
    // (Compositor gives us access only through the C bridge; we work around this
    //  by using the compositor's focused() for keyboard and routing mouse events
    //  by broadcasting to all roots.)

    // Keyboard events go to the focused widget
    if (ev.type == EventType::KeyPress || ev.type == EventType::KeyRelease) {
        Widget* fw = comp_->focused();
        if (fw) fw->on_event(ev);
        return;
    }

    // Scroll goes to the focused widget first, then falls through to hit-test
    if (ev.type == EventType::MouseScroll) {
        Widget* fw = comp_->focused();
        if (fw && fw->on_event(ev)) return;
    }

    // Mouse events: walk the root list (internal — we cast past the class
    // boundary via the stored pointer; roots_ field offset is an impl detail).
    // To keep this self-contained we broadcast to all roots and let each one
    // do its own hit-test / containment check.
    // (In a real WM the compositor would expose a hit-test API.)
    struct CompLayout { void* fb; uint32_t stride, w, h; Widget* roots[32]; int rc; };
    CompLayout* cl = reinterpret_cast<CompLayout*>(comp_);
    for (int i = cl->rc - 1; i >= 0; --i) {
        if (cl->roots[i] && cl->roots[i]->visible()) {
            Widget* hit = cl->roots[i]->hit_test(ev.pos);
            if (hit) {
                // Update focus on press
                if (ev.type == EventType::MousePress) {
                    if ((hit->flags() & WF_FOCUSABLE) && hit != comp_->focused())
                        comp_->set_focus(hit);
                }
                if (hit->on_event(ev)) return;
                // Also offer to the root (e.g. Window drag)
                cl->roots[i]->on_event(ev);
                return;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────

void InputRouter::inject_mouse_packet(uint8_t buttons, int8_t dx, int8_t dy) {
    int32_t old_mx = mx_;
    int32_t old_my = my_;
    mx_ += (int32_t)dx;
    my_ -= (int32_t)dy;   // PS/2 Y axis is inverted
    clamp_mouse();
    
    if (cursor_) {
        // Dirty old cursor area
        cursor_->mark_dirty({ old_mx, old_my, CursorBitmap::W, CursorBitmap::H });
        // Update widget position and mark new area dirty
        cursor_->set_rect({ mx_, my_, CursorBitmap::W, CursorBitmap::H });
        cursor_->mark_dirty();
    }

    InputEvent ev;
    ev.pos = { mx_, my_ };

    // Build move event
    ev.type = EventType::MouseMove;
    dispatch(ev);

    // Detect left button press/release
    bool left_now  = (buttons & 0x01) != 0;
    bool left_prev = (prev_btn_ & 0x01) != 0;
    if (left_now && !left_prev) {
        ev.type   = EventType::MousePress;
        ev.button = MouseButton::Left;
        dispatch(ev);
    } else if (!left_now && left_prev) {
        ev.type   = EventType::MouseRelease;
        ev.button = MouseButton::Left;
        dispatch(ev);
    }

    // Right button
    bool right_now  = (buttons & 0x02) != 0;
    bool right_prev = (prev_btn_ & 0x02) != 0;
    if (right_now && !right_prev) {
        ev.type   = EventType::MousePress;
        ev.button = MouseButton::Right;
        dispatch(ev);
    } else if (!right_now && right_prev) {
        ev.type   = EventType::MouseRelease;
        ev.button = MouseButton::Right;
        dispatch(ev);
    }

    prev_btn_ = buttons;
}

void InputRouter::inject_mouse_absolute(int32_t ax, int32_t ay, uint8_t buttons) {
    int32_t old_mx = mx_;
    int32_t old_my = my_;
    mx_ = ax; my_ = ay;
    clamp_mouse();

    if (cursor_) {
        cursor_->mark_dirty({ old_mx, old_my, CursorBitmap::W, CursorBitmap::H });
        cursor_->set_rect({ mx_, my_, CursorBitmap::W, CursorBitmap::H });
        cursor_->mark_dirty();
    }

    // Build move event
    InputEvent ev;
    ev.type = EventType::MouseMove;
    ev.pos = { mx_, my_ };
    dispatch(ev);

    // Detect left button press/release
    bool left_now  = (buttons & 0x01) != 0;
    bool left_prev = (prev_btn_ & 0x01) != 0;
    if (left_now && !left_prev) {
        ev.type   = EventType::MousePress;
        ev.button = MouseButton::Left;
        dispatch(ev);
    } else if (!left_now && left_prev) {
        ev.type   = EventType::MouseRelease;
        ev.button = MouseButton::Left;
        dispatch(ev);
    }

    // Right button
    bool right_now  = (buttons & 0x02) != 0;
    bool right_prev = (prev_btn_ & 0x02) != 0;
    if (right_now && !right_prev) {
        ev.type   = EventType::MousePress;
        ev.button = MouseButton::Right;
        dispatch(ev);
    } else if (!right_now && right_prev) {
        ev.type   = EventType::MouseRelease;
        ev.button = MouseButton::Right;
        dispatch(ev);
    }

    prev_btn_ = buttons;
}

void InputRouter::inject_key_press(uint32_t key, uint8_t mods) {
    InputEvent ev;
    ev.type = EventType::KeyPress;
    ev.key  = key;
    ev.mods = mods;
    dispatch(ev);
}

void InputRouter::inject_key_release(uint32_t key, uint8_t mods) {
    InputEvent ev;
    ev.type = EventType::KeyRelease;
    ev.key  = key;
    ev.mods = mods;
    dispatch(ev);
}

void InputRouter::inject_scroll(int32_t delta) {
    InputEvent ev;
    ev.type         = EventType::MouseScroll;
    ev.scroll_delta = delta;
    ev.pos          = { mx_, my_ };
    dispatch(ev);
}

// ─── C bridge ─────────────────────────────────────────────────────────────

extern "C" {

InputRouter* input_router_create(Compositor* c, int32_t sw, int32_t sh) {
    void* mem = ::malloc(sizeof(InputRouter));
    if (!mem) return nullptr;
    return new(mem) InputRouter(c, sw, sh);
}

void input_router_destroy(InputRouter* r) {
    if (r) { r->~InputRouter(); ::free(r); }
}

void input_router_mouse_packet(InputRouter* r,
                                uint8_t buttons, int8_t dx, int8_t dy) {
    if (r) r->inject_mouse_packet(buttons, dx, dy);
}

void input_router_mouse_absolute(InputRouter* r,
                                  int32_t x, int32_t y, uint8_t buttons) {
    if (r) r->inject_mouse_absolute(x, y, buttons);
}

void input_router_key_press(InputRouter* r, uint32_t key, uint8_t mods) {
    if (r) r->inject_key_press(key, mods);
}

void input_router_key_release(InputRouter* r, uint32_t key, uint8_t mods) {
    if (r) r->inject_key_release(key, mods);
}

void input_router_scroll(InputRouter* r, int32_t delta) {
    if (r) r->inject_scroll(delta);
}

int32_t input_router_mouse_x(InputRouter* r) {
    return r ? r->mouse_x() : 0;
}

int32_t input_router_mouse_y(InputRouter* r) {
    return r ? r->mouse_y() : 0;
}

uint32_t input_router_scancode(uint8_t sc, int shift) {
    return InputRouter::scancode_to_key(sc, shift != 0);
}

} // extern "C"
