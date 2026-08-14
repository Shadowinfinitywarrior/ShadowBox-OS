#include "sys.h"
#include "stat.h"
#include "uname.h"

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

// Split a command line into whitespace-separated tokens, honoring double quotes.
static int tokenize_quoted(char *line, char **argv, int max) {
    int argc = 0;
    char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (argc >= max) break;
        if (*p == '"') {
            p++;
            argv[argc] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') { *p = 0; p++; }
        } else {
            argv[argc] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) { *p = 0; p++; }
        }
        argc++;
    }
    return argc;
}

static int path_join(const char *dir, const char *name, char *out, int outsz) {
    int i = 0;
    while (*dir && i < outsz - 2) out[i++] = *dir++;
    if (i > 0 && out[i-1] != '/') {
        if (i >= outsz - 1) return 0;
        out[i++] = '/';
    }
    while (*name && i < outsz - 1) out[i++] = *name++;
    out[i] = 0;
    return 1;
}

// Search the PATH environment variable for an executable, trying both the bare
// name and the ".elf" suffix used by this system's userland programs.
static int find_program(const char *name, char *out, int outsz) {
    int has_slash = 0;
    for (const char *p = name; *p; p++) {
        if (*p == '/') { has_slash = 1; break; }
    }
    if (has_slash) {
        strcpy_n(out, name, outsz);
        return sys_access(out, 0) == 0;
    }
    const char *path = env_get("PATH");
    if (!path) path = "/";
    char dir[128];
    char tryp[256];
    const char *p = path;
    while (*p) {
        int di = 0;
        while (*p && *p != ':' && di < 127) dir[di++] = *p++;
        dir[di] = 0;
        if (*p == ':') p++;
        if (di == 0) continue;
        if (path_join(dir, name, tryp, 256) && sys_access(tryp, 0) == 0) {
            strcpy_n(out, tryp, outsz);
            return 1;
        }
        char named[192];
        int ni = 0;
        while (name[ni] && ni < 184) { named[ni] = name[ni]; ni++; }
        named[ni++] = '.'; named[ni++] = 'e'; named[ni++] = 'l'; named[ni++] = 'f';
        named[ni] = 0;
        if (path_join(dir, named, tryp, 256) && sys_access(tryp, 0) == 0) {
            strcpy_n(out, tryp, outsz);
            return 1;
        }
    }
    return 0;
}

// Build a full path from a base directory and entry name.
static void path_build(char *out, int outsz, const char *base, const char *name) {
    if (strcmp(base, ".") == 0) {
        strcpy_n(out, name, outsz);
        return;
    }
    path_join(base, name, out, outsz);
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
    "sort", "rev", "tr", "fold", "cut", "calc", "crc32", "find", "tree", "du",
    "alias", "unalias", "which", "type", "printf", "printenv", "rand", "bench",
    "pushd", "popd", "dirs", "sync", "reset", "yes", "basename", "dirname",
    "stat", "file", "banner", "sum",
    0
};

