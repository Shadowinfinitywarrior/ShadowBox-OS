// Window.hpp  —  Decorated, draggable, resizable top-level widget
#pragma once
#include "Widget.hpp"
#include "Button.hpp"
#include "Label.hpp"

// ═══════════════════════════════════════════════════════════════════════════
//  Window
//
//  Children added via add_client() are offset into the client area
//  (below the title bar). Built-in close / minimise buttons and a title
//  Label are added as regular children of this widget.
// ═══════════════════════════════════════════════════════════════════════════
class Window : public Widget {
public:
    static constexpr int TITLEBAR_H  = 32;
    static constexpr int BORDER_W    = 2;
    static constexpr int RESIZE_ZONE = 8;

    explicit Window(Widget* parent = nullptr);
    ~Window() override;

    void        set_title(const char* title);
    const char* title()   const { return title_; }

    // Client area in screen coords — where user content lives
    Rect client_rect() const;
    // Add a widget into the client area (adjusts its position)
    void add_client(Widget* child);

    // Feature flags
    bool resizable   = true;
    bool closeable   = true;
    bool minimizable = true;

    // Callbacks
    VoidFn on_close    = nullptr;
    VoidFn on_minimize = nullptr;

    // Theme colours
    Color title_bg   = Colors::BarBg;
    Color title_fg   = Colors::Text;
    Color window_bg  = Colors::WindowBg;
    Color border_col = Colors::Border;

	// ── Pop / progress animation state ───────────────────────────────
	bool animating_ = false;
	int anim_elapsed_ms_ = 0;
	bool anim_open_ = true;
	Rect anim_target_rect_ = {};
	float anim_progress_ = 0.0f;   // 0.0 → 1.0 over the animation

	static constexpr int  ANIM_DURATION_MS = 150;
	static constexpr float ANIM_MAX_SCALE   = 1.05f;
	static constexpr float ANIM_START_SCALE = 0.85f;
	static constexpr float ANIM_START_ALPHA = 0.00f;

public:
	// Advance animation by dt_ms. Returns true while still active.
	bool tick_anim(int dt_ms);
	float get_anim_progress() const { return anim_progress_; }
	void start_close_animation();
	void tick(int dt_ms) override;

protected:
    void paint_self    (const Rect& dirty, void* fb, uint32_t stride) override;
    bool on_mouse_move   (const InputEvent& ev) override;
    bool on_mouse_press  (const InputEvent& ev) override;
    bool on_mouse_release(const InputEvent& ev) override;
    void on_resize() override;
    void paint(const Rect& dirty_screen, void* fb, uint32_t stride) override;
	void start_anim_open();
	void start_anim_close();


private:
    char    title_[128]        = {};
    bool    dragging_          = false;
    bool    resizing_          = false;
    Point   drag_origin_       = {};
    Point   resize_origin_     = {};
    Rect    rect_before_resize_ = {};

    // Built-in child widgets (allocated in ctor, freed in dtor)
    Label*  title_label_  = nullptr;
    Button* close_btn_    = nullptr;
    Button* min_btn_      = nullptr;

    bool in_titlebar  (Point screen_pt) const;
    bool in_resize_zone(Point screen_pt) const;
    void layout_controls();
};
