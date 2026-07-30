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
