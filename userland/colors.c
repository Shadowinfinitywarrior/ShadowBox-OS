#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("=== ANSI Color Palette ===\n\n");
    print("Standard Colors:\n");
    print("\033[30m Black \033[0m | \033[31m Red \033[0m | \033[32m Green \033[0m | \033[33m Yellow \033[0m\n");
    print("\033[34m Blue \033[0m  | \033[35m Mag \033[0m | \033[36m Cyan \033[0m  | \033[37m White \033[0m\n\n");
    
    print("Bold Colors:\n");
    print("\033[1;30m Black \033[0m | \033[1;31m Red \033[0m | \033[1;32m Green \033[0m | \033[1;33m Yellow \033[0m\n");
    print("\033[1;34m Blue \033[0m  | \033[1;35m Mag \033[0m | \033[1;36m Cyan \033[0m  | \033[1;37m White \033[0m\n\n");

    print("Background Colors:\n");
    print("\033[40m \033[37m Black \033[0m | \033[41m \033[37m Red \033[0m | \033[42m \033[30m Green \033[0m | \033[43m \033[30m Yellow \033[0m\n");
    print("\033[44m \033[37m Blue \033[0m  | \033[45m \033[37m Mag \033[0m | \033[46m \033[30m Cyan \033[0m  | \033[47m \033[30m White \033[0m\n\n");

    sb_terminate(0);
}