static void cmd_help(void) {
    print(ANSI_BOLD ANSI_CYAN "ShadowBox Shell v0.4.0 -- Commands:\n" ANSI_RESET);
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
    print("    ls -l [dir]          Long listing with sizes\n");
    print("    stat <path>          Show file/dir metadata\n");
    print("    file <path>          Detect file type\n");
    print("    crc32 <file>         CRC-32 checksum\n");
    print("    sum <file>           Simple 16-bit checksum\n");
    print("    du [dir]             Disk usage of a directory\n");
    print(ANSI_GREEN "  Text Utilities:\n" ANSI_RESET);
    print("    sort [file]          Sort lines (-r reverses)\n");
    print("    rev [file]           Reverse each line\n");
    print("    wc <file>            Count lines/words/bytes\n");
    print("    tr <set1> <set2>     Translate chars (stdin)\n");
    print("    fold [width] [file]  Wrap lines at a width\n");
    print("    cut <delim> <f> [f]  Extract fields (delim, field#)\n");
    print("    basename <path>      Strip directory from path\n");
    print("    dirname <path>       Print directory part of path\n");
    print("    calc <expr>          Evaluate arithmetic (3+5*2)\n");
    print(ANSI_GREEN "  Directory Operations:\n" ANSI_RESET);
    print("    mkdir / create_dir   Create directory\n");
    print("    rmdir / remove_dir   Remove empty directory\n");
    print("    change_dir <dir>     Change current directory\n");
    print("    pwd                  Print working directory\n");
    print("    tree [dir]           Show directory tree\n");
    print("    find <dir> <pat>     Find files matching pattern\n");
    print("    pushd <dir>          Push dir onto stack\n");
    print("    popd                 Pop dir from stack\n");
    print("    dirs                 Show dir stack\n");
    print(ANSI_GREEN "  Process Management:\n" ANSI_RESET);
    print("    execute <file>       Run a program (search PATH)\n");
    print("    processes / ps       List running processes\n");
    print("    stop <pid>           Kill a process\n");
    print("    sleep <sec>          Pause execution\n");
    print("    bench                Measure ~1s of CPU work\n");
    print("    uptime               Show system uptime\n");
    print(ANSI_GREEN "  System Info:\n" ANSI_RESET);
    print("    info                 Show system information\n");
    print("    memory               Show memory usage\n");
    print("    free                 Show memory (alias)\n");
    print("    uname                Show kernel name/version\n");
    print("    uname -a             Show full kernel identity\n");
    print("    df                   Show disk/filesystem info\n");
    print("    dmesg                Show kernel log\n");
    print("    date                 Show wall-clock date/time\n");
    print("    rand [max]           Print a random number\n");
    print(ANSI_GREEN "  Environment & Shell:\n" ANSI_RESET);
    print("    env / printenv       Show all variables\n");
    print("    set <name> <value>   Set environment variable\n");
    print("    unset <name>         Remove variable\n");
    print("    alias                List aliases\n");
    print("    alias n=v            Define alias\n");
    print("    unalias <name>       Remove alias\n");
    print("    which <program>      Locate a program\n");
    print("    type <name>          Describe a command\n");
    print("    printf <fmt> <args>  Formatted output\n");
    print("    echo [-n] <text>     Print text\n");
    print("    yes [text] [count]   Print repeated text\n");
    print("    banner <text>        Render big ASCII text\n");
    print("    history              Show command history\n");
    print(ANSI_GREEN "  Filesystem & System:\n" ANSI_RESET);
    print("    mount <type> <path>  Mount filesystem\n");
    print("    umount <path>        Unmount filesystem\n");
    print("    sync                 Flush filesystem\n");
    print("    reset                Reset terminal\n");
    print("    reboot               Reboot system\n");
    print("    exit                 Exit shell\n");
    print(ANSI_GREEN "  Debugging:\n" ANSI_RESET);
    print("    peek <addr> [len]    Read memory at address\n");
    print("    poke <addr> <val>    Write memory at address\n");
    print("    debug                Show debug info\n");
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

// Parse < input and > output redirections out of a command line and apply them
// to the shell's fd 0/1. Returns 0 on success (with a restore marker set), -1 on
// error after printing a message. The cleaned command line replaces buf.
static int setup_redirs(char *buf, int *save_in, int *save_out) {
    *save_in = -1;
    *save_out = -1;
    char *toks[64];
    int n = tokenize_quoted(buf, toks, 63);
    if (n <= 0) return 0;
    char out[512];
    int oi = 0;
    for (int i = 0; i < n; i++) {
        char t = toks[i][0];
        if (t == '<' || t == '>') {
            const char *fname = toks[i] + 1;
            if (!*fname) {
                if (i + 1 >= n) { print(ANSI_RED "redirection: missing file\n" ANSI_RESET); return -1; }
                fname = toks[++i];
            }
            if (t == '<') {
                int fd = sb_acquire(fname, 0);
                if (fd < 0) { print(ANSI_RED "redirection: cannot open " ANSI_RESET); print(fname); print("\n"); return -1; }
                *save_in = sys_dup(0);
                sys_dup2(fd, 0);
                sb_release(fd);
            } else {
                int fd = sb_acquire(fname, 0x40 | 0x1 | 0x200);
                if (fd < 0) { print(ANSI_RED "redirection: cannot create " ANSI_RESET); print(fname); print("\n"); return -1; }
                *save_out = sys_dup(1);
                sys_dup2(fd, 1);
                sb_release(fd);
            }
        } else {
            for (const char *q = toks[i]; *q && oi < 505; q++) out[oi++] = *q;
            out[oi++] = ' ';
        }
    }
    while (oi > 0 && out[oi - 1] == ' ') oi--;
    out[oi] = 0;
    strcpy_n(buf, out, 256);
    return 0;
}

static void restore_redirs(int save_in, int save_out) {
    if (save_out >= 0) { sys_dup2(save_out, 1); sb_release(save_out); }
    if (save_in >= 0) { sys_dup2(save_in, 0); sb_release(save_in); }
}

static void cmd_execute(const char *arg, int background) {
    if (!arg || !*arg) { print("Usage: execute <program> [args...]\n"); return; }
    char line[512];
    strcpy_n(line, arg, 512);
    char *argv[64];
    int argc = tokenize_quoted(line, argv, 63);
    if (argc < 1) return;
    char prog[256];
    if (!find_program(argv[0], prog, 256)) {
        print(ANSI_RED "Program not found: "); print(argv[0]); print("\n" ANSI_RESET);
        return;
    }
    argv[0] = prog;
    argv[argc] = 0;
    uint64_t pid = sb_replicate();
    if ((int64_t)pid == 0) {
        char *envp[] = {0};
        sb_morph(prog, argv, envp);
        print(ANSI_RED "Could not execute: "); print(prog); print("\n" ANSI_RESET);
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
    const char *path = ".";
    int long_fmt = 0, all_fmt = 0;
    if (arg && *arg) {
        if (arg[0] == '-') {
            for (const char *f = arg + 1; *f; f++) {
                if (*f == 'l') long_fmt = 1;
                if (*f == 'a') all_fmt = 1;
            }
        } else {
            path = arg;
        }
    }
    int fd = sb_acquire(path, 0);
    if (fd < 0) { print(ANSI_RED "Cannot open directory: "); print(path); print("\n" ANSI_RESET); return; }
    struct dirent entry;
    int count = 0;
    while (1) {
        int r = sys_getdents(fd, &entry, sizeof(struct dirent));
        if (r <= 0) break;
        if (!all_fmt && (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)) continue;
        if (long_fmt) {
            char full[300];
            path_build(full, 300, path, entry.name);
            struct stat st;
            if (syscall2(SYS_STAT, (uint64_t)full, (uint64_t)&st) == 0) {
                if ((st.st_mode & S_IFMT) == S_IFDIR) print(ANSI_BLUE "d" ANSI_RESET);
                else if ((st.st_mode & S_IFMT) == S_IFCHR) print(ANSI_YELLOW "c" ANSI_RESET);
                else print("-");
                print("  ");
                print_uint(st.st_size);
                print("  ");
            } else {
                print("?  ");
            }
            print(entry.name);
            if ((st.st_mode & S_IFMT) == S_IFDIR) print("/");
            print("\n");
        } else {
            print(ANSI_BLUE);
            print(entry.name);
            print(ANSI_RESET "  ");
            count++;
            if (count % 6 == 0) print("\n");
        }
    }
    if (!long_fmt && count % 6 != 0) print("\n");
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

static void update_pwd(const char *dir) {
    if (dir[0] == '/') {
        env_set("PWD", dir);
        return;
    }
    const char *cwd = env_get("PWD");
    if (!cwd) cwd = "/";
    char new_pwd[256];
    if (dir[0] == '.' && dir[1] == '.' && dir[2] == 0) {
        int i = 0; while (cwd[i]) { new_pwd[i] = cwd[i]; i++; } new_pwd[i] = 0;
        int len = i;
        while (len > 0 && new_pwd[len - 1] != '/') len--;
        if (len > 1) len--;
        new_pwd[len] = 0;
        env_set("PWD", new_pwd);
    } else if (dir[0] == '.' && dir[1] == 0) {
        /* stay in place */
    } else {
        int i = 0; while (cwd[i]) { new_pwd[i] = cwd[i]; i++; } new_pwd[i] = 0;
        if (i > 0 && new_pwd[i - 1] != '/') { new_pwd[i++] = '/'; new_pwd[i] = 0; }
        int j = 0; while (dir[j]) { new_pwd[i++] = dir[j++]; } new_pwd[i] = 0;
        env_set("PWD", new_pwd);
    }
}

static void cmd_change_dir(const char *arg) {
    if (!arg || !*arg) { print("Usage: change_dir <dir>\n"); return; }
    int r = sys_chdir(arg);
    if (r == 0) {
        update_pwd(arg);
    }
    else { print(ANSI_RED "Directory not found: "); print(arg); print("\n" ANSI_RESET); }
}

static void cmd_echo(const char *arg) {
    int newline = 1;
    if (arg && arg[0] == '-' && arg[1] == 'n') {
        newline = 0;
        arg += 2;
        while (*arg == ' ') arg++;
    }
    if (!arg) { print("\n"); return; }
    // Expand environment variables
    char expanded[512];
    expand_env(expanded, arg, 512);
    print(expanded);
    if (newline) print("\n");
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

static void cmd_uname(const char *arg) {
    if (arg && *arg == '-') {
        struct utsname u;
        if (syscall1(SYS_UNAME, (uint64_t)&u) == 0) {
            for (const char *f = arg + 1; *f; f++) {
                switch (*f) {
                    case 'a':
                        print(u.sysname); print(" "); print(u.nodename); print(" "); print(u.release);
                        print(" "); print(u.version); print(" "); print(u.machine); print("\n");
                        return;
                    case 's': print(u.sysname); print("\n"); break;
                    case 'n': print(u.nodename); print("\n"); break;
                    case 'r': print(u.release); print("\n"); break;
                    case 'v': print(u.version); print("\n"); break;
                    case 'm': print(u.machine); print("\n"); break;
                }
            }
            return;
        }
    }
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

static int is_leap_year(uint64_t y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static void print_uint_pad(uint64_t v, int width) {
    char buf[24];
    int idx = 0;
    if (v == 0) buf[idx++] = '0';
    uint64_t n = v;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    while (idx < width) buf[idx++] = '0';
    for (int i = idx - 1; i >= 0; i--) sb_push(1, &buf[i], 1);
}

static void cmd_date(void) {
    uint64_t tv[2] = {0, 0};
    if (sys_gettimeofday(tv) != 0 || tv[0] == 0) {
        uint64_t tbuf[2];
        uint64_t ticks = sys_times(tbuf);
        print("Ticks since boot: "); print_uint(ticks); print("\n");
        return;
    }
    uint64_t t = tv[0];
    static const char *wdays[] = {"Thu","Fri","Sat","Sun","Mon","Tue","Wed"};
    static const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                   "Jul","Aug","Sep","Oct","Nov","Dec"};
    static const uint8_t mdays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    uint64_t epoch_days = t / 86400;
    uint64_t rem = t % 86400;
    uint64_t h = rem / 3600; rem %= 3600;
    uint64_t mi = rem / 60;
    uint64_t s = rem % 60;
    uint64_t days = epoch_days, y = 1970;
    while (1) {
        uint64_t yd = is_leap_year(y) ? 366 : 365;
        if (days < yd) break;
        days -= yd;
        y++;
    }
    uint64_t m = 0;
    while (1) {
        uint64_t md = mdays[m];
        if (m == 1 && is_leap_year(y)) md++;
        if (days < md) break;
        days -= md;
        m++;
    }
    print(wdays[(epoch_days + 4) % 7]);
    print(" ");
    print(months[m]);
    print(" ");
    print_uint(days + 1);
    print(" ");
    print_uint_pad(h, 2); print(":");
    print_uint_pad(mi, 2); print(":");
    print_uint_pad(s, 2);
    print(" "); print_uint(y); print("\n");
}

// ---------------- Aliases ----------------
#define MAX_ALIAS 32
static char alias_names[MAX_ALIAS][32];
static char alias_vals[MAX_ALIAS][128];
static int alias_count = 0;

static const char *alias_get(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_names[i], name) == 0) return alias_vals[i];
    }
    return 0;
}

static void alias_set(const char *name, const char *val) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_names[i], name) == 0) {
            strcpy_n(alias_vals[i], val, 128);
            return;
        }
    }
    if (alias_count < MAX_ALIAS) {
        strcpy_n(alias_names[alias_count], name, 32);
        strcpy_n(alias_vals[alias_count], val, 128);
        alias_count++;
    }
}

