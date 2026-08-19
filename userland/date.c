#include "sys.h"

/* date - print the current wall-clock date and time (RTC + timezone + NTP) */

static void print(const char *s) { sb_push(1, s, strlen(s)); }
static void print_num(uint64_t v) {
    char buf[24]; int idx = 0;
    if (v == 0) buf[idx++] = '0';
    while (v > 0) { buf[idx++] = '0' + (v % 10); v /= 10; }
    while (idx > 0) sb_push(1, &buf[--idx], 1);
}
static void print_pad(uint64_t v) {
    if (v < 10) print("0");
    print_num(v);
}

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0); }
static int dim(int m, int y) {
    static const int d[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == 2 && is_leap(y)) return 29;
    return d[m];
}

void _start(void) {
    timeval_t tv;
    if (sys_gettimeofday(&tv) != 0) { print("date: cannot read clock\n"); sb_terminate(1); }
    int64_t now = (int64_t)tv.tv_sec;
    if (now < 0) now = 0;

    uint64_t days = (uint64_t)now / 86400;
    uint64_t rem = (uint64_t)now % 86400;
    uint64_t hour = rem / 3600, min = (rem % 3600) / 60, sec = rem % 60;

    int year = 1970;
    while (days >= (uint64_t)(is_leap(year) ? 366 : 365)) {
        days -= is_leap(year) ? 366 : 365;
        year++;
    }
    int month = 1;
    while (month <= 12 && days >= (uint64_t)dim(month, year)) {
        days -= dim(month, year);
        month++;
    }
    int day = (int)days + 1;

    static const char *mon[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                "Jul","Aug","Sep","Oct","Nov","Dec"};
    print(mon[month - 1]); print(" ");
    print_pad(day); print(" ");
    print_num(year); print("  ");
    print_pad(hour); print(":");
    print_pad(min); print(":");
    print_pad(sec); print("\n");
    sb_terminate(0);
}