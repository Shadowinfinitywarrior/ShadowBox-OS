#include "sys.h"

#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_CLEAR   "\033[2J\033[H"

#define HIST_MAX 32
static char history[HIST_MAX][256];
static int hist_count = 0;
static int hist_pos = 0;

// Environment variables
#define MAX_ENV 32
#define ENV_VAL_LEN 128
static char env_names[MAX_ENV][64];
static char env_vals[MAX_ENV][ENV_VAL_LEN];
static int env_count = 0;

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

static char read_stdin(void) {
    char c;
    while (1) {
        long r = (long)sb_pull(0, &c, 1);
        if (r > 0) return c;
        syscall0(SYS_SCHED_YIELD);
    }
}

static void print_n(const char *s, uint64_t n) {
    sb_push(1, s, n);
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int strncmp(const char *a, const char *b, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

static void strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
}

static int strcpy_n(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
    return i;
}

static void print_uint(uint64_t val) {
    char buf[24];
    int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

static void print_hex(uint64_t val, int digits) {
    for (int i = digits - 1; i >= 0; i--) {
        int nibble = (val >> (i * 4)) & 0xF;
        char c = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
        sb_push(1, &c, 1);
    }
}

static int atoi(const char *s) {
    int r = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') r = r * 10 + (*s++ - '0');
    return neg ? -r : r;
}

static void add_history(const char *line) {
    if (line[0] == '\0') return;
    int last = (hist_count > 0) ? (hist_pos - 1 + HIST_MAX) % HIST_MAX : -1;
    if (last >= 0 && strcmp(history[last], line) == 0) return;
    strcpy(history[hist_pos], line);
    hist_pos = (hist_pos + 1) % HIST_MAX;
    if (hist_count < HIST_MAX) hist_count++;
}

static const char *get_history(int offset) {
    if (hist_count == 0) return 0;
    int idx = (hist_pos - offset - 1 + HIST_MAX) % HIST_MAX;
    if (hist_count < HIST_MAX && idx >= hist_count) return 0;
    return history[idx];
}

// Environment variable helpers
static void env_set(const char *name, const char *value) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_names[i], name) == 0) {
            strcpy_n(env_vals[i], value, ENV_VAL_LEN);
            return;
        }
    }
    if (env_count < MAX_ENV) {
        strcpy_n(env_names[env_count], name, 64);
        strcpy_n(env_vals[env_count], value, ENV_VAL_LEN);
        env_count++;
    }
}

static const char *env_get(const char *name) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_names[i], name) == 0) return env_vals[i];
    }
    return 0;
}

static void env_unset(const char *name) {
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_names[i], name) == 0) {
            for (int j = i; j < env_count - 1; j++) {
                strcpy_n(env_names[j], env_names[j+1], 64);
                strcpy_n(env_vals[j], env_vals[j+1], ENV_VAL_LEN);
            }
            env_count--;
            return;
        }
    }
}

// Expand environment variables like $HOME in a string
static void expand_env(char *dst, const char *src, int max) {
    int di = 0;
    while (*src && di < max - 1) {
        if (*src == '$' && *(src+1) && *(src+1) != ' ') {
            src++;
            char varname[64];
            int vi = 0;
            while (*src && *src != ' ' && *src != '/' && vi < 63) {
                varname[vi++] = *src++;
            }
            varname[vi] = 0;
            const char *val = env_get(varname);
            if (val) {
                while (*val && di < max - 1) dst[di++] = *val++;
            }
        } else {
            dst[di++] = *src++;
        }
    }
    dst[di] = 0;
}

static void print_prompt(void) {
    const char *user = env_get("USER");
    if (!user) user = "shadow";
    const char *host = env_get("HOSTNAME");
    if (!host) host = "box";
    const char *cwd = env_get("PWD");
    if (!cwd) cwd = "/";

    print(ANSI_GREEN);
    print(user);
    print(ANSI_RESET "@" ANSI_CYAN);
    print(host);
    print(ANSI_RESET ":" ANSI_BLUE);
    print(cwd);
    print(ANSI_GREEN " $ " ANSI_RESET);
}

static const char *commands[] = {
    "help", "exit", "info", "memory", "time", "processes", "stop", "clear",
    "execute", "list", "read", "write", "create_dir", "remove_dir", "delete",
    "rename", "change_dir", "cat", "touch", "cp", "mv", "uptime", "whoami",
    "hostname", "env", "set", "unset", "export", "mount", "umount", "history",
    "ps", "dmesg", "uname", "wc", "echo", "free", "df", "id", "chmod", "chown",
    "log", "debug", "peek", "poke", "reboot", "shutdown",
    "ls", "rm", "mkdir", "rmdir", "head", "tail", "grep", "sleep", "hexdump", "base64", "date",
    "set_permissions", "set_owner", "show_tasks", "stop_task", "system_info",
    "disk_space", "kernel_log", "show_file", "clear_screen", "memory_usage",
    0
};