static void cmd_alias(const char *arg) {
    if (!arg || !*arg) {
        print(ANSI_BOLD "Aliases:\n" ANSI_RESET);
        for (int i = 0; i < alias_count; i++) {
            print("alias "); print(alias_names[i]);
            print("='"); print(alias_vals[i]); print("'\n");
        }
        return;
    }
    const char *eq = 0;
    for (const char *p = arg; *p; p++) {
        if (*p == '=') { eq = p; break; }
    }
    if (eq) {
        char name[32]; int i = 0;
        while (arg + i < eq && i < 31) { name[i] = arg[i]; i++; }
        name[i] = 0;
        alias_set(name, eq + 1);
    } else {
        const char *v = alias_get(arg);
        if (v) { print("alias "); print(arg); print("='"); print(v); print("'\n"); }
        else { print(ANSI_RED "alias: "); print(arg); print(": not found\n" ANSI_RESET); }
    }
}

static void cmd_unalias(const char *arg) {
    if (!arg || !*arg) { print("Usage: unalias <name>\n"); return; }
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_names[i], arg) == 0) {
            for (int j = i; j < alias_count - 1; j++) {
                strcpy_n(alias_names[j], alias_names[j+1], 32);
                strcpy_n(alias_vals[j], alias_vals[j+1], 128);
            }
            alias_count--;
            return;
        }
    }
    print(ANSI_RED "unalias: "); print(arg); print(": not found\n" ANSI_RESET);
}

// ---------------- Directory stack ----------------
#define DIRS_MAX 16
static char dir_stack[DIRS_MAX][256];
static int dir_sp = 0;

static void cmd_dirs(void) {
    for (int i = 0; i < dir_sp; i++) {
        print_uint(i); print("  "); print(dir_stack[i]); print("\n");
    }
}

static void cmd_pushd(const char *arg) {
    if (!arg || !*arg) { print("Usage: pushd <dir>\n"); return; }
    if (dir_sp >= DIRS_MAX) { print(ANSI_RED "directory stack full\n" ANSI_RESET); return; }
    int r = sys_chdir(arg);
    if (r != 0) { print(ANSI_RED "Directory not found: "); print(arg); print("\n" ANSI_RESET); return; }
    strcpy_n(dir_stack[dir_sp++], arg, 256);
    update_pwd(arg);
    cmd_dirs();
}

static void cmd_popd(void) {
    if (dir_sp == 0) { print(ANSI_RED "directory stack empty\n" ANSI_RESET); return; }
    dir_sp--;
    const char *d = dir_stack[dir_sp];
    int r = sys_chdir(d);
    if (r != 0) { print(ANSI_RED "Cannot return to: "); print(d); print("\n" ANSI_RESET); return; }
    update_pwd(d);
    cmd_dirs();
}

// ---------------- text utilities ----------------
static int line_less(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a < (unsigned char)*b;
}

static int line_greater(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a > (unsigned char)*b;
}

static int substr_match(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (const char *h = hay; *h; h++) {
        const char *a = h, *b = needle;
        while (*a && *b && *a == *b) { a++; b++; }
        if (!*b) return 1;
    }
    return 0;
}

