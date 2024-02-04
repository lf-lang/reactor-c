/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng (Steven) Wong
 * @copyright (c) 2020-2023, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Time and tag definitions and functions for Lingua Franca
 */

#ifndef TAG_H
#define TAG_H

#define NSEC(t) (t * 1LL)
#define NSECS(t) (t * 1LL)
#define USEC(t) (t * 1000LL)
#define USECS(t) (t * 1000LL)
#define MSEC(t) (t * 1000000LL)
#define MSECS(t) (t * 1000000LL)
#define SEC(t)  (t * 1000000000LL)
#define SECS(t) (t * 1000000000LL)
#define SECOND(t)  (t * 1000000000LL)
#define SECONDS(t) (t * 1000000000LL)
#define MINUTE(t)   (t * 60000000000LL)
#define MINUTES(t)  (t * 60000000000LL)
#define HOUR(t)  (t * 3600000000000LL)
#define HOURS(t) (t * 3600000000000LL)
#define DAY(t)   (t * 86400000000000LL)
#define DAYS(t)  (t * 86400000000000LL)
#define WEEK(t)  (t * 604800000000000LL)
#define WEEKS(t) (t * 604800000000000LL)

#define NEVER LLONG_MIN
#define NEVER_MICROSTEP 0u
#define FOREVER LLONG_MAX
#define FOREVER_MICROSTEP UINT_MAX
#define NEVER_TAG (tag_t) { .time = NEVER, .microstep = NEVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
#define NEVER_TAG_INITIALIZER { NEVER,  NEVER_MICROSTEP }
#define FOREVER_TAG (tag_t) { .time = FOREVER, .microstep = FOREVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
#define FOREVER_TAG_INITIALIZER { FOREVER,  FOREVER_MICROSTEP }
#define ZERO_TAG (tag_t) { .time = 0LL, .microstep = 0u }

// Convenience for converting times
#define BILLION 1000000000LL

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

////////////////  Type definitions

/**
 * Time instant. Both physical and logical times are represented
 * using this typedef.
 */
typedef int64_t instant_t;

/**
 * Interval of time.
 */
typedef int64_t interval_t;

/**
 * Microstep instant.
 */
typedef uint32_t microstep_t;

/**
 * A tag is a time, microstep pair.
 */
typedef struct {
    instant_t time;
    microstep_t microstep;
} tag_t;

////////////////  Functions

/**
 * Return the current tag, a logical time, microstep pair.
 * @param env A pointer to the environment from which we want the current tag.
 */
tag_t lf_tag(void* env);

/**
 * Add two tags.  If either tag has has NEVER or FOREVER in its time field, then
 * return NEVER_TAG or FOREVER_TAG, respectively. Also return NEVER_TAG or FOREVER_TAG
 * if the result underflows or overflows when adding the times.
 * If the microstep overflows, also return FOREVER_TAG.
 * If the time field of the second tag is greater than 0, then the microstep of the first tag
 * is reset to 0 before adding. This models the delay semantics in LF and makes this
 * addition operation non-commutative.
 * @param a The first tag.
 * @param b The second tag.
 */
tag_t lf_tag_add(tag_t a, tag_t b);

/**
 * Compare two tags. Return -1 if the first is less than
 * the second, 0 if they are equal, and +1 if the first is
 * greater than the second. A tag is greater than another if
 * its time is greater or if its time is equal and its microstep
 * is greater.
 * @param tag1
 * @param tag2
 * @return -1, 0, or 1 depending on the relation.
 */
int lf_tag_compare(tag_t tag1, tag_t tag2);

/**
 * Delay a tag by the specified time interval to realize the "after" keyword.
 * Any interval less than 0 (including NEVER) is interpreted as "no delay",
 * whereas an interval equal to 0 is interpreted as one microstep delay.
 * If the time field of the tag is NEVER or the interval is negative,
 * return the unmodified tag. If the time interval is 0LL, add one to
 * the microstep, leave the time field alone, and return the result.
 * Otherwise, add the interval to the time field of the tag and reset
 * the microstep to 0. If the sum overflows, saturate the time value at
 * FOREVER. For example:
 * - if tag = (t, 0) and interval = 10, return (t + 10, 0)
 * - if tag = (t, 0) and interval = 0, return (t, 1)
 * - if tag = (t, 0) and interval = NEVER, return (t, 0)
 * - if tag = (FOREVER, 0) and interval = 10, return (FOREVER, 0)
 *
 * @param tag The tag to increment.
 * @param interval The time interval.
 */