static void cmd_help(void) {
    print(ANSI_BOLD ANSI_CYAN "ShadowBox Shell v0.3.0 -- Commands:\n" ANSI_RESET);
    print(ANSI_GREEN "  File Operations:\n" ANSI_RESET);
    print("    cat <file>           Print file contents\n");
    print("    head <file>          Print first 10 lines\n");
    print("    tail <file>          Print last 10 lines\n");
    print("    grep <pat> <file>    Search pattern in file\n");
    print("    hexdump <file>       Show hex dump of file\n");
    print("    base64 <file>        Encode file to base64\n");
    print("    read <file>          Print file contents (alias)\n");
    print("    write <f> <text>     Write text to file\n");
    print("    touch <file>         Create empty file\n");
    print("    cp <src> <dst>       Copy file\n");
    print("    mv <src> <dst>       Move/rename file\n");
    print("    rm / delete <file>   Delete file\n");
    print("    ls / list [dir]      List directory contents\n");
    print(ANSI_GREEN "  Directory Operations:\n" ANSI_RESET);
    print("    mkdir / create_dir   Create directory\n");
    print("    rmdir / remove_dir   Remove empty directory\n");
    print("    change_dir <dir>     Change current directory\n");
    print("    pwd                  Print working directory\n");
    print(ANSI_GREEN "  Process Management:\n" ANSI_RESET);
    print("    execute <file>       Run a program\n");
    print("    processes / ps       List running processes\n");
    print("    stop <pid>           Kill a process\n");
    print("    sleep <sec>          Pause execution\n");
    print("    uptime               Show system uptime\n");
    print(ANSI_GREEN "  System Info:\n" ANSI_RESET);
    print("    info                 Show system information\n");
    print("    memory               Show memory usage\n");
    print("    free                 Show memory (alias)\n");
    print("    uname                Show kernel name/version\n");
    print("    df                   Show disk/filesystem info\n");
    print("    dmesg                Show kernel log\n");
    print("    date                 Show system time/ticks\n");
    print(ANSI_GREEN "  Environment:\n" ANSI_RESET);
    print("    env                  Show all variables\n");
    print("    set <name> <value>   Set environment variable\n");
    print("    unset <name>         Remove variable\n");
    print(ANSI_GREEN "  Filesystem:\n" ANSI_RESET);
    print("    mount <type> <path>  Mount filesystem\n");
    print("    umount <path>        Unmount filesystem\n");
    print(ANSI_GREEN "  Debugging:\n" ANSI_RESET);
    print("    peek <addr> [len]    Read memory at address\n");
    print("    poke <addr> <val>    Write memory at address\n");
    print("    debug                Show debug info\n");
    print(ANSI_GREEN "  Other:\n" ANSI_RESET);
    print("    echo <text>          Print text\n");
    print("    history              Show command history\n");
    print("    clear                Clear screen\n");
    print("    reboot               Reboot system\n");
    print("    exit                 Exit shell\n");
    print(ANSI_GREEN "  Simple Commands:\n" ANSI_RESET);
    print("    show_file <file>     Print file contents\n");
    print("    set_permissions      Change file permissions (chmod)\n");
    print("    set_owner            Change file owner (chown)\n");
    print("    show_tasks           List running processes\n");
    print("    stop_task <pid>      Kill a process\n");
    print("    system_info          Show kernel name/version\n");
    print("    disk_space           Show disk/filesystem info\n");
    print("    kernel_log           Show kernel log (dmesg)\n");
    print("    memory_usage         Show memory usage\n");
    print("    clear_screen         Clear screen\n");
    print("\n  " ANSI_YELLOW "Pipes supported: cmd1 | cmd2\n" ANSI_RESET);
    print("  " ANSI_YELLOW "Background: cmd &\n" ANSI_RESET);
}

static void cmd_info(void) {
    print(ANSI_CYAN "ShadowBox" ANSI_RESET " v" ANSI_YELLOW "0.3.0" ANSI_RESET " x86_64 Native OS\n");
    print("  Kernel: ShadowBox microkernel\n");
    print("  Architecture: x86_64\n");
    print("  Features: CFS scheduler, CoW fork, demand paging,\n");
    print("            ext2, tmpfs, devfs, procfs, networking\n");
}

static void cmd_processes(void) {
    struct proc_info buf[32];
    int n = sys_proc_info(buf, 32);
    print(ANSI_BOLD "  PID   PPID  STATE  CMD\n" ANSI_RESET);
    for (int i = 0; i < n; i++) {
        const char *state_str;
        switch (buf[i].state) {
            case 0: state_str = ANSI_GREEN "RUN" ANSI_RESET; break;
            case 1: state_str = ANSI_YELLOW "RDY" ANSI_RESET; break;
            case 2: state_str = ANSI_BLUE "SLP" ANSI_RESET; break;
            case 3: state_str = ANSI_RED "ZMB" ANSI_RESET; break;
            default: state_str = "???"; break;
        }
        print("  ");
        print_uint(buf[i].pid);
        int pad = 6 - (buf[i].pid > 0 ? 0 : 0);
        // Simple padding
        for (int p = 0; p < (6 - (buf[i].pid / 10 + 1)); p++) print(" ");
        print("  ");
        print_uint(buf[i].ppid);
        for (int p = 0; p < (6 - (buf[i].ppid / 10 + 1)); p++) print(" ");
        print("  ");
        print(state_str);
        print("\n");
    }
}