static void cmd_sort(const char *arg) {
    int reverse = 0;
    const char *path = 0;
    char line[256];
    strcpy_n(line, arg ? arg : "", 256);
    char *toks[8];
    int n = tokenize_quoted(line, toks, 7);
    for (int i = 0; i < n; i++) {
        if (strcmp(toks[i], "-r") == 0 || strcmp(toks[i], "-R") == 0) reverse = 1;
        else path = toks[i];
    }
    int fd = path ? sb_acquire(path, 0) : 0;
    if (fd < 0) { print(ANSI_RED "sort: cannot open "); print(path); print("\n" ANSI_RESET); return; }
    const uint64_t CHUNK = 32768;
    char *data = (char *)sys_sbrk(CHUNK);
    uint64_t cap = CHUNK, used = 0;
    for (;;) {
        if (used + 512 > cap) { sys_sbrk(CHUNK); cap += CHUNK; }
        int r = sb_pull(fd, data + used, 512);
        if (r <= 0) break;
        used += r;
    }
    if (fd != 0) sb_release(fd);
    if (used == 0) return;
    const uint64_t MAXL = 4096;
    char **lines = (char **)sys_sbrk(MAXL * sizeof(char *));
    uint64_t nl = 0;
    lines[nl++] = data;
    for (uint64_t i = 0; i < used && nl < MAXL; i++) {
        if (data[i] == '\n') {
            data[i] = 0;
            if (i + 1 < used) lines[nl++] = &data[i + 1];
        }
    }
    for (uint64_t i = 1; i < nl; i++) {
        char *key = lines[i];
        uint64_t j = i;
        while (j > 0 && (reverse ? line_greater(key, lines[j-1]) : line_less(key, lines[j-1]))) {
            lines[j] = lines[j - 1];
            j--;
        }
        lines[j] = key;
    }
    for (uint64_t i = 0; i < nl; i++) { print(lines[i]); print("\n"); }
}

static void cmd_rev(const char *arg) {
    const char *path = (arg && *arg) ? arg : 0;
    int fd = path ? sb_acquire(path, 0) : 0;
    if (fd < 0) { print(ANSI_RED "rev: cannot open "); print(path); print("\n" ANSI_RESET); return; }
    char cbuf[512];
    char lline[256];
    int len = 0;
    for (;;) {
        int r = sb_pull(fd, cbuf, 512);
        if (r <= 0) {
            for (int i = len - 1; i >= 0; i--) sb_push(1, &lline[i], 1);
            if (len) print("\n");
            break;
        }
        for (int i = 0; i < r; i++) {
            if (cbuf[i] == '\n') {
                for (int j = len - 1; j >= 0; j--) sb_push(1, &lline[j], 1);
                print("\n");
                len = 0;
            } else if (len < 255) {
                lline[len++] = cbuf[i];
            }
        }
    }
    if (fd != 0) sb_release(fd);
}

static void cmd_tr(const char *arg) {
    if (!arg || !*arg) { print("Usage: tr <set1> <set2>\n  Reads stdin, translates characters.\n"); return; }
    char line[256];
    strcpy_n(line, arg, 256);
    char *toks[4];
    int n = tokenize_quoted(line, toks, 3);
    if (n < 2) { print("Usage: tr <set1> <set2>\n"); return; }
    const char *s1 = toks[0], *s2 = toks[1];
    size_t l1 = strlen(s1), l2 = strlen(s2);
    char buf[512];
    for (;;) {
        int r = sb_pull(0, buf, 512);
        if (r <= 0) break;
        for (int i = 0; i < r; i++) {
            char c = buf[i];
            for (size_t j = 0; j < l1; j++) {
                if (s1[j] == c) { c = (j < l2) ? s2[j] : s2[l2 - 1]; break; }
            }
            buf[i] = c;
        }
        sb_push(1, buf, r);
    }
}

static void cmd_fold(const char *arg) {
    int width = 80;
    const char *path = 0;
    if (arg && *arg) {
        char line[256];
        strcpy_n(line, arg, 256);
        char *toks[4];
        int n = tokenize_quoted(line, toks, 3);
        if (n >= 1) {
            int first_is_num = 1;
            for (const char *q = toks[0]; *q; q++) {
                if (!(*q >= '0' && *q <= '9')) { first_is_num = 0; break; }
            }
            if (first_is_num) {
                width = atoi(toks[0]);
                if (n >= 2) path = toks[1];
            } else {
                path = toks[0];
            }
        }
    }
    if (width <= 0) width = 80;
    int fd = path ? sb_acquire(path, 0) : 0;
    if (fd < 0) { print(ANSI_RED "fold: cannot open "); print(path); print("\n" ANSI_RESET); return; }
    char cbuf[512];
    char out[256];
    int olen = 0;
    for (;;) {
        int r = sb_pull(fd, cbuf, 512);
        if (r <= 0) {
            if (olen) { sb_push(1, out, olen); print("\n"); }
            break;
        }
        for (int i = 0; i < r; i++) {
            if (cbuf[i] == '\n' || olen >= width) {
                if (cbuf[i] == '\n' && olen == 0) { print("\n"); continue; }
                if (olen) { sb_push(1, out, olen); print("\n"); olen = 0; }
                if (cbuf[i] == '\n') continue;
            }
            out[olen++] = cbuf[i];
        }
    }
    if (fd != 0) sb_release(fd);
}

static void cmd_cut(const char *arg) {
    if (!arg || !*arg) { print("Usage: cut <delimiter> <field> [file]\n"); return; }
    char line[256];
    strcpy_n(line, arg, 256);
    char *toks[8];
    int n = tokenize_quoted(line, toks, 7);
    if (n < 2) { print("Usage: cut <delimiter> <field> [file]\n"); return; }
    const char *delim = toks[0];
    int field = atoi(toks[1]);
    const char *path = (n >= 3) ? toks[2] : 0;
    int fd = path ? sb_acquire(path, 0) : 0;
    if (fd < 0) { print(ANSI_RED "cut: cannot open "); print(path); print("\n" ANSI_RESET); return; }
    char cbuf[512];
    char lbuf[256];
    int len = 0;
    for (;;) {
        int r = sb_pull(fd, cbuf, 512);
        if (r <= 0) { len = 0; break; }
        for (int i = 0; i < r; i++) {
            if (cbuf[i] == '\n') {
                lbuf[len] = 0;
                int fi = 1;
                char *start = lbuf;
                char *end = 0;
                for (char *q = lbuf;; q++) {
                    if (*q == delim[0] || *q == 0) {
                        if (fi == field) { end = q; break; }
                        fi++;
                        start = q + 1;
                    }
                    if (*q == 0) break;
                }
                if (end) sb_push(1, start, end - start);
                print("\n");
                len = 0;
            } else if (len < 255) {
                lbuf[len++] = cbuf[i];
            }
        }
    }
    if (fd != 0) sb_release(fd);
}

