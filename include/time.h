#ifndef SHADOWBOX_TIME_H
#define SHADOWBOX_TIME_H

#include "types.h"

/*
 * timespec - POSIX timespec structure
 * @tv_sec:  Seconds
 * @tv_nsec: Nanoseconds
 */
struct timespec {
    uint64_t tv_sec;
    uint64_t tv_nsec;
};

/*
 * timeval - POSIX timeval structure
 * @tv_sec:  Seconds
 * @tv_usec: Microseconds
 */
struct timeval {
    uint64_t tv_sec;
    uint64_t tv_usec;
};

/*
 * itimerspec - POSIX interval timer spec
 * @it_interval: Timer interval
 * @it_value:    Initial expiration
 */
struct itimerspec {
    struct timespec it_interval;
    struct timespec it_value;
};

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define CLOCK_MONOTONIC_RAW      4
#define CLOCK_REALTIME_COARSE    5
#define CLOCK_MONOTONIC_COARSE   6
#define CLOCK_BOOTTIME           7

#define TIMER_ABSTIME 0x01

/*
 * hrtimer_t - High-resolution timer
 * @expires:  Expiration time in nanoseconds
 * @interval: Interval for periodic timers
 * @function: Callback function
 * @data:     Callback data
 * @state:    Timer state
 * @mode:     Absolute or relative
 */
typedef struct hrtimer {
    uint64_t expires;
    uint64_t interval;
    int (*function)(struct hrtimer *timer);
    void *data;
    int state;
    int mode;
} hrtimer_t;

#define HRTIMER_STATE_INACTIVE 0
#define HRTIMER_STATE_ENQUEUED 1
#define HRTIMER_STATE_CALLBACK 2

extern uint64_t boot_ticks;
extern uint64_t hz;

/*
 * get_ns_time - Get current time in nanoseconds
 * Returns: Current nanosecond timestamp
 */
uint64_t get_ns_time(void);

/*
 * get_us_time - Get current time in microseconds
 * Returns: Current microsecond timestamp
 */
uint64_t get_us_time(void);

/*
 * get_ms_time - Get current time in milliseconds
 * Returns: Current millisecond timestamp
 */
uint64_t get_ms_time(void);

/*
 * get_s_time - Get current time in seconds
 * Returns: Current second timestamp
 */
uint64_t get_s_time(void);

/*
 * hrtimer_init - Initialize a high-resolution timer
 * @timer:    Timer to initialize
 * @clock_id: Clock source
 * Returns: 0 on success, -1 on error
 */
int hrtimer_init(hrtimer_t *timer, int clock_id);

/*
 * hrtimer_start - Start a high-resolution timer
 * @timer:    Timer to start
 * @expires:  Expiration time (ns)
 * @interval: Interval for periodic timers
 * @mode:     Absolute or relative
 * Returns: 0 on success, -1 on error
 */
int hrtimer_start(hrtimer_t *timer, uint64_t expires, uint64_t interval, int mode);

/*
 * hrtimer_cancel - Cancel a high-resolution timer
 * @timer: Timer to cancel
 * Returns: 0 on success, -1 on error
 */
int hrtimer_cancel(hrtimer_t *timer);

/*
 * hrtimer_forward_now - Forward timer expiration by interval
 * @timer:    Timer to forward
 * @interval: Interval to add
 * Returns: Number of overruns
 */
int hrtimer_forward_now(hrtimer_t *timer, uint64_t interval);

/*
 * clock_gettime - Get time of a specific clock
 * @clock_id: Clock identifier
 * @tp:       Output for time
 * Returns: 0 on success, -1 on error
 */
int clock_gettime(int clock_id, struct timespec *tp);

/*
 * clock_settime - Set time of a specific clock
 * @clock_id: Clock identifier
 * @tp:       Time to set
 * Returns: 0 on success, -1 on error
 */
int clock_settime(int clock_id, const struct timespec *tp);

/*
 * clock_getres - Get resolution of a clock
 * @clock_id: Clock identifier
 * @res:      Output for resolution
 * Returns: 0 on success, -1 on error
 */
