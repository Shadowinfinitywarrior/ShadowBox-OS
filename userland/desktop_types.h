#ifndef SHADOWBOX_DESKTOP_TYPES_H
#define SHADOWBOX_DESKTOP_TYPES_H

#include <stdint.h>
#include "sys.h" // For struct dirent

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

#define WTYPE_TERMINAL  0
#define WTYPE_FILE_BRO  1
#define WTYPE_SYS_MON   2
#define WTYPE_ABOUT     3
#define WTYPE_VIEWER    4
#define WTYPE_SNAKE     5
#define WTYPE_CALC      6
#define WTYPE_EDITOR    7
#define WTYPE_PAINT     8
#define WTYPE_PROCMON   9
#define WTYPE_HEXVIEW   10
#define WTYPE_TETRIS    11
#define WTYPE_G2048     12
#define WTYPE_MANDEL    13
#define WTYPE_CLOCK     14
#define WTYPE_FORTUNE   15
#define WTYPE_PONG      16
#define WTYPE_MATRIX    17
#define WTYPE_PKG_MGR   18
#define WTYPE_REMINDER  19
#define WTYPE_SETTINGS  20
#define WTYPE_KBD_POWER 21
#define WTYPE_FILE_MANAGER 22
#define WTYPE_NOTEPAD   23
#define WTYPE_FONTVIEW  24
#define WTYPE_MEMORY_VIEW 25

#define MAX_WINDOWS 16

typedef struct window_t {
    int id;
    int active;
    int type; int focused; int always_on_top;
    int x, y, w, h;
    char title[64];
    uint32_t bg_color;
    int minimized;
    
    // Terminal / Editor
    char text[24 * 60];
    int cursor_x, cursor_y;
    char file_path[256]; // Path for editor file
    
    // For File Browser
    struct dirent entries[32];
    int num_entries;
    char current_dir[128];
    int sel_line;
    
    // For SysMon
    uint64_t last_update;
    
    // For Snake Game
    int snake_x[64];
    int snake_y[64];
    int snake_len;
    int snake_dir; // 0=Up, 1=Right, 2=Down, 3=Left
    int food_x, food_y;
    int snake_dead;
    
    // For Calculator
    int64_t calc_acc;
    int64_t calc_disp;
    int calc_op;     // '+','-','*','/' or 0
    int calc_fresh;
    
    // For Paint
    uint32_t *paint_canvas;
    int painting;
    uint32_t paint_color;
    int brush_size;
    
    // For Tetris
    signed char tetris_board[10 * 22];
    int tetris_px, tetris_py;
    int tetris_type, tetris_next;
    int tetris_score, tetris_lines, tetris_over;
    
    // For 2048
    int g2048[16];
    int g2048_score;
    int g2048_over;
    
    // For Pong
    int pong_py, pong_ai;
    int pong_bx, pong_by, pong_vx, pong_vy;
    int pong_s1, pong_s2, pong_over;
    
    // For Hex Viewer
    int hex_fd;
    int hex_offset;
    
    // For Process Monitor
    int proc_scroll;
    
    // For Matrix Rain
    int matrix_off[40];
    
    // For Mandelbrot
    uint32_t *mandel_buf;
    int mandel_ready;
    
    // For Fortune
    int fortune_idx;

    // For Settings
    int settings_category;  /* 0=Appearance, 1=Display, 2=Audio, 3=Network, 4=Power, 5=Users, 6=Accessibility, 7=System */
    int settings_scroll;
    int settings_toggle[32]; /* toggle states for each option */
    char settings_text[32][32]; /* text values for settings options */
} window_t;

extern window_t windows[MAX_WINDOWS];
extern int num_windows;

#endif
