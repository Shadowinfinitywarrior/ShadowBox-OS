#include "time.h"
#include "kernel.h"

uint64_t get_ns_time(void) {
    return (boot_ticks * 1000000000) / hz;
}

uint64_t get_us_time(void) {
    return (boot_ticks * 1000000) / hz;
}

uint64_t get_ms_time(void) {
    return (boot_ticks * 1000) / hz;
}

uint64_t get_s_time(void) {
    return boot_ticks / hz;
}

void ndelay(uint64_t ns) {
    for (volatile uint64_t i = 0; i < ns; i++) {
        __asm__ volatile("nop");
    }
}

void udelay(uint64_t us) {
    for (volatile uint64_t i = 0; i < us * 10; i++) {
        __asm__ volatile("nop");
    }
}

void mdelay(uint64_t ms) {
    udelay(ms * 1000);
}

void msleep(uint64_t ms) {
    uint64_t target = boot_ticks + (ms * hz) / 1000;
    extern void yield(void);
    while (boot_ticks < target) {
        yield();
    }
}

void ssleep(uint64_t s) {
    msleep(s * 1000);
}

uint64_t ktime_get(void) {
    return get_ms_time();
}

uint64_t ktime_get_ns(void) {
    return get_ns_time();
}

uint64_t ktime_get_real_ns(void) {
    return get_ns_time();
}

void ktime_get_ts(struct timespec *ts) {
    if (ts) {
        uint64_t ns = get_ns_time();
        ts->tv_sec = ns / 1000000000;
        ts->tv_nsec = ns % 1000000000;
    }
}

/* --- Time zone / clock adjustment state --- */
static int tz_offset_min = 0;     /* offset from UTC in minutes */
static int64_t clock_adjust_sec = 0; /* NTP correction applied to wall clock */

void time_set_timezone_offset(int minutes) {
    tz_offset_min = minutes;
}

int time_get_timezone_offset(void) {
    return tz_offset_min;
}

void time_set_adjust(int64_t seconds) {
    clock_adjust_sec = seconds;
}

int64_t time_get_adjust(void) {
    return clock_adjust_sec;
}

/* Current wall-clock time as a Unix epoch (seconds), from the RTC, before
 * timezone/adjustment offsets are applied. */
static int tz_days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

uint64_t rtc_unix_time_now(void) {
    extern void rtc_get_time(uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint32_t*);
    uint8_t sec, min, hour, day, month;
    uint32_t year;
    rtc_get_time(&sec, &min, &hour, &day, &month, &year);
    uint64_t days = 0;
    for (int y = 1970; y < (int)year; y++) {
        days += 365 + ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    }
    for (int m = 1; m < (int)month; m++) {
        days += tz_days_in_month[m];
        if (m == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) days++;
    }
    days += day - 1;
    return ((days * 24 + hour) * 60 + min) * 60 + sec;
}