static void cmd_basename(const char *arg) {
    if (!arg || !*arg) { print("Usage: basename <path> [suffix]\n"); return; }
    char line[256];
    strcpy_n(line, arg, 256);
    char *toks[4];
    int n = tokenize_quoted(line, toks, 3);
    const char *p = toks[0];
    int len = 0;
    while (p[len]) len++;
    while (len > 1 && p[len - 1] == '/') len--;
    int start = 0;
    for (int i = 0; i < len; i++) {
        if (p[i] == '/') start = i + 1;
    }
    char out[256];
    int oi = 0;
    for (int i = start; i < len; i++) out[oi++] = p[i];
    out[oi] = 0;
    if (n >= 2) {
        int sl = 0;
        while (toks[1][sl]) sl++;
        if (sl > 0 && oi >= sl && strncmp(out + oi - sl, toks[1], sl) == 0) {
            oi -= sl;
            out[oi] = 0;
        }
    }
    print(out);
    print("\n");
}

static void cmd_dirname(const char *arg) {
    if (!arg || !*arg) { print("Usage: dirname <path>\n"); return; }
    const char *p = arg;
    int len = 0;
    while (p[len]) len++;
    while (len > 1 && p[len - 1] == '/') len--;
    int last_slash = -1;
    for (int i = 0; i < len; i++) {
        if (p[i] == '/') last_slash = i;
    }
    if (last_slash < 0) { print(".\n"); return; }
    if (last_slash == 0) { print("/\n"); return; }
    print_n(p, last_slash);
    print("\n");
}

// ---------------- filesystem introspection ----------------
static void cmd_stat(const char *arg) {
    if (!arg || !*arg) { print("Usage: stat <path>\n"); return; }
    struct stat st;
    if (syscall2(SYS_STAT, (uint64_t)arg, (uint64_t)&st) != 0) {
        print(ANSI_RED "stat: cannot stat "); print(arg); print("\n" ANSI_RESET);
        return;
    }
    print("  File: "); print(arg); print("\n");
    print("  Size: "); print_uint(st.st_size); print(" bytes  ");
    uint32_t t = st.st_mode & S_IFMT;
    if (t == S_IFDIR) print("Type: directory");
    else if (t == S_IFCHR) print("Type: character device");
    else print("Type: regular file");
    print("\n");
    print("  Mode: 0"); print_hex(st.st_mode, 4);
    print("  UID: "); print_uint(st.st_uid);
    print("  GID: "); print_uint(st.st_gid);
    print("\n");
}