static void cmd_memory(void) {
    uint64_t info[2];
    sys_mem_info(info);
    uint64_t total_kb = info[0] * 4;
    uint64_t used_kb = info[1] * 4;
    uint64_t free_kb = total_kb - used_kb;
    print(ANSI_YELLOW "Memory Status:\n" ANSI_RESET);
    print("  Total:     "); print_uint(total_kb); print(" KB\n");
    print("  Used:      "); print_uint(used_kb); print(" KB\n");
    print("  Free:      "); print_uint(free_kb); print(" KB\n");
    print("  Used %%:    ");
    if (total_kb > 0) print_uint(used_kb * 100 / total_kb);
    print("%%\n");
}

static void cmd_time(void) {
    uint64_t tbuf[2];
    uint64_t ticks = sys_times(tbuf);
    uint64_t hz = tbuf[1];
    if (hz == 0) hz = 100;
    uint64_t secs = ticks / hz;
    uint64_t mins = secs / 60;
    uint64_t hours = mins / 60;
    print(ANSI_YELLOW "Uptime: " ANSI_RESET);
    print_uint(hours); print("h ");
    print_uint(mins % 60); print("m ");
    print_uint(secs % 60); print("s\n");
}

static void cmd_stop(const char *arg) {
    if (!arg || !*arg) { print("Usage: stop <pid>\n"); return; }
    int pid = atoi(arg);
    uint64_t ret = sys_kill((uint64_t)pid, 9);
    if (ret == 0) { print(ANSI_GREEN "Killed process " ANSI_RESET); print_uint(pid); print("\n"); }
    else { print(ANSI_RED "Failed to kill process " ANSI_RESET); print_uint(pid); print("\n"); }
}

static void cmd_execute(const char *arg, int background) {
    if (!arg || !*arg) { print("Usage: execute <file>\n"); return; }
    uint64_t pid = sb_replicate();
    if ((int64_t)pid == 0) {
        char *argv[] = {(char*)arg, 0};
        char *envp[] = {0};
        sb_morph(arg, argv, envp);
        print(ANSI_RED "Could not execute: "); print(arg); print("\n" ANSI_RESET);
        sb_terminate(127);
    } else if ((int64_t)pid > 0) {
        if (!background) {
            int status;
            sys_wait4(pid, &status, 0);
        } else {
            print("["); print_uint(pid); print("] running in background\n");
        }
    } else {
        print(ANSI_RED "Failed to fork\n" ANSI_RESET);
    }
}

static void cmd_list(const char *arg) {
    const char *path = arg && *arg ? arg : ".";
    int fd = sb_acquire(path, 0);
    if (fd < 0) { print(ANSI_RED "Cannot open directory: "); print(path); print("\n" ANSI_RESET); return; }
    struct dirent entry;
    int count = 0;
    while (1) {
        int r = sys_getdents(fd, &entry, sizeof(struct dirent));
        if (r <= 0) break;
        print(ANSI_BLUE);
        print(entry.name);
        print(ANSI_RESET "  ");
        count++;
        if (count % 6 == 0) print("\n");
    }
    if (count % 6 != 0) print("\n");
    sb_release(fd);
}

static void cmd_read_file(const char *arg) {
    if (!arg || !*arg) { print("Usage: cat <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found: "); print(arg); print("\n" ANSI_RESET); return; }
    char buffer[512];
    while (1) {
        int bytes = sb_pull(fd, buffer, 512);
        if (bytes <= 0) break;
        sb_push(1, buffer, bytes);
    }
    sb_release(fd);
}

static void cmd_write_file(const char *arg) {
    if (!arg || !*arg) { print("Usage: write <file> <text>\n"); return; }
    char filename[128]; int i = 0;
    while (*arg && *arg != ' ' && i < 127) filename[i++] = *arg++;
    filename[i] = '\0';
    while (*arg == ' ') arg++;
    if (!*arg) { print("Usage: write <file> <text>\n"); return; }
    int fd = sb_acquire(filename, 0x40 | 0x1 | 0x200);
    if (fd == -30) { print(ANSI_RED "Cannot create file: Read-only file system\n" ANSI_RESET); return; }
    if (fd == -1) { print(ANSI_RED "Cannot create file: Permission denied\n" ANSI_RESET); return; }
    if (fd < 0) { print(ANSI_RED "Cannot create file\n" ANSI_RESET); return; }
    sb_push(fd, arg, strlen(arg));
    sb_release(fd);
    print(ANSI_GREEN "Written to "); print(filename); print("\n" ANSI_RESET);
}

static void cmd_touch(const char *arg) {
    if (!arg || !*arg) { print("Usage: touch <file>\n"); return; }
    int fd = sb_acquire(arg, 0x40 | 0x1 | 0x200);
    if (fd >= 0) { sb_release(fd); print(ANSI_GREEN "Created "); print(arg); print("\n" ANSI_RESET); }
    else if (fd == -30) { print(ANSI_RED "Failed to create: Read-only file system\n" ANSI_RESET); }
    else if (fd == -1) { print(ANSI_RED "Failed to create: Permission denied\n" ANSI_RESET); }
    else { print(ANSI_RED "Failed to create "); print(arg); print("\n" ANSI_RESET); }
}

