#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

void _start(void) {
    print("=== ShadowBox Spreadsheet ===\n");
    print("[Placeholder] Spreadsheet functionality not yet implemented.\n");
    sb_terminate(0);
}