static void cmd_file(const char *arg) {
    if (!arg || !*arg) { print("Usage: file <path>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "file: cannot open "); print(arg); print("\n" ANSI_RESET); return; }
    unsigned char h[320];
    int r = sb_pull(fd, h, 320);
    if (r < 0) r = 0;
    sb_release(fd);
    print(arg); print(": ");
    if (r >= 4 && h[0] == 0x7f && h[1] == 'E' && h[2] == 'L' && h[3] == 'F') {
        print("ELF executable");
    } else if (r >= 262 && h[257] == 'u' && h[258] == 's' && h[259] == 't' && h[260] == 'a' && h[261] == 'r') {
        print("POSIX tar archive");
    } else if (r >= 2 && h[0] == 'B' && h[1] == 'M') {
        print("BMP image");
    } else if (r >= 2 && h[0] == 'P' && h[1] >= '1' && h[1] <= '7') {
        print("netpbm image");
    } else if (r >= 2 && h[0] == 'P' && h[1] == 'K') {
        print("zip archive");
    } else {
        int text = 1;
        for (int i = 0; i < r; i++) {
            unsigned char c = h[i];
            if (c == 0 || c > 126) { text = 0; break; }
        }
        if (text && r > 0) print("ASCII text");
        else print("data");
    }
    print("\n");
}

static void find_rec(const char *base, const char *pattern) {
    int fd = sb_acquire(base, 0);
    if (fd < 0) return;
    struct dirent de;
    while (sys_getdents(fd, &de, sizeof(de)) > 0) {
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        char full[512];
        path_build(full, 512, base, de.name);
        if (substr_match(de.name, pattern)) { print(full); print("\n"); }
        int subfd = sb_acquire(full, 0);
        int isdir = 0;
        if (subfd >= 0) {
            struct dirent t;
            isdir = sys_getdents(subfd, &t, sizeof(t)) > 0;
            sb_release(subfd);
        }
        if (isdir) find_rec(full, pattern);
    }
    sb_release(fd);
}

static void cmd_find(const char *arg) {
    if (!arg || !*arg) { print("Usage: find <path> <pattern>\n"); return; }
    char line[256];
    strcpy_n(line, arg, 256);
    char *toks[8];
    int n = tokenize_quoted(line, toks, 7);
    const char *pattern = toks[0];
    const char *path = (n >= 2) ? toks[1] : ".";
    find_rec(path, pattern);
}

static void tree_rec(const char *base, int depth) {
    if (depth > 4) return;
    int fd = sb_acquire(base, 0);
    if (fd < 0) return;
    struct dirent de;
    while (sys_getdents(fd, &de, sizeof(de)) > 0) {
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        for (int i = 0; i < depth; i++) print("  ");
        char full[512];
        path_build(full, 512, base, de.name);
        int subfd = sb_acquire(full, 0);
        int isdir = 0;
        if (subfd >= 0) {
            struct dirent t;
            isdir = sys_getdents(subfd, &t, sizeof(t)) > 0;
            sb_release(subfd);
        }
        if (isdir) {
            print(ANSI_BLUE); print(de.name); print("/\n" ANSI_RESET);
            tree_rec(full, depth + 1);
        } else {
            print(de.name); print("\n");
        }
    }
    sb_release(fd);
}

static void cmd_tree(const char *arg) {
    const char *path = (arg && *arg) ? arg : ".";
    print(path);
    print("\n");
    tree_rec(path, 1);
}

static uint64_t du_rec(const char *base) {
    uint64_t sum = 0;
    int fd = sb_acquire(base, 0);
    if (fd < 0) {
        struct stat st;
        if (syscall2(SYS_STAT, (uint64_t)base, (uint64_t)&st) == 0) return st.st_size;
        return 0;
    }
    struct dirent de;
    while (sys_getdents(fd, &de, sizeof(de)) > 0) {
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        char full[512];
        path_build(full, 512, base, de.name);
        int subfd = sb_acquire(full, 0);
        int isdir = 0;
        if (subfd >= 0) {
            struct dirent t;
            isdir = sys_getdents(subfd, &t, sizeof(t)) > 0;
            sb_release(subfd);
        }
        if (isdir) sum += du_rec(full);
        else {
            struct stat st;
            if (syscall2(SYS_STAT, (uint64_t)full, (uint64_t)&st) == 0) sum += st.st_size;
        }
    }
    sb_release(fd);
    return sum;
}

static void cmd_du(const char *arg) {
    const char *path = (arg && *arg) ? arg : ".";
    uint64_t sz = du_rec(path);
    print_uint(sz); print("\t"); print(path); print("\n");
}

// ---------------- command lookup ----------------
static void cmd_which(const char *arg) {
    if (!arg || !*arg) { print("Usage: which <program> [program...]\n"); return; }
    char line[256];
    strcpy_n(line, arg, 256);
    char *toks[16];
    int n = tokenize_quoted(line, toks, 15);
    for (int i = 0; i < n; i++) {
        char path[256];
        if (find_program(toks[i], path, 256)) { print(path); print("\n"); }
        else { print(toks[i]); print(": not found\n"); }
    }
}

static void cmd_type(const char *arg) {
    if (!arg || !*arg) { print("Usage: type <name>\n"); return; }
    for (int i = 0; commands[i]; i++) {
        if (strcmp(commands[i], arg) == 0) {
            print(arg); print(" is a shell builtin\n");
            return;
        }
    }
    char path[256];
    if (find_program(arg, path, 256)) {
        print(arg); print(" is "); print(path); print("\n");
        return;
    }
    print(arg); print(": not found\n");
}

// ---------------- environment ----------------
static void cmd_printenv(const char *arg) {
    if (arg && *arg) {
        const char *v = env_get(arg);
        if (v) { print(v); print("\n"); }
        return;
    }
    for (int i = 0; i < env_count; i++) {
        print(env_names[i]); print("="); print(env_vals[i]); print("\n");
    }
}

// ---------------- printf ----------------
static void uint_to_str(uint64_t val, char *buf) {
    int idx = 0;
    if (val == 0) buf[idx++] = '0';
    uint64_t n = val;
    while (n > 0) { buf[idx++] = '0' + (n % 10); n /= 10; }
    for (int i = 0; i < idx / 2; i++) {
        char t = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = t;
    }
    buf[idx] = 0;
}

static void cmd_printf(const char *arg) {
    if (!arg || !*arg) { print("\n"); return; }
    char line[512];
    strcpy_n(line, arg, 512);
    char *toks[32];
    int n = tokenize_quoted(line, toks, 31);
    if (n == 0) return;
    const char *fmt = toks[0];
    int ai = 1;
    char out[512];
    int oi = 0;
    for (const char *p = fmt; *p && oi < 510; p++) {
        if (*p == '\\' && *(p + 1)) {
            p++;
            if (*p == 'n') out[oi++] = '\n';
            else if (*p == 't') out[oi++] = '\t';
            else if (*p == 'r') out[oi++] = '\r';
            else if (*p == '\\') out[oi++] = '\\';
            else out[oi++] = *p;
            continue;
        }
        if (*p == '%' && *(p + 1)) {
            p++;
            if (*p == '%') { out[oi++] = '%'; continue; }
            if (ai >= n) break;
            const char *a = toks[ai++];
            if (*p == 's') {
                for (const char *q = a; *q && oi < 510; q++) out[oi++] = *q;
            } else if (*p == 'd' || *p == 'i') {
                long v = (long)atoi(a);
                char nb[24];
                if (v < 0) { out[oi++] = '-'; uint_to_str((uint64_t)(-(long long)v), nb); }
                else uint_to_str((uint64_t)v, nb);
                for (char *q = nb; *q && oi < 510; q++) out[oi++] = *q;
            } else if (*p == 'u') {
                char nb[24];
                uint_to_str((uint64_t)((unsigned long)atoi(a)), nb);
                for (char *q = nb; *q && oi < 510; q++) out[oi++] = *q;
            } else if (*p == 'x' || *p == 'X') {
                uint32_t v = (uint32_t)atoi(a);
                char nb[12];
                int idx = 0;
                if (v == 0) nb[idx++] = '0';
                while (v) {
                    int d = v & 0xF;
                    nb[idx++] = (d < 10) ? '0' + d : (*p == 'X' ? 'A' : 'a') + d - 10;
                    v >>= 4;
                }
                for (int i = idx - 1; i >= 0; i--) out[oi++] = nb[i];
            } else if (*p == 'c') {
                out[oi++] = a[0];
            } else {
                out[oi++] = '%';
                out[oi++] = *p;
            }
        } else {
            out[oi++] = *p;
        }
    }
    sb_push(1, out, oi);
}

// ---------------- misc ----------------
static void cmd_print_int(int64_t v) {
    if (v < 0) {
        print("-");
        print_uint((uint64_t)(-(long long)v));
    } else {
        print_uint((uint64_t)v);
    }
}

static const char *expr_p;
static int expr_err;

static int64_t expr_add(void);
static int64_t expr_term(void);
static int64_t expr_factor(void);

static int64_t expr_factor(void) {
    while (*expr_p == ' ' || *expr_p == '\t') expr_p++;
    int64_t val;
    if (*expr_p == '(') {
        expr_p++;
        val = expr_add();
        while (*expr_p == ' ' || *expr_p == '\t') expr_p++;
        if (*expr_p == ')') expr_p++;
        else expr_err = 1;
        return val;
    }
    if (*expr_p == '-') { expr_p++; return -expr_factor(); }
    if (*expr_p >= '0' && *expr_p <= '9') {
        val = 0;
        while (*expr_p >= '0' && *expr_p <= '9') {
            val = val * 10 + (*expr_p - '0');
            expr_p++;
        }
        return val;
    }
    expr_err = 1;
    return 0;
}

static int64_t expr_term(void) {
    int64_t a = expr_factor();
    for (;;) {
        while (*expr_p == ' ' || *expr_p == '\t') expr_p++;
        char op = *expr_p;
        if (op == '*' || op == '/' || op == '%') {
            expr_p++;
            int64_t b = expr_factor();
            if (op == '*') a = a * b;
            else if (op == '/') a = b ? a / b : 0;
            else a = b ? a % b : 0;
        } else {
            return a;
        }
    }
}

static int64_t expr_add(void) {
    int64_t a = expr_term();
    for (;;) {
        while (*expr_p == ' ' || *expr_p == '\t') expr_p++;
        char op = *expr_p;
        if (op == '+' || op == '-') {
            expr_p++;
            int64_t b = expr_term();
            a = (op == '+') ? a + b : a - b;
        } else {
            return a;
        }
    }
}

static void cmd_calc(const char *arg) {
    if (!arg || !*arg) { print("Usage: calc <expression>  e.g. calc (3+5)*2-7\n"); return; }
    expr_p = arg;
    expr_err = 0;
    int64_t r = expr_add();
    if (expr_err) { print(ANSI_RED "calc: syntax error\n" ANSI_RESET); return; }
    cmd_print_int(r);
    print("\n");
}

static uint32_t crc_tab[256];
static int crc_tab_ready = 0;

static void crc_tab_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : (c >> 1);
        crc_tab[i] = c;
    }
    crc_tab_ready = 1;
}

