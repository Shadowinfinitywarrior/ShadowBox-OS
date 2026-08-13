// Enhanced Scientific Calculator for ShadowBox OS
// Features: Trigonometry, logarithms, memory, history, constants
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/calculator_scientific.c -o calculator_scientific.elf
//
// Supported operations:
//   Basic: +, -, *, /, ^ (power)
//   Constants: pi, e
//   Memory: m= (store), m (recall), mc (clear)
//   History: h (show), !N (recall Nth)
//   Functions: sqrt, abs
// Note: Trig and log functions removed due to SSE restrictions

#include "sys.h"

#define HIST_MAX 32

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

static void print_uint(uint64_t val) {
    char buf[24]; int idx = 0;
    if (val == 0) buf[idx++] = '0';
    while (val > 0) {
        buf[idx++] = '0' + (val % 10);
        val /= 10;
    }
    while (idx--) {
        sb_push(1, &buf[idx], 1);
    }
}

static void print_int(int64_t val) {
    if (val < 0) {
        char c = '-';
        sb_push(1, &c, 1);
        val = -val;
    }
    print_uint((uint64_t)val);
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static void parse_int(const char *s, const char **endptr, int64_t *out) {
    int64_t result = 0;
    int sign = 1;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        s++;
    }
    *endptr = s;
    *out = sign * result;
}

static void pow_int(int64_t base, uint64_t exp, int64_t *out) {
    int64_t result = 1;
    while (exp) {
        if (exp & 1ULL) result *= base;
        base *= base;
        exp >>= 1ULL;
    }
    *out = result;
}

static int64_t sqrt_int(int64_t x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    int64_t guess = x;
    int64_t prev;
    do {
        prev = guess;
        guess = (guess + x / guess) / 2;
    } while (guess < prev);
    return prev;
}

// Memory and history
static int64_t memory = 0;
static int has_memory = 0;
static int64_t history[HIST_MAX];
static int hist_count = 0;

void _start(void) {
    print("\033[2J\033[H"); // Clear screen
    print("\033[1;36m");
    print("=== Enhanced Scientific Calculator ===\033[0m\n");
    print("Operations: +, -, *, /, ^ (power)\n");
    print("Functions: sqrt, abs\n");
    print("Constants: pi, e (as integers)\n");
    print("Memory: m= (store), m (recall), mc (clear)\n");
    print("History: h (show), !N (recall Nth)\n");
    print("Type 'q' to quit\n\n");
    
    while (1) {
        print("> ");
        char buf[128];
        int len = 0;
        while (len < 127) {
            char c;
            if (sb_pull(0, &c, 1) <= 0) break;
            if (c == '\n' || c == '\r') { print("\n"); break; }
            if (c == '\b' || c == 127) {
                if (len > 0) { print("\b \b"); len--; }
            } else {
                sb_push(1, &c, 1);
                buf[len++] = c;
            }
        }
        buf[len] = '\0';
        
        if (buf[0] == 'q' || buf[0] == 'Q') break;
        if (len == 0) continue;
        
        // Handle special commands
        if (strcmp(buf, "h") == 0) {
            print("History:\n");
            for (int i = 0; i < hist_count; i++) {
                print_uint(i + 1);
                print(": ");
                print_int(history[i]);
                print("\n");
            }
            continue;
        }
        
        if (strcmp(buf, "mc") == 0) {
            has_memory = 0;
            memory = 0;
            print("Memory cleared\n");
            continue;
        }
        
        if (strcmp(buf, "m") == 0) {
            if (has_memory) {
                print("= ");
                print_int(memory);
                print("\n");
            } else {
                print("Memory empty\n");
            }
            continue;
        }
        
        // Handle history recall !N
        if (buf[0] == '!') {
            int idx = 0;
            int i = 1;
            while (buf[i] >= '0' && buf[i] <= '9') {
                idx = idx * 10 + (buf[i] - '0');
                i++;
            }
            if (idx > 0 && idx <= hist_count) {
                print("= ");
                print_int(history[idx - 1]);
                print("\n");
            } else {
                print("Invalid history index\n");
            }
            continue;
        }
        
        // Handle constants
        int64_t a = 0;
        const char *p = buf;
        
        if (strcmp(buf, "pi") == 0) {
            a = 3; // Approximated as integer
            print("= ");
            print_int(a);
            print("\n");
            if (hist_count < HIST_MAX) history[hist_count++] = a;
            continue;
        }
        
        if (strcmp(buf, "e") == 0) {
            a = 2; // Approximated as integer
            print("= ");
            print_int(a);
            print("\n");
            if (hist_count < HIST_MAX) history[hist_count++] = a;
            continue;
        }
        
        // Handle single-argument functions
        int64_t arg;
        parse_int(p, &p, &arg);
        
        if (strcmp(buf, "sqrt") == 0) {
            if (arg < 0) {
                print("Error: sqrt of negative\n");
            } else {
                int64_t res = sqrt_int(arg);
                print("= ");
                print_int(res);
                print("\n");
                if (hist_count < HIST_MAX) history[hist_count++] = res;
            }
            continue;
        }
        
        if (strcmp(buf, "abs") == 0) {
            int64_t res = arg < 0 ? -arg : arg;
            print("= ");
            print_int(res);
            print("\n");
            if (hist_count < HIST_MAX) history[hist_count++] = res;
            continue;
        }
        
        // Handle memory store: m= value
        if (buf[0] == 'm' && buf[1] == '=') {
            parse_int(buf + 2, &p, &memory);
            has_memory = 1;
            print("Stored: ");
            print_int(memory);
            print("\n");
            continue;
        }
        
        // Handle binary operations
        p = buf;
        parse_int(p, &p, &a);
        while (*p == ' ') p++;
        char op = *p++;
        while (*p == ' ') p++;
        int64_t b; parse_int(p, &p, &b);
        
        int64_t res = 0;
        int error = 0;
        
        switch (op) {
            case '+': res = a + b; break;
            case '-': res = a - b; break;
            case '*': res = a * b; break;
            case '/':
                if (b != 0) res = a / b;
                else { print("Div by zero!\n"); error = 1; }
                break;
            case '^':
                if (b < 0) {
                    print("Neg exponent unsupported!\n");
                    error = 1;
                } else {
                    uint64_t exp = (uint64_t)b;
                    pow_int(a, exp, &res);
                }
                break;
            default:
                print("Unknown operator.\n");
                error = 1;
                break;
        }
        
        if (!error) {
            print("= ");
            print_int(res);
            print("\n");
            if (hist_count < HIST_MAX) history[hist_count++] = res;
        }
    }
    
    sb_terminate(0);
}
