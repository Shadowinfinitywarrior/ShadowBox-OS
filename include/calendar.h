#ifndef SHADOWBOX_CALENDAR_H
#define SHADOWBOX_CALENDAR_H

#include "types.h"
#include "time.h"

/*
 * calendar_date - Date components (UTC)
 * @year:  Full year (e.g., 2026)
 * @month: Month of year, 1‑12
 * @day:   Day of month, 1‑31
 */
struct calendar_date {
    int year;
    int month;
    int day;
};

/*
 * calendar_time - Time‑of‑day components (UTC)
 * @hour:   Hours since midnight, 0‑23
 * @minute: Minutes after the hour, 0‑59
 * @second: Seconds after the minute, 0‑59
 */
struct calendar_time {
    int hour;
    int minute;
    int second;
};

/*
 * calendar_get_date - Retrieve current date using CLOCK_REALTIME.
 * @date: Output pointer for date components.
 * Returns: 0 on success, -1 if @date is NULL or time acquisition fails.
 */
int calendar_get_date(struct calendar_date *date);

/*
 * calendar_get_time - Retrieve current time‑of‑day using CLOCK_REALTIME.
 * @tod: Output pointer for time components.
 * Returns: 0 on success, -1 if @tod is NULL or time acquisition fails.
 */
int calendar_get_time(struct calendar_time *tod);

#endif // SHADOWBOX_CALENDAR_H
