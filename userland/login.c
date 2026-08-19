#include "sys.h"
#include "font8x8.h"

#define SCREEN_W 1024
#define SCREEN_H 768
#define FB_ADDR  0x78000000ULL

typedef unsigned int   u32;
typedef unsigned char  u8;
typedef unsigned long  u64;

static u32       *g_fb;
static u32       *g_back;
static int        mouse_x = 512;
static int        mouse_y = 384;
static int        mouse_down = 0;

static char       username[32];
static int        username_len = 0;
static char       password[32];
static int        password_len = 0;
static int        login_error = 0;
static u64        error_time = 0;
static int        field_focus = 0;

static u32        rng_state = 0x5A5A5A5A;
static u64        start_time = 0;
static u32 rnd(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

#define MAX_PARTICLES 220
static int p_x[MAX_PARTICLES], p_y[MAX_PARTICLES];
static int p_r[MAX_PARTICLES], p_vy[MAX_PARTICLES];
static int num_particles = 0;

static void particles_init(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        p_x[i] = (int)(rnd() % SCREEN_W);
        p_y[i] = (int)(rnd() % SCREEN_H);
        p_r[i] = 1 + (int)(rnd() % 3);
        p_vy[i] = 1 + (int)(rnd() % 4);
        num_particles++;
    }
}

static void particles_update(void) {
    for (int i = 0; i < num_particles; i++) {
        p_y[i] += p_vy[i];
        if (p_y[i] - p_r[i] > SCREEN_H) {
            p_y[i] = -p_r[i];
            p_x[i] = (int)(rnd() % SCREEN_W);
        }
    }
}

static u32 blend(u32 bg, u32 fg, int a) {
    if (a <= 0) return bg;
    if (a >= 255) return fg;
    int ia = 255 - a;
    int r = (((bg>>16)&0xFF)*ia + ((fg>>16)&0xFF)*a) / 255;
    int g = (((bg>> 8)&0xFF)*ia + ((fg>> 8)&0xFF)*a) / 255;
    int b = (((bg    )&0xFF)*ia + ((fg    )&0xFF)*a) / 255;
    return 0xFF000000u | (u32)(r<<16) | (u32)(g<<8) | (u32)b;
}

static void put_px(int x, int y, u32 c) {
    if ((u32)x < SCREEN_W && (u32)y < SCREEN_H)
        g_back[(u32)y * SCREEN_W + (u32)x] = c;
}

static void fill_rect(int x, int y, int w, int h, u32 c) {
    if (x < 0)          { w += x; x = 0; }
    if (y < 0)          { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    for (int j = 0; j < h; j++) {
        u32 *row = &g_back[(y + j) * SCREEN_W + x];
        for (int i = 0; i < w; i++) row[i] = c;
    }
}

static void fill_rect_alpha(int x, int y, int w, int h, u32 c, int a) {
    if (x < 0)          { w += x; x = 0; }
    if (y < 0)          { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    for (int j = 0; j < h; j++) {
        u32 *row = g_back + (y + j) * SCREEN_W + x;
        for (int i = 0; i < w; i++) row[i] = blend(row[i], c, a);
    }
}

static void fill_round_rect(int x, int y, int w, int h, int rad, u32 c) {
    if (rad < 1) { fill_rect(x, y, w, h, c); return; }
    if (w < rad*2) rad = w/2;
    if (h < rad*2) rad = h/2;
    fill_rect(x+rad, y, w-2*rad, h, c);
    fill_rect(x, y+rad, rad, h-2*rad, c);
    fill_rect(x+w-rad, y+rad, rad, h-2*rad, c);
    for (int j = 0; j < rad; j++) {
        for (int i = 0; i < rad; i++) {
            int dx = rad-1-i, dy = rad-1-j;
            if (dx*dx + dy*dy <= rad*rad) {
                put_px(x+i,   y+j,     c);
                put_px(x+w-1-i, y+j,   c);
                put_px(x+i,   y+h-1-j, c);
                put_px(x+w-1-i, y+h-1-j, c);
            }
        }
    }
}

static void fill_round_alpha(int x, int y, int w, int h, int rad, u32 c, int a) {
    if (rad < 1) { fill_rect_alpha(x, y, w, h, c, a); return; }
    if (w < 2*rad) rad = w/2;
    if (h < 2*rad) rad = h/2;
    fill_rect_alpha(x+rad, y, w-2*rad, h, c, a);
    fill_rect_alpha(x, y+rad, rad, h-2*rad, c, a);
    fill_rect_alpha(x+w-rad, y+rad, rad, h-2*rad, c, a);
    for (int j = 0; j < rad; j++) {
        for (int i = 0; i < rad; i++) {
            int dx = rad-1-i, dy = rad-1-j;
            if (dx*dx + dy*dy <= rad*rad) {
                u32 col = blend(g_back[(y+j)*SCREEN_W + (x+i)], c, a);
                put_px(x+i,   y+j,     col);
                put_px(x+w-1-i, y+j,   col);
                put_px(x+i,   y+h-1-j, col);
                put_px(x+w-1-i, y+h-1-j, col);
            }
        }
    }
}

// ── Text ────────────────────────────────────────────────────────────────────
static void draw_char(int x, int y, char c, u32 color) {
    u8 uc = (u8)c;
    if (uc > 127) return;
    char *bitmap = font8x8_basic[uc];
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            if (bitmap[j] & (1 << i))
                put_px(x + i, y + j, color);
        }
    }
}