tag_t lf_delay_tag(tag_t tag, interval_t interval);

/**
 * Return the latest tag strictly less than the specified tag plus the
 * interval, unless tag is NEVER or interval is negative (including NEVER),
 * in which case return the tag unmodified.  Any interval less than 0
 * (including NEVER) is interpreted as "no delay", whereas an interval
 * equal to 0 is interpreted as one microstep delay. If the time sum
 * overflows, saturate the time value at FOREVER.  For example:
 * - if tag = (t, 0) and interval = 10, return (t + 10 - 1, UINT_MAX)
 * - if tag = (t, 0) and interval = 0, return (t, 0)
 * - if tag = (t, 0) and interval = NEVER, return (t, 0)
 * - if tag = (FOREVER, 0) and interval = 10, return (FOREVER, 0)
 *
 * @param tag The tag to increment.
 * @param interval The time interval.
 */
tag_t lf_delay_strict(tag_t tag, interval_t interval);

/**
 * Return the current logical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 *
 * @param env The environment from which we want the current logical time.
 * @return A time instant.
 */
instant_t lf_time_logical(void* env);

/**
 * Return the elapsed logical time in nanoseconds
 * since the start of execution.
 * @param env The environment from which we want the elapsed logical time.
 * @return A time interval.
 */
interval_t lf_time_logical_elapsed(void *env);

/**
 * Return the current physical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * @return A time instant.
 */
instant_t lf_time_physical(void);

/**
 * Return the elapsed physical time in nanoseconds.
 * This is the time returned by lf_time_physical(void) minus the
 * physical start time as measured by lf_time_physical(void) when
 * the program was started.
 */
instant_t lf_time_physical_elapsed(void);

/**
 * Return the physical and logical time of the start of execution in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * @return A time instant.
 */
instant_t lf_time_start(void);

/**
 * Set a fixed offset to the physical clock.
 * After calling this, the value returned by lf_time_physical(void)
 * and get_elpased_physical_time(void) will have this specified offset
 * added to what it would have returned before the call.
 */
void lf_set_physical_clock_offset(interval_t offset);

/**
 * For user-friendly reporting of time values, the buffer length required.
 * This is calculated as follows, based on 64-bit time in nanoseconds:
 * Maximum number of weeks is 15,250
 * Maximum number of days is 6
 * Maximum number of hours is 23
 * Maximum number of minutes is 59
 * Maximum number of seconds is 59
 * Maximum number of nanoseconds is 999,999,999
 * Maximum number of microsteps is 4,294,967,295
 * Total number of characters for the above is 24.
 * Text descriptions and spaces add an additional 55,
 * for a total of 79. One more allows for a null terminator.
 */
#define LF_TIME_BUFFER_LENGTH 80

/**
 * Store into the specified buffer a string giving a human-readable
 * rendition of the specified time. The buffer must have length at least
 * equal to LF_TIME_BUFFER_LENGTH. The format is:
 * ```
 *    x weeks, x days, x hours, x minutes, x seconds, x unit
 * ```
 * where each `x` is a string of numbers with commas inserted if needed
 * every three numbers and `unit` is nanoseconds, microseconds, or
 * milliseconds.
 * @param buffer The buffer into which to write the string.
 * @param time The time to write.
 * @return The number of characters written (not counting the null terminator).
 */
size_t lf_readable_time(char* buffer, instant_t time);

/**
 * Print a non-negative time value in nanoseconds with commas separating thousands
 * into the specified buffer. Ideally, this would use the locale to
 * use periods if appropriate, but I haven't found a sufficiently portable
 * way to do that.
 * @param buffer A buffer long enough to contain a string like "9,223,372,036,854,775,807".
 * @param time A time value.
 * @return The number of characters written (not counting the null terminator).
 */
size_t lf_comma_separated_time(char* buffer, instant_t time);

#endif // TAG_H