static void cmd_cp(const char *arg) {
    if (!arg || !*arg) { print("Usage: cp <src> <dst>\n"); return; }
    char src[128], dst[128]; int i = 0;
    while (*arg && *arg != ' ' && i < 127) src[i++] = *arg++;
    src[i] = '\0';
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg && *arg != ' ' && i < 127) dst[i++] = *arg++;
    dst[i] = '\0';
    int fd_in = sb_acquire(src, 0);
    if (fd_in < 0) { print(ANSI_RED "Source not found\n" ANSI_RESET); return; }
    int fd_out = sb_acquire(dst, 0x40 | 0x1 | 0x200);
    if (fd_out < 0) { print(ANSI_RED "Cannot create destination\n" ANSI_RESET); sb_release(fd_in); return; }
    char buf[512];
    while (1) {
        int r = sb_pull(fd_in, buf, 512);
        if (r <= 0) break;
        sb_push(fd_out, buf, r);
    }
    sb_release(fd_in);
    sb_release(fd_out);
    print(ANSI_GREEN "Copied "); print(src); print(" -> "); print(dst); print("\n" ANSI_RESET);
}

static void cmd_mv(const char *arg) {
    if (!arg || !*arg) { print("Usage: mv <src> <dst>\n"); return; }
    char src[128], dst[128]; int i = 0;
    while (*arg && *arg != ' ' && i < 127) src[i++] = *arg++;
    src[i] = '\0';
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg && *arg != ' ' && i < 127) dst[i++] = *arg++;
    dst[i] = '\0';
    int r = sys_rename(src, dst);
    if (r == 0) { print(ANSI_GREEN "Moved "); print(src); print(" -> "); print(dst); print("\n" ANSI_RESET); }
    else { print(ANSI_RED "Failed to move\n" ANSI_RESET); }
}

static void cmd_delete(const char *arg) {
    if (!arg || !*arg) { print("Usage: delete <file>\n"); return; }
    int r = sys_unlink(arg);
    if (r == 0) { print(ANSI_GREEN "Deleted "); print(arg); print("\n" ANSI_RESET); }
    else if (r == -30) { print(ANSI_RED "Read-only file system\n" ANSI_RESET); } // EROFS
    else if (r == -1) { print(ANSI_RED "Permission denied\n" ANSI_RESET); } // EPERM
    else { print(ANSI_RED "Failed to delete file\n" ANSI_RESET); }
}

static void cmd_create_dir(const char *arg) {
    if (!arg || !*arg) { print("Usage: create_dir <name>\n"); return; }
    int r = sys_mkdir(arg, 0777);
    if (r == 0) { print(ANSI_GREEN "Created directory "); print(arg); print("\n" ANSI_RESET); }
    else if (r == -30) { print(ANSI_RED "Read-only file system (Cannot write to root or protected mount)\n" ANSI_RESET); } // EROFS
    else if (r == -1) { print(ANSI_RED "Permission denied\n" ANSI_RESET); } // EPERM
    else { print(ANSI_RED "Failed to create directory\n" ANSI_RESET); }
}

static void cmd_remove_dir(const char *arg) {
    if (!arg || !*arg) { print("Usage: remove_dir <name>\n"); return; }
    int r = sys_rmdir(arg);
    if (r == 0) { print(ANSI_GREEN "Removed directory "); print(arg); print("\n" ANSI_RESET); }
    else if (r == -30) { print(ANSI_RED "Read-only file system\n" ANSI_RESET); } // EROFS
    else if (r == -1) { print(ANSI_RED "Permission denied\n" ANSI_RESET); } // EPERM
    else { print(ANSI_RED "Failed (dir not empty or not found)\n" ANSI_RESET); }
}

static void cmd_rename(const char *arg) {
    if (!arg || !*arg) { print("Usage: rename <old> <new>\n"); return; }
    char oldn[128], newn[128]; int i = 0;
    while (*arg && *arg != ' ' && i < 127) oldn[i++] = *arg++;
    oldn[i] = '\0';
    while (*arg == ' ') arg++;
    i = 0;
    while (*arg && i < 127) newn[i++] = *arg++;
    newn[i] = '\0';
    int r = sys_rename(oldn, newn);
    if (r == 0) { print(ANSI_GREEN "Renamed successfully\n" ANSI_RESET); }
    else { print(ANSI_RED "Failed to rename\n" ANSI_RESET); }
}

static void cmd_change_dir(const char *arg) {
    if (!arg || !*arg) { print("Usage: change_dir <dir>\n"); return; }
    int r = sys_chdir(arg);
    if (r == 0) { 
        if (arg[0] == '/') {
            env_set("PWD", arg);
        } else {
            const char *cwd = env_get("PWD");
            char new_pwd[256];
            if (arg[0] == '.' && arg[1] == '.' && arg[2] == 0) {
                int i = 0; while (cwd[i]) { new_pwd[i] = cwd[i]; i++; } new_pwd[i] = 0;
                int len = i;
                while (len > 0 && new_pwd[len - 1] != '/') len--;
                if (len > 1) len--; // remove trailing slash unless it's root '/'
                new_pwd[len] = 0;
                env_set("PWD", new_pwd);
            } else if (arg[0] == '.' && arg[1] == 0) {
                // do nothing
            } else {
                int i = 0; while (cwd[i]) { new_pwd[i] = cwd[i]; i++; } new_pwd[i] = 0;
                if (new_pwd[i - 1] != '/') { new_pwd[i++] = '/'; new_pwd[i] = 0; }
                int j = 0; while (arg[j]) { new_pwd[i++] = arg[j++]; } new_pwd[i] = 0;
                env_set("PWD", new_pwd);
            }
        }
    }
    else { print(ANSI_RED "Directory not found: "); print(arg); print("\n" ANSI_RESET); }
}