static void draw_text(int x, int y, const char *s, u32 color) {
    while (*s) { draw_char(x, y, *s, color); x += 8; s++; }
}

static void draw_text_center(int cx, int y, const char *s, u32 color) {
    int len = 0;
    while (s[len]) len++;
    draw_text(cx - len * 4, y, s, color);
}

static void draw_text_alpha(int x, int y, const char *s, u32 color, int a) {
    while (*s) {
        u8 uc = (u8)*s;
        if (uc <= 127) {
            char *bitmap = font8x8_basic[uc];
            for (int j = 0; j < 8; j++) {
                for (int i = 0; i < 8; i++) {
                    if (bitmap[j] & (1 << i)) {
                        u32 col = blend(g_back[(y+j)*SCREEN_W + (x+i)], color, a);
                        put_px(x + i, y + j, col);
                    }
                }
            }
        }
        x += 8; s++;
    }
}

// ── Mouse cursor (visible!) ─────────────────────────────────────────────────
static void draw_cursor(int x, int y) {
    if (x < 0) x = 0;
    if (x > SCREEN_W - 2) x = SCREEN_W - 2;
    if (y < 0) y = 0;
    if (y > SCREEN_H - 2) y = SCREEN_H - 2;
    // black outline + white pointer body + blue tip
    fill_rect(x-1, y-1, 14, 14, 0xFF000000);
    fill_rect(x,   y,   12, 12, 0xFFF5F7FA);
    fill_rect(x,   y,   4,   8, 0xFF3498DB);  // accent stripe
    put_px(x+12, y+12, 0xFF000000);
}

// ── Login screen ────────────────────────────────────────────────────────────
// Animates a shifting vertical gradient + floating particles, then draws a
// glass card with the login form on top.

#define CARD_W 420
#define CARD_H 330

static int        fade_in_done = 0;
static int        card_slide_y = 80;  /* card starts 80px below final position */

