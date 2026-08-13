// demo_main.cpp — Minimal demo that creates a SettingsPanel using the GUI toolkit.
extern "C" {
    #include "../../userland/sys.h"
    #include "../c/gui_bridge.h"
}

#include "Compositor.hpp"
#include "InputRouter.hpp"
#include "SettingsPanel.hpp"
#include "TopBar.hpp"
#include "SysmonWindow.hpp"
#include "CalculatorWindow.hpp"
#include "ClockWindow.hpp"
#include "SnakeWindow.hpp"
#include "TetrisWindow.hpp"
#include "MatrixWindow.hpp"
#include "HexEditWindow.hpp"

extern "C" void* sys_sbrk(long);
extern "C" void _start(void) {
    extern int main();
    int ret = main();
    syscall1(SB_TERMINATE, ret);
    while (1);
}

int main() {
    // Map framebuffer to userspace
    sys_fb_mmap();

    // Use the same frame buffer address as the rest of the OS.
    uint32_t *fb = (uint32_t *)0x78000000ULL;
    const int SCREEN_WIDTH = 1024;
    const int SCREEN_HEIGHT = 768;
    const int PITCH = SCREEN_WIDTH * 4;

    // Allocate a back buffer via the simple sbrk stub.
    uint32_t *backbuf = (uint32_t *)sys_sbrk(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if ((int64_t)backbuf < 0) return 1;

    // Initialise compositor.
    Compositor comp(fb, PITCH, SCREEN_WIDTH, SCREEN_HEIGHT);
    comp.backbuf = backbuf;

    // Create input router (initializes mouse cursor)
    InputRouter* router = new InputRouter(&comp, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    auto* clock_win = new ClockWindow(nullptr);
    clock_win->set_pos(350, 50);
    comp.add_root(clock_win, false);
    
    auto* snake_win = new SnakeWindow(nullptr);
    snake_win->set_pos(600, 100);
    comp.add_root(snake_win, false);
    
    auto* tetris_win = new TetrisWindow(nullptr);
    tetris_win->set_pos(100, 400);
    comp.add_root(tetris_win, false);
    
    auto* matrix_win = new MatrixWindow(nullptr);
    matrix_win->set_pos(150, 200);
    comp.add_root(matrix_win, false);
    
    auto* hex_win = new HexEditWindow(nullptr);
    hex_win->set_pos(250, 250);
    hex_win->load_file("/init.elf"); // Load an example file
    comp.add_root(hex_win, false);
    
    auto* topbar = new TopBar(nullptr, SCREEN_WIDTH);
    comp.add_root(topbar);

    SysmonWindow* sysmon = new SysmonWindow(nullptr);
    comp.add_root(sysmon);

    SettingsPanel *panel = new SettingsPanel(nullptr);
    comp.add_root(panel);

    CalculatorWindow *calc = new CalculatorWindow(nullptr);
    calc->set_pos(100, 200);
    comp.add_root(calc);

    int input_fd = sb_acquire("/dev/input", 0);
    if (input_fd < 0) return 1;

    int mouse_x = SCREEN_WIDTH / 2;
    int mouse_y = SCREEN_HEIGHT / 2;
    uint8_t mouse_buttons = 0;
    
    // Inject initial position
    router->inject_mouse_absolute(mouse_x, mouse_y, mouse_buttons);

    // Initial frame
    comp.frame();

    uint64_t last_time = sys_times(0);

    while (1) {
        input_event_t ev;
        
        uint64_t current_time = sys_times(0);
        int dt_ms = (int)((current_time - last_time) * 10);
        
        while (sb_pull(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == 2) { // Mouse Movement
                mouse_x += ev.x; mouse_y += ev.y;
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > SCREEN_WIDTH - 2) mouse_x = SCREEN_WIDTH - 2;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > SCREEN_HEIGHT - 2) mouse_y = SCREEN_HEIGHT - 2;
                router->inject_mouse_absolute(mouse_x, mouse_y, mouse_buttons);
            } else if (ev.type == 3) { // Mouse Button
                if (ev.code == 0) { // Left
                    if (ev.value == 1) mouse_buttons |= 0x01;
                    else mouse_buttons &= ~0x01;
                    router->inject_mouse_absolute(mouse_x, mouse_y, mouse_buttons);
                } else if (ev.code == 1) { // Right
                    if (ev.value == 1) mouse_buttons |= 0x02;
                    else mouse_buttons &= ~0x02;
                    router->inject_mouse_absolute(mouse_x, mouse_y, mouse_buttons);
                } else if (ev.code == 3) { // Wheel
                    router->inject_scroll(ev.value);
                }
            } else if (ev.type == 0) { // Key Press
                router->inject_key_press(ev.code & 0x7F, 0); // Need proper mod keys later
            } else if (ev.type == 1) { // Key Release
                router->inject_key_release(ev.code & 0x7F, 0);
            }
        }
        
        if (dt_ms > 0) {
            comp.frame(dt_ms);
            last_time = current_time;
        } else {
            syscall1(SYS_SCHED_YIELD, 0);
        }
    }

    return 0;
}
