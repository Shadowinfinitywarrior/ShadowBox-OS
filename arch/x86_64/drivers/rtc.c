#include "rtc.h"
#include "kernel.h"
#include "io.h"

#define CURRENT_YEAR 2026 // Fallback if century register isn't present

static inline uint8_t get_update_in_progress_flag() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

static inline uint8_t get_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void rtc_init(void) {
    printk(KERN_INFO "RTC: Initialized CMOS RTC driver.\n");
}

void rtc_get_time(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year) {
    uint8_t last_sec, last_min, last_hour, last_day, last_month, last_year, registerB;
    uint8_t s, m, h, d, mo, y;
    
    while (get_update_in_progress_flag());
    s = get_rtc_register(0x00);
    m = get_rtc_register(0x02);
    h = get_rtc_register(0x04);
    d = get_rtc_register(0x07);
    mo = get_rtc_register(0x08);
    y = get_rtc_register(0x09);

    do {
        last_sec = s;
        last_min = m;
        last_hour = h;
        last_day = d;
        last_month = mo;
        last_year = y;

        while (get_update_in_progress_flag());
        s = get_rtc_register(0x00);
        m = get_rtc_register(0x02);
        h = get_rtc_register(0x04);
        d = get_rtc_register(0x07);
        mo = get_rtc_register(0x08);
        y = get_rtc_register(0x09);
    } while ( (last_sec != s) || (last_min != m) || (last_hour != h) ||
              (last_day != d) || (last_month != mo) || (last_year != y) );

    registerB = get_rtc_register(0x0B);

    if (!(registerB & 0x04)) {
        s = (s & 0x0F) + ((s / 16) * 10);
        m = (m & 0x0F) + ((m / 16) * 10);
        h = ( (h & 0x0F) + (((h & 0x70) / 16) * 10) ) | (h & 0x80);
        d = (d & 0x0F) + ((d / 16) * 10);
        mo = (mo & 0x0F) + ((mo / 16) * 10);
        y = (y & 0x0F) + ((y / 16) * 10);
    }

    if (!(registerB & 0x02) && (h & 0x80)) {
        h = ((h & 0x7F) + 12) % 24;
    }

    uint32_t full_year = y + (CURRENT_YEAR / 100) * 100;
    if (full_year < CURRENT_YEAR) full_year += 100;

    if (sec) *sec = s;
    if (min) *min = m;
    if (hour) *hour = h;
    if (day) *day = d;
    if (month) *month = mo;
    if (year) *year = full_year;
}