static void draw_background(u64 ticks) {
    /* Dark-to-blue gradient that shifts over time */
    int shift = (int)((ticks / 20) % 256);
    for (int y = 0; y < SCREEN_H; y++) {
        int tt = y * 1000 / SCREEN_H;
        int r = 12 + tt * 18 / 1000;
        int g = 16 + tt * 36 / 1000;
        int b = 48 + tt * 80 / 1000;
        int wv = (shift + y) & 63;
        r += wv / 10; g += wv / 14; b += wv / 7;
        u32 c = 0xFF000000u | ((u32)r << 16) | ((u32)g << 8) | (u32)b;
        u32 *row = &g_back[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = c;
    }

    /* Slow drifting glow orbs */
    for (int i = 0; i < 8; i++) {
        int v = i * 173 + (i << 11) + (int)(ticks >> 4);
        int ox = (v * 211) % (SCREEN_W + 200) - 100;
        int oy = ((v >> 3) * 283) % (SCREEN_H + 160) - 80;
        u32 orb_col = (i & 1) ? 0x5E81F4 : 0x7CE0FF;
        int sz = 60 + (i % 3) * 20;
        fill_rect_alpha(ox, oy, sz, sz, orb_col, 36 + (i & 1) * 12);
    }

    /* Particles with varied brightness */
    for (int i = 0; i < num_particles; i++) {
        int a = 60 + (p_y[i] * 120 / SCREEN_H);
        u32 col = 0xFFA7C4FF;
        if (p_r[i] >= 3) col = 0xFF7CE0FF;
        else if (p_r[i] == 1) col = 0xFFD0E8FF;
        fill_rect_alpha(p_x[i], p_y[i], p_r[i]*2, p_r[i]*2, col, a);
    }

    /* Clock in top-right corner */
    {
        int sec = (int)((ticks / 100) % 60);
        int min = (int)((ticks / 6000) % 60);
        int hr  = (int)((ticks / 360000 + 8) % 24);  /* offset to ~8:00 */
        char buf[6];
        buf[0] = '0' + (hr / 10);
        buf[1] = '0' + (hr % 10);
        buf[2] = ':';
        buf[3] = '0' + (min / 10);
        buf[4] = '0' + (min % 10);
        buf[5] = 0;
        draw_text_alpha(SCREEN_W - 70, 20, buf, 0xFFCCD6F6, 180);
    }
}

static void draw_login_card(u64 ticks) {
    int final_cx = (SCREEN_W - CARD_W) / 2;
    int final_cy = (SCREEN_H - CARD_H) / 2;

    /* Slide-in: card drops from below over the first ~40 ticks */
    if (card_slide_y > 0 && ticks > 5) {
        card_slide_y -= 4;
        if (card_slide_y < 0) card_slide_y = 0;
    }
    int cx = final_cx;
    int cy = final_cy + card_slide_y;

    /* Card alpha ramps up during slide-in */
    int card_alpha = (card_slide_y > 0) ? 200 + (80 - card_slide_y) * 1 : 245;
    if (card_alpha > 245) card_alpha = 245;

    /* soft drop shadow (moves with card) */
    fill_round_alpha(cx + 8, cy + 12, CARD_W, CARD_H, 18, 0x000000, 100 + (80 - card_slide_y));

    /* glass card body */
    fill_round_alpha(cx, cy, CARD_W, CARD_H, 18, 0x0E0E16, card_alpha);

    /* top accent bar — animated gradient hue */
    int hue = (int)((ticks / 15) % 120);
    u32 accent;
    if (hue < 40)      accent = 0xFF28B0C9;  /* teal */
    else if (hue < 80) accent = 0xFF8E5CF7;  /* purple */
    else               accent = 0xFF3498DB;  /* blue */
    fill_round_rect(cx, cy, CARD_W, 3, 2, accent);

    /* subtle animated edge glow (pulses with time) */
    int pulse = (int)((ticks / 8) % 60);
    int edge_a = 20 + (pulse < 30 ? pulse : 60 - pulse);
    fill_rect_alpha(cx - 1, cy - 1, CARD_W + 2, 1, 0xFF5E81F4, edge_a);
    fill_rect_alpha(cx - 1, cy + CARD_H, CARD_W + 1, 1, 0xFF5E81F4, edge_a);

    /* border */
    fill_rect_alpha(cx - 1, cy - 1, CARD_W + 2, 1, 0x2A2A3C, 100);
    fill_rect_alpha(cx - 1, cy + CARD_H, CARD_W + 1, 1, 0x2A2A3C, 100);
    fill_rect_alpha(cx - 1, cy - 1, 1, CARD_H + 1, 0x2A2A3C, 100);
    fill_rect_alpha(cx + CARD_W, cy - 1, 1, CARD_H + 1, 0x2A2A3C, 100);

    /* --- header --- */
    draw_text_center(cx + CARD_W / 2, cy + 34, "ShadowBox OS", 0xFFE8E8F0);
    draw_text_center(cx + CARD_W / 2, cy + 52, "sign in to continue", 0xFF9898B0);
    fill_rect_alpha(cx + 30, cy + 68, CARD_W - 60, 1, 0x2A2A3C, 100);
}

// Input-field box drawing (glass field with focus ring)
static void draw_field(int x, int y, int w, int h, int focus) {
    u32 body = focus ? 0xFF2C3E50 : 0xFF1B2631;
    u32 ring = focus ? 0xFF5E81F4 : 0xFF3498DB;
    fill_round_rect(x, y, w, h, 6, body);
    // focus glow underneath
    if (focus) fill_round_alpha(x-2, y-2, w+4, h+4, 8, ring, 40);
    // bottom accent line
    fill_rect(x, y + h - 2, w, 2, focus ? ring : 0xFF3A3A4A);
}

static void draw_button(int x, int y, int w, int h, int hover, int down) {
    u32 col = 0xFF3498DB;
    if (hover) col = 0xFF2980B9;
    if (down)  col = 0xFF1F618D;
    fill_round_rect(x, y, w, h, 8, col);
    // subtle top highlight + bottom shade for depth
    fill_rect_alpha(x + 6, y + 1, w - 12, 2, 0xFFFFFFFF, 70);
    fill_rect_alpha(x + 6, y + h - 2, w - 12, 2, 0x000000, 50);
    draw_text_center(x + w/2, y + (h - 8)/2, "Sign In", 0xFFFFFFFF);
}

static void draw_login_screen(u64 ticks) {
    draw_background(ticks);
    draw_login_card(ticks);

    int final_cx = (SCREEN_W - CARD_W) / 2;
    int final_cy = (SCREEN_H - CARD_H) / 2;
    int cy_offset = card_slide_y;
    int cx = final_cx;
    int cy = final_cy + cy_offset;
    int fw = CARD_W - 80;         // field width
    int fx = cx + 40;             // field x

    /* Don't draw fields until card is mostly in place */
    if (card_slide_y > 30) return;

    /* ---- username ---- */
    draw_text(fx, cy + 90,  "USERNAME", 0xFF9898B0);
    int ux = fx, uy = cy + 100, uw = fw, uh = 38;
    draw_field(ux, uy, uw, uh, field_focus == 0);
    if (username_len == 0) {
        draw_text_alpha(ux + 12, uy + 11, "enter username", 0xFF7F8C8F, 130);
    } else {
        draw_text(ux + 12, uy + 11, username, 0xFFECF0F1);
    }

    /* --- password ---- */
    draw_text(fx, cy + 162, "PASSWORD", 0xFF9898B0);
    int px = fx, py = cy + 172, pw = fw, ph = 38;
    draw_field(px, py, pw, ph, field_focus == 1);
    if (password_len == 0) {
        draw_text_alpha(fx + 12, py + 11, "enter password", 0xFF7A849C, 130);
    } else {
        for (int i = 0; i < password_len; i++) {
            int d = (i % 15 < 8) ? 0xFF28B0C9 : 0xFF3498DB;
            fill_round_rect(fx + 12 + i * 12, py + 14, 8, 8, 4, d);
        }
    }

    /* --- sign in button ---- */
    int bx = fx, by = cy + 238, bw = fw, bh = 44;
    int hover = (mouse_x >= bx && mouse_x < bx + bw &&
                 mouse_y >= by && mouse_y < by + bh);
    draw_button(bx, by, bw, bh, hover, mouse_down);

    /* --- error / status --- */
    if (login_error) {
        u64 t = (ticks - error_time);
        if (t < 300) {
            int shake = 0;
            if (t < 20) shake = ((int)(t * 3) % 6) - 3;
            draw_text_center(cx + CARD_W / 2 + shake, cy + 312,
                             "Invalid credentials - try again", 0xFFEF4444);
        } else {
            login_error = 0;
        }
    }

    /* Fade-in overlay: fades from black over the first ~40 ticks */
    if (ticks < 40) {
        int a = 255 - (int)(ticks * 255 / 40);
        if (a > 0) fill_rect_alpha(0, 0, SCREEN_W, SCREEN_H, 0xFF000000, a);
    }
}

// ── Input handling ──────────────────────────────────────────────────────────
#define EV_MOUSE_MOVE   2
#define EV_MOUSE_BTN    3
#define EV_KEY_PRESS    0
#define EV_MOUSE_LEFT   0

typedef struct {
    u8          type;
    u8          code;
    signed short x;
    signed short y;
    unsigned short value;
} input_event_t;

static int streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

static void attempt_login(void) {
    if (username_len > 0 && password_len > 0 &&
        streq(username, "shadow-box") &&
        streq(password, "shadow-box")) {
        // success -> launch desktop
        sb_morph("desktop.elf", 0, 0);
        sb_terminate(1);
    } else {
        login_error = 1;
        error_time = sys_times(0);
        username_len = 0; username[0] = 0;
        password_len = 0; password[0] = 0;
    }
}

static void handle_key(u8 scancode, char ch) {
    if (scancode == 0x0F) {          /* Tab: switch field */
        field_focus = 1 - field_focus;
        return;
    }
    if (scancode == 0x1C) {          /* Enter: attempt login */
        attempt_login();
        return;
    }
    if (scancode == 0x01) {          /* Escape */
        sb_terminate(0);
        return;
    }
    if (scancode == 0x0E || scancode == 0x53) { /* Backspace / Delete */
        if (field_focus == 0) {
            if (username_len > 0) { username_len--; username[username_len] = 0; }
        } else {
            if (password_len > 0) { password_len--; password[password_len] = 0; }
        }
        return;
    }
    if (ch >= 32 && ch < 127) {
        if (field_focus == 0) {
            if (username_len < 31) { username[username_len++] = ch; username[username_len] = 0; }
        } else {
            if (password_len < 31) { password[password_len++] = ch; password[password_len] = 0; }
        }
    }
}

static void handle_click(int x, int y) {
    int cx = (SCREEN_W - CARD_W) / 2;
    int cy = (SCREEN_H - CARD_H) / 2;
    int fw = CARD_W - 80;
    int fx = cx + 40;

    // username field
    if (x >= fx && x < fx + fw && y >= cy + 100 && y < cy + 138) {
        field_focus = 0; return;
    }
    // password field
    if (x >= fx && x < fx + fw && y >= cy + 172 && y < cy + 210) {
        field_focus = 1; return;
    }
    // sign-in button
    if (x >= fx && x < fx + fw && y >= cy + 238 && y < cy + 282) {
        attempt_login();
    }
}

void _start(void) {
    /* Map framebuffer */
    if (syscall0(SYS_FB_MMAP) < 0) sb_terminate(1);
    g_fb = (u32 *)FB_ADDR;

    /* Back buffer */
    g_back = (u32 *)sys_sbrk(SCREEN_W * SCREEN_H * 4);
    if (!g_back) sb_terminate(2);

    /* Init state */
    username[0] = 0; password[0] = 0;
    username_len = 0; password_len = 0;
    field_focus = 0; login_error = 0;
    mouse_x = SCREEN_W / 2; mouse_y = SCREEN_H / 2;
    rng_state = (u32)sys_times(0) ^ 0x5A5A5A5A;
    start_time = sys_times(0);

    particles_init();

    /* Open the unified input device */
    int fd = sb_acquire("/dev/input", 0);
    if (fd < 0) sb_terminate(1);

    u64 last_draw = 0;
    u64 last_part = 0;

    for (;;) {
        input_event_t ev;
        int got = 0;

        while (sb_pull(fd, &ev, sizeof(ev)) == sizeof(ev)) {
            got = 1;
            if (ev.type == EV_MOUSE_MOVE) {
                mouse_x += ev.x;
                mouse_y += ev.y;             // kernel already negated dy
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > SCREEN_W - 4) mouse_x = SCREEN_W - 4;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > SCREEN_H - 4) mouse_y = SCREEN_H - 4;
            } else if (ev.type == EV_MOUSE_BTN) {
                if (ev.code == EV_MOUSE_LEFT) {
                    int was = mouse_down;
                    mouse_down = ev.x;   // button state in ev.x
                    if (ev.x && !was) handle_click(mouse_x, mouse_y);
                }
            } else if (ev.type == EV_KEY_PRESS) {
                handle_key(ev.code, (char)ev.x);
            }
        }

        u64 now = sys_times(0);
        int dirty = got;

        /* hz = 100 ticks/second.  Redraw every 3 ticks (~33 Hz) so the cursor
         * glides smoothly even during idle animation. */
        if ((now / 3) != (last_draw / 3)) dirty = 1;

        /* Update particles every 15 ticks (~6.6 Hz drift cadence) */
        if ((now / 15) != (last_part / 15)) {
            particles_update();
            last_part = now;
        }

        if (dirty) {
            draw_login_screen(now - start_time);
            draw_cursor(mouse_x, mouse_y);
            for (int j = 0; j < SCREEN_H; j++) {
                u32 *src = &g_back[j * SCREEN_W];
                u32 *dst = &g_fb[j * SCREEN_W];
                for (int i = 0; i < SCREEN_W; i++) dst[i] = src[i];
            }
            last_draw = now;
        }

        syscall0(SYS_SCHED_YIELD);
    }
}
