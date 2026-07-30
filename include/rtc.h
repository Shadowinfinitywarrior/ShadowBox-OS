#ifndef SHADOWBOX_RTC_H
#define SHADOWBOX_RTC_H

#include "types.h"

/*
 * rtc_init - Initialize Real-Time Clock
 */
void rtc_init(void);

/*
 * rtc_get_time - Read current time from RTC
 * @sec:   Output for seconds
 * @min:   Output for minutes
 * @hour:  Output for hours
 * @day:   Output for day of month
 * @month: Output for month
 * @year:  Output for year
 */
void rtc_get_time(uint8_t *sec, uint8_t *min, uint8_t *hour, uint8_t *day, uint8_t *month, uint32_t *year);

#endif