static void cmd_crc32(const char *arg) {
    if (!arg || !*arg) { print("Usage: crc32 <file>\n"); return; }
    if (!crc_tab_ready) crc_tab_init();
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "crc32: cannot open "); print(arg); print("\n" ANSI_RESET); return; }
    uint32_t crc = 0xFFFFFFFFu;
    char buf[512];
    for (;;) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        for (int i = 0; i < r; i++) {
            crc = (crc >> 8) ^ crc_tab[(crc ^ (uint8_t)buf[i]) & 0xFF];
        }
    }
    sb_release(fd);
    crc ^= 0xFFFFFFFFu;
    print_hex(crc, 8); print("  "); print(arg); print("\n");
}

static void cmd_sum(const char *arg) {
    if (!arg || !*arg) { print("Usage: sum <file>\n"); return; }
    int fd = sb_acquire(arg, 0);
    if (fd < 0) { print(ANSI_RED "sum: cannot open "); print(arg); print("\n" ANSI_RESET); return; }
    uint32_t sum = 0;
    uint64_t blocks = 0;
    char buf[512];
    for (;;) {
        int r = sb_pull(fd, buf, 512);
        if (r <= 0) break;
        for (int i = 0; i < r; i++) sum = (sum + (uint8_t)buf[i]) % 65536;
        blocks += (uint64_t)r / 512;
        if (r % 512) blocks++;
    }
    sb_release(fd);
    print_uint(sum);
    print("  ");
    print_uint(blocks);
    print("  ");
    print(arg);
    print("\n");
}

static void cmd_sync(void) {
    if (sys_sync() == 0) print("synced\n");
    else print(ANSI_RED "sync failed\n" ANSI_RESET);
}

static void cmd_reset(void) {
    print("\033c\033[0m\033[2J\033[H");
}

static uint64_t rng_state = 0;

static uint64_t xrand(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void cmd_rand(const char *arg) {
    if (!rng_state) {
        rng_state = sys_time() * 2654435761u ^ (sys_getpid() << 21) ^ (uint64_t)&rng_state;
        if (!rng_state) rng_state = 0x9E3779B97F4A7C15u;
    }
    int max = (arg && *arg) ? atoi(arg) : 0;
    uint64_t v = xrand();
    if (max > 0) v %= (uint64_t)max;
    print_uint(v); print("\n");
}

static void cmd_bench(const char *arg) {
    uint64_t target = (arg && *arg) ? (uint64_t)atoi(arg) : 0;
    uint64_t t[2];
    sys_times(t);
    uint64_t hz = t[1] ? t[1] : 100;
    uint64_t start = t[0];
    volatile uint64_t count = 0;
    while (1) {
        for (int i = 0; i < 4096; i++) count++;
        uint64_t now[2];
        sys_times(now);
        if (now[0] - start >= hz) break;
        if (target && count >= target) break;
    }
    print_uint(count);
    print(" iterations in ~1 second\n");
}

static void cmd_yes(const char *arg) {
    char line[256];
    strcpy_n(line, arg ? arg : "", 256);
    char *toks[8];
    int n = tokenize_quoted(line, toks, 7);
    const char *word = (n > 0 && toks[0][0]) ? toks[0] : "y";
    uint64_t max = 100;
    if (n > 1) max = (uint64_t)atoi(toks[1]);
    if (max == 0) max = 100;
    uint64_t wl = strlen(word);
    for (uint64_t i = 0; i < max; i++) {
        sb_push(1, word, wl);
        print("\n");
    }
}

// ---------------- banner ----------------
static const char *const bglyphs[41][5] = {
  /* A  */ {" ### ","#   #","#####","#   #","#   #"},
  /* B  */ {"#### ","#   #","#### ","#   #","#### "},
  /* C  */ {" ####","#    ","#    ","#    "," ####"},
  /* D  */ {"#### ","#   #","#   #","#   #","#### "},
  /* E  */ {"#####","#    ","#### ","#    ","#####"},
  /* F  */ {"#####","#    ","#### ","#    ","#    "},
  /* G  */ {" ####","#    ","# ###","#   #"," ####"},
  /* H  */ {"#   #","#   #","#####","#   #","#   #"},
  /* I  */ {"#####","  #  ","  #  ","  #  ","#####"},
  /* J  */ {"  ###","   # ","   # ","#  # "," ##  "},
  /* K  */ {"#   #","#  # ","###  ","#  # ","#   #"},
  /* L  */ {"#    ","#    ","#    ","#    ","#####"},
  /* M  */ {"#   #","## ##","# # #","#   #","#   #"},
  /* N  */ {"#   #","##  #","# # #","#  ##","#   #"},
  /* O  */ {" ### ","#   #","#   #","#   #"," ### "},
  /* P  */ {"#### ","#   #","#### ","#    ","#    "},
  /* Q  */ {" ### ","#   #","#   #","#  # "," ## #"},
  /* R  */ {"#### ","#   #","#### ","#  # ","#   #"},
  /* S  */ {" ####","#    "," ### ","    #","#### "},
  /* T  */ {"#####","  #  ","  #  ","  #  ","  #  "},
  /* U  */ {"#   #","#   #","#   #","#   #"," ### "},
  /* V  */ {"#   #","#   #","#   #"," # # ","  #  "},
  /* W  */ {"#   #","#   #","# # #","## ##","#   #"},
  /* X  */ {"#   #"," # # ","  #  "," # # ","#   #"},
  /* Y  */ {"#   #"," # # ","  #  ","  #  ","  #  "},
  /* Z  */ {"#####","   # ","  #  "," #   ","#####"},
  /* 0  */ {" ### ","#   #","#   #","#   #"," ### "},
  /* 1  */ {"  #  "," ##  ","  #  ","  #  ","#####"},
  /* 2  */ {" ### ","#   #","  ## "," #   ","#####"},
  /* 3  */ {"#### ","    #"," ### ","    #","#### "},
  /* 4  */ {"  ## "," # # ","#####","   # ","   # "},
  /* 5  */ {"#####","#    ","#### ","    #","#### "},
  /* 6  */ {" ### ","#    ","#### ","#   #"," ### "},
  /* 7  */ {"#####","   # ","  #  "," #   "," #   "},
  /* 8  */ {" ### ","#   #"," ### ","#   #"," ### "},
  /* 9  */ {" ### ","#   #"," ####","    #"," ### "},
  /* sp */ {"     ","     ","     ","     ","     "},
  /* !  */ {"  #  ","  #  ","  #  ","     ","  #  "},
  /* ?  */ {" ### ","#   #","  ## ","     ","  #  "},
  /* .  */ {"     ","     ","     "," ##  "," ##  "},
  /* -  */ {"     ","     ","#####","     ","     "},
};

static int glyph_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    if (c == ' ') return 36;
    if (c == '!') return 37;
    if (c == '?') return 38;
    if (c == '.') return 39;
    if (c == '-') return 40;
    return 36;
}

