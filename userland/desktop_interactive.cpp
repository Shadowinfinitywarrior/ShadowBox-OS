#include "sys.h"
#include "font8x8.h"
#include "gui_bridge.h"

#include "Compositor.hpp"
#include "InputRouter.hpp"
#include "Window.hpp"
#include "Widget.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "TextBox.hpp"
#include "ScrollView.hpp"

#ifndef input_event_t
typedef struct {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
    int16_t value;
} input_event_t;
#endif

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define PITCH (SCREEN_WIDTH * 4)
#define MAX_WINDOWS 8

static uint32_t *fb = (uint32_t *)0x78000000ULL;
static uint32_t *backbuffer = NULL;
static Compositor *g_comp = NULL;
static InputRouter *g_input = NULL;
static Window *g_desktop = NULL;

static void init_wallpaper(void) {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    uint32_t rb = (50 + (y * 50 / SCREEN_HEIGHT)) << 16;
    uint32_t g = (10 + (y * 30 / SCREEN_HEIGHT)) << 8;
    uint32_t b = (100 + (y * 100 / SCREEN_HEIGHT));
    uint32_t c = rb | g | b;
    for (int x = 0; x < SCREEN_WIDTH; x++)
      backbuffer[y * SCREEN_WIDTH + x] = c;
  }
}

static void maybe_load_wallpaper(void) {
  int fd = sb_acquire("/wallpaper.bmp", 0);
  if (fd >= 0) {
    uint8_t header[54];
    if (sb_pull(fd, header, 54) == 54 && header[0] == 'B' && header[1] == 'M') {
      uint32_t offset = *(uint32_t *)&header[10];
      int w = *(int32_t *)&header[18], h = *(int32_t *)&header[22];
      if (offset > 54) {
        uint8_t dummy[128]; int skip = offset - 54;
        while (skip > 0) { int n = skip > 128 ? 128 : skip; sb_pull(fd, dummy, n); skip -= n; }
      }
      uint8_t row[1024 * 3 + 32];
      int row_bytes = (w * 3 + 3) & ~3;
      for (int y = h - 1; y >= 0; y--) {
        sb_pull(fd, row, row_bytes);
        for (int x = 0; x < w && x < SCREEN_WIDTH; x++) {
          uint8_t b = row[x*3], g = row[x*3+1], r = row[x*3+2];
          if (y < SCREEN_HEIGHT)
            backbuffer[y * SCREEN_WIDTH + x] = (r << 16) | (g << 8) | b;
        }
      }
      sb_release(fd);
      return;
    }
    sb_release(fd);
  }
  init_wallpaper();
}

static void term_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("ShadowBox Terminal");
  w->set_pos(100, 60);
  w->set_size(560, 340);

  TextBox *tb = new TextBox(w);
  tb->set_pos(6, 28);
  tb->set_size(540, 300);

  g_comp->add_root((Widget *)w);
}

static void filebrowser_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("File Explorer");
  w->set_pos(140, 90);
  w->set_size(480, 360);
  g_comp->add_root((Widget *)w);
}

static void sysmon_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("System Monitor");
  w->set_pos(120, 80);
  w->set_size(420, 260);
  g_comp->add_root((Widget *)w);
}

static void about_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("About ShadowBox");
  w->set_pos(360, 220);
  w->set_size(340, 200);

  Button *ok = new Button(w);
  ok->set_label("OK");
  ok->set_pos(260, 150);
  ok->set_size(60, 28);

  g_comp->add_root((Widget *)w);
}

static void viewer_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("Image Viewer");
  w->set_pos(180, 100);
  w->set_size(500, 400);
  g_comp->add_root((Widget *)w);
}

static void snake_create(void) {
  Window *w = new Window(nullptr);
  w->set_title("Snake Game");
  w->set_pos(200, 100);
  w->set_size(400, 420);
  g_comp->add_root((Widget *)w);
}

static void network_manager_create(void) {
    Window *w = new Window(nullptr);
    w->set_title("Network Manager");
    w->set_pos(250, 120);
    w->set_size(420, 300);
    TextBox *tb = new TextBox(w);
    tb->set_pos(6, 28);
    tb->set_size(408, 260);
    // Load network info from sysfs
    int fd = sb_acquire("/sys/net_info", 0);
    if (fd >= 0) {
        char buf[4096];
        uint64_t total = 0;
        uint64_t n;
        while ((n = sb_pull(fd, buf + total, sizeof(buf) - total)) > 0) {
            total += n;
            if (total >= sizeof(buf)) break;
        }
        sb_release(fd);
        if (total > 0) {
            if (total < sizeof(buf)) buf[total] = '\0';
            tb->set_text(buf);
        } else {
            tb->set_text("No network devices found.");
        }
    } else {
        tb->set_text("Unable to read network info.");
    }
    g_comp->add_root((Widget *)w);
}

extern "C" void _start(void) {
  backbuffer = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
  if ((int64_t)backbuffer <= 0) { syscall1(SB_TERMINATE, 2); }

  maybe_load_wallpaper();
  sys_fb_mmap();

  g_comp = new Compositor(fb, PITCH, SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!g_comp) { syscall1(SB_TERMINATE, 3); }
  g_comp->backbuf = backbuffer;

  g_input = new InputRouter(g_comp, SCREEN_WIDTH, SCREEN_HEIGHT);

  g_desktop = new Window(nullptr);
  g_desktop->set_pos(0, 0);
  g_desktop->set_size(SCREEN_WIDTH, SCREEN_HEIGHT);
  g_comp->add_root((Widget *)g_desktop);

  about_create();
    network_manager_create();

  int input_fd = sb_acquire("/dev/input", 0);
  if (input_fd < 0) { syscall1(SB_TERMINATE, 4); }

  g_comp->frame();

  while (1) {
    input_event_t ev;
    if (sb_pull(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
      if (ev.type == 2) {
        g_input->inject_mouse_packet(0, ev.x, -ev.y);
      } else if (ev.type == 3) {
        if (ev.code == 1) {
          g_input->inject_mouse_packet(1, 0, 0);
        } else if (ev.code == 0) {
          g_input->inject_mouse_packet(0, 0, 0);
        }
      } else if (ev.type == 0) {
        g_input->inject_key_press((uint32_t)ev.code, 0);
        g_input->inject_key_release((uint32_t)ev.code, 0);
      } else if (ev.type == 1) {
      }
    }
    g_comp->frame();
    syscall0(SYS_SCHED_YIELD);
  }
}