static void cmd_echo(const char *arg) {
    if (!arg) { print("\n"); return; }
    // Expand environment variables
    char expanded[512];
    expand_env(expanded, arg, 512);
    print(expanded);
    print("\n");
}

static void cmd_env_show(void) {
    print(ANSI_BOLD "Environment Variables:\n" ANSI_RESET);
    for (int i = 0; i < env_count; i++) {
        print(ANSI_GREEN);
        print(env_names[i]);
        print(ANSI_RESET "=" ANSI_YELLOW);
        print(env_vals[i]);
        print(ANSI_RESET "\n");
    }
}

static void cmd_set_var(const char *arg) {
    if (!arg || !*arg) { print("Usage: set <name> <value>\n"); return; }
    char name[64]; int i = 0;
    while (*arg && *arg != ' ' && i < 63) name[i++] = *arg++;
    name[i] = '\0';
    while (*arg == ' ') arg++;
    env_set(name, arg);
}

static void cmd_mount_fs(const char *arg) {
    if (!arg || !*arg) { print("Usage: mount <type> <path>\n  Types: tmpfs, devfs, proc\n"); return; }
    char ftype[64]; int i = 0;
    while (*arg && *arg != ' ' && i < 63) ftype[i++] = *arg++;
    ftype[i] = '\0';
    while (*arg == ' ') arg++;
    if (!*arg) { print("Usage: mount <type> <path>\n"); return; }
    int r = sys_mount(0, arg, ftype, 0);
    if (r == 0) { print(ANSI_GREEN "Mounted "); print(ftype); print(" at "); print(arg); print("\n" ANSI_RESET); }
    else { print(ANSI_RED "Failed to mount\n" ANSI_RESET); }
}

static void cmd_uptime(void) {
    uint64_t tbuf[2];
    uint64_t ticks = sys_times(tbuf);
    uint64_t hz = tbuf[1]; if (hz == 0) hz = 100;
    uint64_t secs = ticks / hz;
    print(ANSI_YELLOW "Uptime: " ANSI_RESET);
    print_uint(secs / 86400); print("d ");
    print_uint((secs / 3600) % 24); print("h ");
    print_uint((secs / 60) % 60); print("m ");
    print_uint(secs % 60); print("s\n");
}

static void cmd_uname(void) {
    print("ShadowBox 0.3.0 x86_64\n");
}

static void cmd_free(void) {
    cmd_memory();
}

static void cmd_df(void) {
    print(ANSI_BOLD "Filesystem     Type     Mounted\n" ANSI_RESET);
    print("/              tarfs    /\n");
    print("/dev           devfs    /dev\n");
    print("/proc          proc     /proc\n");
    print("/tmp           tmpfs    /tmp\n");
}

static void cmd_id(void) {
    print("uid=0(root) gid=0(root)\n");
}

static void cmd_history(void) {
    print(ANSI_BOLD "Command History:\n" ANSI_RESET);
    int start = hist_count < HIST_MAX ? 0 : hist_count - HIST_MAX;
    for (int i = start; i < hist_count; i++) {
        int idx = (hist_pos - hist_count + i + HIST_MAX) % HIST_MAX;
        print("  ");
        print_uint(i + 1);
        print("  ");
        print(history[idx]);
        print("\n");
    }
}

