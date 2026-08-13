#include "calendar.h"
#include "time.h"
#include "kernel.h"

/* Helper: determine if a year is a leap year in the Gregorian calendar. */
static bool is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Days per month for a non‑leap year. */
static const int days_in_month[12] = {
    31, /* January */
    28, /* February */
    31, /* March */
    30, /* April */
    31, /* May */
    30, /* June */
    31, /* July */
    31, /* August */
    30, /* September */
    31, /* October */
    30, /* November */
    31  /* December */
};

int calendar_get_date(struct calendar_date *date)
{
    if (!date)
        return -1;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        /* Fallback to kernel monotonic time if the realtime clock is unavailable. */
        ts.tv_sec = get_s_time();
        ts.tv_nsec = 0;
    }

    /* Number of days since the Unix epoch (1970‑01‑01). */
    uint64_t days = ts.tv_sec / 86400ULL;

    int year = 1970;
    while (1) {
        int diy = is_leap_year(year) ? 366 : 365;
        if (days >= (uint64_t)diy) {
            days -= diy;
            year++;
        } else {
            break;
        }
    }

    int month = 1;
    for (int i = 0; i < 12; ++i) {
        int dim = days_in_month[i];
        if (i == 1 && is_leap_year(year))
            dim = 29;
        if (days >= (uint64_t)dim) {
            days -= dim;
            month++;
        } else {
            break;
        }
    }

    date->year = year;
    date->month = month;
    date->day = (int)days + 1;
    return 0;
}

int calendar_get_time(struct calendar_time *tod)
{
    if (!tod)
        return -1;

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        ts.tv_sec = get_s_time();
        ts.tv_nsec = 0;
    }

    uint64_t secs = ts.tv_sec % 86400ULL; // seconds within the current day
    tod->hour   = secs / 3600ULL;
    tod->minute = (secs % 3600ULL) / 60ULL;
    tod->second = secs % 60ULL;
    return 0;
}