static void cmd_banner(const char *arg) {
    if (!arg || !*arg) { print("Usage: banner <text>\n"); return; }
    for (int row = 0; row < 5; row++) {
        for (const char *p = arg; *p; p++) {
            print(bglyphs[glyph_index(*p)][row]);
        }
        print("\n");
    }
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
        char line[256];
        strcpy_n(line, cmd1, 256);
        char *argv[32];
        int argc = tokenize_quoted(line, argv, 31);
        char prog[256];
        if (argc < 1 || !find_program(argv[0], prog, 256)) {
            print(ANSI_RED "exec failed: "); print(cmd1); print("\n" ANSI_RESET);
            sb_terminate(127);
        }
        argv[0] = prog;
        argv[argc] = 0;
        sb_morph(prog, argv, 0);
        print(ANSI_RED "exec failed: "); print(cmd1); print("\n" ANSI_RESET);
        sb_terminate(127);
    }

    uint64_t pid2 = sb_replicate();
    if ((int64_t)pid2 == 0) {
        sys_dup2(pipefd[0], 0);
        sb_release(pipefd[0]);
        sb_release(pipefd[1]);
        char line[256];
        strcpy_n(line, cmd2, 256);
        char *argv[32];
        int argc = tokenize_quoted(line, argv, 31);
        char prog[256];
        if (argc < 1 || !find_program(argv[0], prog, 256)) {
            print(ANSI_RED "exec failed: "); print(cmd2); print("\n" ANSI_RESET);
            sb_terminate(127);
        }
        argv[0] = prog;
        argv[argc] = 0;
        sb_morph(prog, argv, 0);
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

        // Alias expansion (single level)
        {
            int wl = 0;
            while (buf[wl] && buf[wl] != ' ' && buf[wl] != '\t') wl++;
            if (wl > 0 && wl < 63) {
                char w[64];
                for (int q = 0; q < wl; q++) w[q] = buf[q];
                w[wl] = 0;
                const char *av = alias_get(w);
                if (av) {
                    char nb[300];
                    int nbi = 0;
                    for (const char *s = av; *s && nbi < 260; s++) nb[nbi++] = *s;
                    if (buf[wl]) {
                        if (nbi && nb[nbi - 1] != ' ') nb[nbi++] = ' ';
                        for (int q = wl; buf[q] && nbi < 290; q++) nb[nbi++] = buf[q];
                    }
                    nb[nbi] = 0;
                    strcpy(buf, nb);
                    i = nbi;
                }
            }
        }

        // Check for pipe
        int pipe_pos = -1;
        for (int p = 0; buf[p]; p++) {
            if (buf[p] == '|') { pipe_pos = p; break; }
        }

        int save_in = -1, save_out = -1;
        int redirected = 0;
        if (pipe_pos < 0) {
            // Apply < / > redirections for builtins and external programs.
            // (Pipe handling overrides the std fds for the pipeline children.)
            if (setup_redirs(buf, &save_in, &save_out) < 0) continue;
            redirected = (save_in >= 0 || save_out >= 0);
            pipe_pos = -1;
            for (int p = 0; buf[p]; p++) {
                if (buf[p] == '|') { pipe_pos = p; break; }
            }
        }

        if (pipe_pos >= 0) {
            buf[pipe_pos] = '\0';
            const char *left = buf;
            const char *right = buf + pipe_pos + 1;
            while (*left == ' ') left++;
            while (*right == ' ') right++;
            run_piped(left, right);
            if (redirected) restore_redirs(save_in, save_out);
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
        else if ((cmd_len == 5 && strncmp(buf, "uname", 5) == 0) || (cmd_len == 11 && strncmp(buf, "system_info", 11) == 0)) cmd_uname(expanded_arg);
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
        else if (cmd_len == 4 && strncmp(buf, "sort", 4) == 0) cmd_sort(expanded_arg);
        else if (cmd_len == 3 && strncmp(buf, "rev", 3) == 0) cmd_rev(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "tr", 2) == 0) cmd_tr(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "fold", 4) == 0) cmd_fold(expanded_arg);
        else if (cmd_len == 3 && strncmp(buf, "cut", 3) == 0) cmd_cut(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "calc", 4) == 0) cmd_calc(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "crc32", 5) == 0) cmd_crc32(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "find", 4) == 0) cmd_find(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "tree", 4) == 0) cmd_tree(expanded_arg);
        else if (cmd_len == 2 && strncmp(buf, "du", 2) == 0) cmd_du(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "alias", 5) == 0) cmd_alias(expanded_arg);
        else if (cmd_len == 7 && strncmp(buf, "unalias", 7) == 0) cmd_unalias(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "which", 5) == 0) cmd_which(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "type", 4) == 0) cmd_type(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "printf", 6) == 0) cmd_printf(expanded_arg);
        else if (cmd_len == 8 && strncmp(buf, "printenv", 8) == 0) cmd_printenv(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "rand", 4) == 0) cmd_rand(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "bench", 5) == 0) cmd_bench(expanded_arg);
        else if (cmd_len == 5 && strncmp(buf, "pushd", 5) == 0) cmd_pushd(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "popd", 4) == 0) cmd_popd();
        else if (cmd_len == 4 && strncmp(buf, "dirs", 4) == 0) cmd_dirs();
        else if (cmd_len == 4 && strncmp(buf, "sync", 4) == 0) cmd_sync();
        else if (cmd_len == 5 && strncmp(buf, "reset", 5) == 0) cmd_reset();
        else if (cmd_len == 3 && strncmp(buf, "yes", 3) == 0) cmd_yes(expanded_arg);
        else if (cmd_len == 8 && strncmp(buf, "basename", 8) == 0) cmd_basename(expanded_arg);
        else if (cmd_len == 7 && strncmp(buf, "dirname", 7) == 0) cmd_dirname(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "stat", 4) == 0) cmd_stat(expanded_arg);
        else if (cmd_len == 4 && strncmp(buf, "file", 4) == 0) cmd_file(expanded_arg);
        else if (cmd_len == 6 && strncmp(buf, "banner", 6) == 0) cmd_banner(expanded_arg);
        else if (cmd_len == 3 && strncmp(buf, "sum", 3) == 0) cmd_sum(expanded_arg);
        else {
            // Try as executable
            cmd_execute(buf, background);
        }
        if (redirected) restore_redirs(save_in, save_out);
    }
}
