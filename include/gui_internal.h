#pragma once

#include <cstdint>
#include <cstddef>
#include "c_std.h"

struct Point { int32_t x=0, y=0; constexpr Point()=default; constexpr Point(int32_t x_, int32_t y_):x(x_),y(y_){}; constexpr Point operator+(const Point& o) const { return {x+o.x, y+o.y}; }; constexpr Point operator-(const Point& o) const { return {x-o.x, y-o.y}; }; constexpr bool operator==(const Point& o) const { return x==o.x && y==o.y; } };

struct Rect { int32_t x=0, y=0, w=0, h=0; constexpr Rect()=default; constexpr Rect(int32_t x_, int32_t y_, int32_t w_, int32_t h_):x(x_),y(y_),w(w_),h(h_){}; constexpr bool contains(Point p) const { return p.x>=x && p.x<x+w && p.y>=y && p.y<y+h; }; constexpr bool intersects(const Rect& o) const { return !(o.x>=x+w || o.x+o.w<=x || o.y>=y+h || o.y+o.h<=y); }; constexpr Rect intersection(const Rect& o) const { int32_t ix=(x>o.x)?x:o.x; int32_t iy=(y>o.y)?y:o.y; int32_t ir=((x+w)<(o.x+o.w))?(x+w):(o.x+o.w); int32_t ib=((y+h)<(o.y+o.h))?(y+h):(o.y+o.h); if(ir<=ix || ib<=iy) return {0,0,0,0}; return {ix,iy,ir-ix,ib-iy}; }; constexpr Rect translated(int32_t dx, int32_t dy) const { return {x+dx, y+dy, w, h}; }; constexpr bool empty() const { return w<=0 || h<=0; } };

using Color = uint32_t;

namespace Colors { inline constexpr Color Transparent=0x00000000u; inline constexpr Color Black=0xFF000000u; inline constexpr Color White=0xFFFFFFFFu; inline constexpr Color DarkGray=0xFF1E1E1Eu; inline constexpr Color MidGray=0xFF3A3A3Au; inline constexpr Color LightGray=0xFFAAAAAu; inline constexpr Color Accent=0xFF5E81F4u; inline constexpr Color AccentHover=0xFF7C9BFFu; inline constexpr Color AccentActive=0xFF3D5BD4u; inline constexpr Color Danger=0xFFEF4444u; inline constexpr Color Success=0xFF10B981u; inline constexpr Color Warning=0xFFF59E0Bu; inline constexpr Color Text=0xFFE8E8F0u; inline constexpr Color TextDim=0xFF9898B0u; inline constexpr Color WindowBg=0xFF12121Au; inline constexpr Color BarBg=0xFF0E0E16u; inline constexpr Color Border=0xFF2A2A3Cu; inline constexpr Color Shadow=0x44000000u; }

inline Color alpha_blend(Color src, Color dst) { uint32_t a=(src>>24)&0xFF; if(a==0xFF) return src; if(a==0x00) return dst; uint32_t ia=255u-a; uint32_t r=((src>>16&0xFF)*a+(dst>>16&0xFF)*ia)/255u; uint32_t g=((src>>8&0xFF)*a+(dst>>8&0xFF)*ia)/255u; uint32_t b=((src&0xFF)*a+(dst&0xFF)*ia)/255u; return 0xFF000000u|(r<<16)|(g<<8)|b; }

inline Color dim(Color c, uint8_t factor) { uint32_t r=((c>>16)&0xFF)*factor/255u; uint32_t g=((c>>8)&0xFF)*factor/255u; uint32_t b=(c&0xFF)*factor/255u; return (c&0xFF000000u)|(r<<16)|(g<<8)|b; }

enum class EventType : uint8_t { MouseMove, MousePress, MouseRelease, MouseScroll, KeyPress, KeyRelease, FocusGained, FocusLost, Resize, Close };

enum class MouseButton : uint8_t { None=0, Left, Right, Middle };

enum class KeyMod : uint8_t { None=0, Shift=1, Ctrl=2, Alt=4 };

struct InputEvent { EventType type=EventType::MouseMove; Point pos={}; MouseButton button=MouseButton::None; int32_t scroll_delta=0; uint32_t key=0; uint8_t mods=0; };

struct DirtyList { static constexpr int MAX=64; Rect rects[MAX]; int count=0; void add(const Rect& r) { if(r.w<=0 || r.h<=0) return; if(count<MAX) rects[count++]=r; else { rects[0]={0,0,4096,4096}; count=1; } } void clear() { count=0; } bool empty() const { return count==0; } };
