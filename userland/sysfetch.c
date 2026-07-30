#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

static void print_uint(uint64_t val) {
    char buf[24]; int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

void _start(void) {
    uint64_t tbuf[2]; sys_times(tbuf);
    uint64_t hz = tbuf[1]; if (!hz) hz = 100;
    uint64_t secs = tbuf[0] / hz;

    uint64_t minfo[2]; sys_mem_info(minfo);
    uint64_t mem_total = minfo[0] * 4;
    uint64_t mem_used = minfo[1] * 4;

    struct proc_info pinfo[32];
    int procs = sys_proc_info(pinfo, 32);

    print("\033[36m");
    print("       .---.       \033[0m ShadowBox OS v0.3.0\n\033[36m");
    print("      /     \\      \033[0m -------------------\n\033[36m");
    print("     | () () |     \033[0m Kernel: ShadowBox microkernel x86_64\n\033[36m");
    print("      \\  ^  /      \033[0m Uptime: "); print_uint(secs); print(" seconds\n\033[36m");
    print("       |||||       \033[0m Memory: "); print_uint(mem_used); print(" KB / "); print_uint(mem_total); print(" KB\n\033[36m");
    print("       |||||       \033[0m Procs : "); print_uint(procs); print("\n\033[36m");
    print("                   \033[0m Shell : ShadowShell\n");
    print("\n");
    
    // Print color blocks
    print("   \033[41m  \033[42m  \033[43m  \033[44m  \033[45m  \033[46m  \033[47m  \033[0m\n\n");
    
    sb_terminate(0);
}