int clock_getres(int clock_id, struct timespec *res);

/*
 * clock_nanosleep - Sleep with nanosecond precision
 * @clock_id: Clock to use
 * @flags:    TIMER_ABSTIME or 0
 * @request:  Sleep duration/time
 * @remain:   Output for remaining time
 * Returns: 0 on success, -1 on error
 */
int clock_nanosleep(int clock_id, int flags, const struct timespec *request, struct timespec *remain);

/*
 * timer_create - Create a POSIX per-process timer
 * @clock_id: Clock source
 * @sigev:    Signal event notification
 * @timer_id: Output for timer ID
 * Returns: 0 on success, -1 on error
 */
int timer_create(int clock_id, void *sigev, int *timer_id);

/*
 * timer_delete - Delete a POSIX timer
 * @timer_id: Timer ID
 * Returns: 0 on success, -1 on error
 */
int timer_delete(int timer_id);

/*
 * timer_settime - Arm or disarm a POSIX timer
 * @timer_id:  Timer ID
 * @flags:     Flags (TIMER_ABSTIME)
 * @new_value: New timer specification
 * @old_value: Output for old specification
 * Returns: 0 on success, -1 on error
 */
int timer_settime(int timer_id, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);

/*
 * timer_gettime - Get POSIX timer status
 * @timer_id:   Timer ID
 * @curr_value: Output for current specification
 * Returns: 0 on success, -1 on error
 */
int timer_gettime(int timer_id, struct itimerspec *curr_value);

/*
 * timer_getoverrun - Get timer overrun count
 * @timer_id: Timer ID
 * Returns: Overrun count
 */
int timer_getoverrun(int timer_id);

/*
 * timespec_add - Add two timespecs, result in first
 * @a: First operand and result
 * @b: Second operand
 */
void timespec_add(struct timespec *a, const struct timespec *b);

/*
 * timespec_sub - Subtract two timespecs, result in first
 * @a: First operand and result
 * @b: Second operand
 */
void timespec_sub(struct timespec *a, const struct timespec *b);

/*
 * timespec_compare - Compare two timespecs
 * @a: First timespec
 * @b: Second timespec
 * Returns: -1, 0, or 1
 */
int timespec_compare(const struct timespec *a, const struct timespec *b);

/*
 * timespec_to_ns - Convert timespec to nanoseconds
 * @ts: Timespec to convert
 * Returns: Nanosecond value
 */
uint64_t timespec_to_ns(const struct timespec *ts);

/*
 * ns_to_timespec - Convert nanoseconds to timespec
 * @ns: Nanosecond value
 * @ts: Output timespec
 */
void ns_to_timespec(uint64_t ns, struct timespec *ts);

/*
 * ktime_get - Get monotonic kernel time
 * Returns: Kernel time in nanoseconds
 */
uint64_t ktime_get(void);

/*
 * ktime_get_ns - Get monotonic time in nanoseconds
 * Returns: Nanosecond timestamp
 */
uint64_t ktime_get_ns(void);

/*
 * ktime_get_real_ns - Get real time in nanoseconds
 * Returns: Nanosecond timestamp
 */
uint64_t ktime_get_real_ns(void);

/*
 * ktime_get_ts - Get monotonic time as timespec
 * @ts: Output timespec
 */
void ktime_get_ts(struct timespec *ts);

/*
 * ndelay - Busy-wait for nanoseconds
 * @ns: Nanoseconds to wait
 */
void ndelay(uint64_t ns);

/*
 * udelay - Busy-wait for microseconds
 * @us: Microseconds to wait
 */
void udelay(uint64_t us);

/*
 * mdelay - Busy-wait for milliseconds
 * @ms: Milliseconds to wait
 */
void mdelay(uint64_t ms);

/*
 * msleep - Sleep for milliseconds
 * @ms: Milliseconds to sleep
 */
void msleep(uint64_t ms);

/*
 * ssleep - Sleep for seconds
 * @s: Seconds to sleep
 */
void ssleep(uint64_t s);

#endif