static void cmd_wc(const char *arg) {
    if (!arg || !*arg) { print("Usage: wc <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    uint64_t bytes = 0, lines = 0, words = 0;
    char buf[512];
    int in_word = 0;
    while (1) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        bytes += r;
        for (int i = 0; i < r; i++) {
            if (buf[i] == '\n') lines++;
            if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') in_word = 0;
            else if (!in_word) { words++; in_word = 1; }
        }
    }
    sb_release(fd);
    print_uint(lines); print(" ");
    print_uint(words); print(" ");
    print_uint(bytes); print(" "); print(arg); print("\n");
}

static void cmd_dmesg(void) {
    print(ANSI_YELLOW "[dmesg] Kernel messages available on serial output\n" ANSI_RESET);
}

static void cmd_debug(void) {
    print(ANSI_CYAN "=== ShadowBox Debug Info ===\n" ANSI_RESET);
    uint64_t tbuf[2]; sys_times(tbuf);
    print("  Tick count: "); print_uint(tbuf[0]); print("\n");
    uint64_t minfo[2]; sys_mem_info(minfo);
    print("  PMM total: "); print_uint(minfo[0]); print(" pages\n");
    print("  PMM used:  "); print_uint(minfo[1]); print(" pages\n");
    struct proc_info pinfo[32];
    int n = sys_proc_info(pinfo, 32);
    print("  Processes: "); print_uint(n); print("\n");
    print("  PID: "); print_uint(sys_getpid()); print("\n");
    print("  PPID: "); print_uint(sys_getppid()); print("\n");
}

static void cmd_log(void) {
    cmd_dmesg();
}

static void cmd_sleep(const char *arg) {
    if (!arg || !*arg) { print("Usage: sleep <seconds>\n"); return; }
    int secs = atoi(arg);
    sys_nanosleep(secs, 0);
}

static void cmd_head(const char *arg) {
    if (!arg || !*arg) { print("Usage: head <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    char buf[1];
    int lines = 0;
    while (lines < 10) {
        if (sb_pull(fd, buf, 1) <= 0) break;
        sb_push(1, buf, 1);
        if (buf[0] == '\n') lines++;
    }
    sb_release(fd);
}

static void cmd_tail(const char *arg) {
    if (!arg || !*arg) { print("Usage: tail <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    char buf[512];
    int lines = 0;
    while (1) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        for (int i=0; i<r; i++) if (buf[i] == '\n') lines++;
    }
    sb_release(fd);
    fd = sb_acquire(arg, 0);
    int to_skip = lines > 10 ? lines - 10 : 0;
    int skipped = 0;
    while (skipped < to_skip) {
        if (sb_pull(fd, buf, 1) <= 0) break;
        if (buf[0] == '\n') skipped++;
    }
    while (1) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        sb_push(1, buf, r);
    }
    sb_release(fd);
}

static void cmd_grep(const char *arg) {
    char pattern[64]; int i = 0;
    while (*arg && *arg != ' ' && i < 63) pattern[i++] = *arg++;
    pattern[i] = '\0';
    while (*arg == ' ') arg++;
    if (!*arg) { print("Usage: grep <pattern> <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    char buf[1];
    char line[256];
    int len = 0;
    while (sb_pull(fd, buf, 1) > 0) {
        if (buf[0] == '\n') {
            line[len] = '\0';
            int p_len = strlen(pattern);
            for (int j = 0; j <= len - p_len; j++) {
                if (strncmp(&line[j], pattern, p_len) == 0) {
                    print(line); print("\n"); break;
                }
            }
            len = 0;
        } else if (len < 255) {
            line[len++] = buf[0];
        }
    }
    sb_release(fd);
}

static void cmd_hexdump(const char *arg) {
    if (!arg || !*arg) { print("Usage: hexdump <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    unsigned char buf[16];
    uint64_t offset = 0;
    while (1) {
        int r = sb_pull(fd, buf, 16);
        if (r <= 0) break;
        print_hex(offset, 8); print("  ");
        for (int i = 0; i < 16; i++) {
            if (i < r) { print_hex(buf[i], 2); print(" "); }
            else print("   ");
            if (i == 7) print(" ");
        }
        print(" |");
        for (int i = 0; i < r; i++) {
            char c = buf[i];
            if (c >= 32 && c <= 126) sb_push(1, &c, 1);
            else print(".");
        }
        print("|\n");
        offset += r;
    }
    sb_release(fd);
}

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void cmd_base64(const char *arg) {
    if (!arg || !*arg) { print("Usage: base64 <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "File not found\n" ANSI_RESET); return; }
    unsigned char in[3];
    char out[4];
    while (1) {
        int r = sb_pull(fd, in, 3);
        if (r <= 0) break;
        out[0] = base64_table[in[0] >> 2];
        if (r == 1) {
            out[1] = base64_table[(in[0] & 0x03) << 4];
            out[2] = '='; out[3] = '=';
        } else if (r == 2) {
            out[1] = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            out[2] = base64_table[(in[1] & 0x0F) << 2];
            out[3] = '=';
        } else {
            out[1] = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            out[2] = base64_table[((in[1] & 0x0F) << 2) | (in[2] >> 6)];
            out[3] = base64_table[in[2] & 0x3F];
        }
        sb_push(1, out, 4);
    }
    print("\n");
    sb_release(fd);
}

static void cmd_date(void) {
    uint64_t tbuf[2];
    uint64_t ticks = sys_times(tbuf);
    print("Ticks since boot: "); print_uint(ticks); print("\n");
}

static int match_prefix(const char *prefix) {
    int matches = 0, last = -1;
    for (int i = 0; commands[i]; i++) {
        if (strncmp(commands[i], prefix, strlen(prefix)) == 0) {
            matches++; last = i;
        }
    }
    return (matches == 1) ? last : -1;
}

// Simple pipe execution
static void run_piped(const char *cmd1, const char *cmd2) {
    int pipefd[2];
    if (sys_pipe(pipefd) < 0) { print(ANSI_RED "Pipe failed\n" ANSI_RESET); return; }

    uint64_t pid1 = sb_replicate();
    if ((int64_t)pid1 == 0) {
        sys_dup2(pipefd[1], 1);
        sb_release(pipefd[0]);
        sb_release(pipefd[1]);
        char *argv[] = {(char*)cmd1, 0};
        sb_morph(cmd1, argv, 0);
        print(ANSI_RED "exec failed: "); print(cmd1); print("\n" ANSI_RESET);
        sb_terminate(127);
    }

    uint64_t pid2 = sb_replicate();
    if ((int64_t)pid2 == 0) {
        sys_dup2(pipefd[0], 0);
        sb_release(pipefd[0]);
        sb_release(pipefd[1]);
        char *argv[] = {(char*)cmd2, 0};
        sb_morph(cmd2, argv, 0);
        print(ANSI_RED "exec failed: "); print(cmd2); print("\n" ANSI_RESET);
        sb_terminate(127);
    }

    sb_release(pipefd[0]);
    sb_release(pipefd[1]);
    int status;
    sys_wait4(pid1, &status, 0);
    sys_wait4(pid2, &status, 0);
}

void _start(void) {
    print(ANSI_CLEAR);
    print(ANSI_BOLD ANSI_MAGENTA "  ╔══════════════════════════════════════╗\n" ANSI_RESET);
    print(ANSI_BOLD ANSI_MAGENTA "  ║      ShadowBox Native OS v0.3.0     ║\n" ANSI_RESET);
    print(ANSI_BOLD ANSI_MAGENTA "  ║      The Independent Kernel         ║\n" ANSI_RESET);
    print(ANSI_BOLD ANSI_MAGENTA "  ╚══════════════════════════════════════╝\n" ANSI_RESET);
    print("\nType " ANSI_GREEN "help" ANSI_RESET " for commands. Pipes " ANSI_YELLOW "|" ANSI_RESET " and background " ANSI_YELLOW "&" ANSI_RESET " supported.\n\n");

    // Set default environment
    env_set("USER", "root");
    env_set("HOSTNAME", "shadowbox");
    env_set("PWD", "/");
    env_set("HOME", "/");
    env_set("SHELL", "/shell.elf");
    env_set("PATH", "/:/tmp:/dev:/proc");
    env_set("TERM", "shadowbox-256color");
    env_set("VERSION", "0.3.0");

    char buf[256];
    int hist_idx = hist_count;

    while (1) {
        print_prompt();
        int i = 0;
        hist_idx = hist_count;

        while (i < 255) {
            char c;
            c = read_stdin();
            if (c == '\n' || c == '\r') { print("\n"); break; }
            else if (c == '\b' || c == 127) { if (i > 0) { print("\b \b"); i--; } }
            else if (c == '\033') {
                char seq[3];
                seq[0] = read_stdin();
                if (seq[0] == '[') {
                    seq[1] = read_stdin();
                    if (seq[1] == 'A') {
                        if (hist_idx > 0 && hist_idx <= hist_count) {
                            hist_idx--;
                            const char *h = get_history(hist_count - hist_idx - 1);
                            if (h) {
                                while (i > 0) { print("\b \b"); i--; }
                                strcpy(buf, h); i = strlen(h); print(h);
                            }
                        }
                    } else if (seq[1] == 'B') {
                        if (hist_idx < hist_count) {
                            hist_idx++;
                            while (i > 0) { print("\b \b"); i--; }
                            buf[0] = '\0'; i = 0;
                        }
                    }
                }
            } else if (c == '\t') {
                if (i > 0) {
                    buf[i] = '\0';
                    int m = match_prefix(buf);
                    if (m >= 0) {
                        while (i > 0) { print("\b \b"); i--; }
                        strcpy(buf, commands[m]); i = strlen(commands[m]);
                        print(buf); print(" "); buf[i++] = ' ';
                    }
                }
            } else if (c >= ' ' && c <= '~') {
                sb_push(1, &c, 1); buf[i++] = c;
            }
        }
        buf[i] = '\0';
        while (i > 0 && (buf[i-1] == ' ' || buf[i-1] == '\t')) buf[--i] = '\0';
        if (i == 0) continue;
        add_history(buf);

        // Check for pipe
        int pipe_pos = -1;
        for (int p = 0; buf[p]; p++) {
            if (buf[p] == '|') { pipe_pos = p; break; }
        }
        if (pipe_pos >= 0) {
            buf[pipe_pos] = '\0';
            const char *left = buf;
            const char *right = buf + pipe_pos + 1;
            while (*left == ' ') left++;
            while (*right == ' ') right++;
            run_piped(left, right);
            continue;
        }

        // Check for background &
        int background = 0;
        if (i > 0 && buf[i-1] == '&') {
            background = 1;
            buf[--i] = '\0';
            while (i > 0 && (buf[i-1] == ' ' || buf[i-1] == '\t')) buf[--i] = '\0';
        }

        const char *arg = buf;
        while (*arg && *arg != ' ') arg++;
        int cmd_len = arg - buf;
        while (*arg == ' ') arg++;

        // Expand $ variables in arg
        char expanded_arg[512];
        expand_env(expanded_arg, arg, 512);

        if (cmd_len == 4 && strncmp(buf, "help", 4) == 0) cmd_help();
        else if (cmd_len == 4 && strncmp(buf, "exit", 4) == 0) { print("Goodbye.\n"); sb_terminate(0); }
        else if (cmd_len == 4 && strncmp(buf, "info", 4) == 0) cmd_info();
        else if ((cmd_len == 9 && strncmp(buf, "processes", 9) == 0) || (cmd_len == 2 && strncmp(buf, "ps", 2) == 0) || (cmd_len == 10 && strncmp(buf, "show_tasks", 10) == 0)) cmd_processes();
        else if ((cmd_len == 6 && strncmp(buf, "memory", 6) == 0) || (cmd_len == 12 && strncmp(buf, "memory_usage", 12) == 0)) cmd_memory();
        else if (cmd_len == 4 && strncmp(buf, "time", 4) == 0) cmd_time();
        else if ((cmd_len == 4 && strncmp(buf, "stop", 4) == 0) || (cmd_len == 9 && strncmp(buf, "stop_task", 9) == 0)) cmd_stop(expanded_arg);
        else if (cmd_len == 7 && strncmp(buf, "execute", 7) == 0) cmd_execute(expanded_arg, background);
        else if ((cmd_len == 5 && strncmp(buf, "clear", 5) == 0) || (cmd_len == 12 && strncmp(buf, "clear_screen", 12) == 0)) print(ANSI_CLEAR);
        else if (cmd_len == 4 && strncmp(buf, "list", 4) == 0) cmd_list(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "read", 4) == 0) cmd_read_file(expanded_arg);
        else if ((cmd_len == 3 && strncmp(buf, "cat", 3) == 0) || (cmd_len == 9 && strncmp(buf, "show_file", 9) == 0)) cmd_read_file(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "write", 5) == 0) cmd_write_file(expanded_arg);
        else if (cmd_len == 10 && strncmp(buf, "create_dir", 10) == 0) cmd_create_dir(expanded_arg);
        else if (cmd_len == 10 && strncmp(buf, "remove_dir", 10) == 0) cmd_remove_dir(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "delete", 6) == 0) cmd_delete(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "rename", 6) == 0) cmd_rename(expanded_arg);
        else if (cmd_len == 10 && strncmp(buf, "change_dir", 10) == 0) cmd_change_dir(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "cd", 2) == 0) cmd_change_dir(expanded_arg);
        else if (cmd_len == 3 && strncmp(buf, "pwd", 3) == 0) {
            const char *cwd = env_get("PWD");
            print(cwd ? cwd : "/"); print("\n");
        }
        else if (cmd_len == 4 && strncmp(buf, "echo", 4) == 0) cmd_echo(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "touch", 5) == 0) cmd_touch(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "cp", 2) == 0) cmd_cp(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "mv", 2) == 0) cmd_mv(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "uptime", 5) == 0) cmd_uptime();
        else if (cmd_len == 6 && strncmp(buf, "whoami", 6) == 0) {
            const char *u = env_get("USER"); print(u ? u : "root"); print("\n");
        }
        else if (cmd_len == 8 && strncmp(buf, "hostname", 8) == 0) {
            const char *h = env_get("HOSTNAME"); print(h ? h : "shadowbox"); print("\n");
        }
        else if (cmd_len == 3 && strncmp(buf, "env", 3) == 0) cmd_env_show();
        else if (cmd_len == 3 && strncmp(buf, "set", 3) == 0) cmd_set_var(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "unset", 5) == 0) env_unset(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "mount", 5) == 0) cmd_mount_fs(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "umount", 6) == 0) {
            int r = sys_umount2(expanded_arg, 0);
            if (r == 0) { print(ANSI_GREEN "Unmounted\n" ANSI_RESET); }
            else { print(ANSI_RED "Failed\n" ANSI_RESET); }
        }
        else if (cmd_len == 7 && strncmp(buf, "history", 7) == 0) cmd_history();
        else if (cmd_len == 2 && strncmp(buf, "wc", 2) == 0) cmd_wc(expanded_arg);
        else if ((cmd_len == 5 && strncmp(buf, "dmesg", 5) == 0) || (cmd_len == 10 && strncmp(buf, "kernel_log", 10) == 0)) cmd_dmesg();
        else if ((cmd_len == 5 && strncmp(buf, "uname", 5) == 0) || (cmd_len == 11 && strncmp(buf, "system_info", 11) == 0)) cmd_uname();
        else if (cmd_len == 4 && strncmp(buf, "free", 4) == 0) cmd_free();
        else if ((cmd_len == 2 && strncmp(buf, "df", 2) == 0) || (cmd_len == 10 && strncmp(buf, "disk_space", 10) == 0)) cmd_df();
        else if (cmd_len == 2 && strncmp(buf, "id", 2) == 0) cmd_id();
        else if (cmd_len == 5 && strncmp(buf, "debug", 5) == 0) cmd_debug();
        else if (cmd_len == 3 && strncmp(buf, "log", 3) == 0) cmd_log();
        else if (cmd_len == 7 && strncmp(buf, "reboot", 7) == 0) {
            print(ANSI_YELLOW "Rebooting...\n" ANSI_RESET);
            sb_terminate(0);
        }
        else if (cmd_len == 8 && strncmp(buf, "shutdown", 8) == 0) {
            print(ANSI_YELLOW "Shutting down...\n" ANSI_RESET);
            sb_terminate(0);
        }
        else if (cmd_len == 2 && strncmp(buf, "ls", 2) == 0) cmd_list(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "rm", 2) == 0) cmd_delete(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "mkdir", 5) == 0) cmd_create_dir(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "rmdir", 5) == 0) cmd_remove_dir(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "head", 4) == 0) cmd_head(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "tail", 4) == 0) cmd_tail(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "grep", 4) == 0) cmd_grep(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "sleep", 5) == 0) cmd_sleep(expanded_arg);
        else if (cmd_len == 7 && strncmp(buf, "hexdump", 7) == 0) cmd_hexdump(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "base64", 6) == 0) cmd_base64(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "date", 4) == 0) cmd_date();
        else if (cmd_len == 15 && strncmp(buf, "set_permissions", 15) == 0) {
            char new_cmd[256] = "chmod "; strcpy(&new_cmd[6], expanded_arg);
            cmd_execute(new_cmd, background);
        }
        else if (cmd_len == 9 && strncmp(buf, "set_owner", 9) == 0) {
            char new_cmd[256] = "chown "; strcpy(&new_cmd[6], expanded_arg);
            cmd_execute(new_cmd, background);
        }
        else {
            // Try as executable
            cmd_execute(buf, background);
        }
    }
}
