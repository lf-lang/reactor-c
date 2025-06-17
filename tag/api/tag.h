/**
 * @file tag.h
 * @brief Time and tag definitions and functions for Lingua Franca.
 * @ingroup API
 *
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng (Steven) Wong
 *
 * This file defines the core time and tag types and operations used throughout
 * the Lingua Franca runtime. It provides functions for manipulating logical and
 * physical time, as well as the tag structure that combines time with microsteps.
 */

#ifndef TAG_H
#define TAG_H

/*! @brief Number of nanoseconds @ingroup Constants */
#define NSEC(t) ((interval_t)(t * 1LL))
/*! @brief Number of nanoseconds @ingroup Constants */
#define NSECS(t) ((interval_t)(t * 1LL))
/*! @brief Number of microseconds @ingroup Constants */
#define USEC(t) ((interval_t)(t * 1000LL))
/*! @brief Number of microseconds @ingroup Constants */
#define USECS(t) ((interval_t)(t * 1000LL))
/*! @brief Number of milliseconds @ingroup Constants */
#define MSEC(t) ((interval_t)(t * 1000000LL))
/*! @brief Number of milliseconds @ingroup Constants */
#define MSECS(t) ((interval_t)(t * 1000000LL))
/*! @brief Number of seconds @ingroup Constants */
#define SEC(t) ((interval_t)(t * 1000000000LL))
/*! @brief Number of seconds @ingroup Constants */
#define SECS(t) ((interval_t)(t * 1000000000LL))
/*! @brief Number of seconds @ingroup Constants */
#define SECOND(t) ((interval_t)(t * 1000000000LL))
/*! @brief Number of seconds @ingroup Constants */
#define SECONDS(t) ((interval_t)(t * 1000000000LL))
/*! @brief Number of minutes @ingroup Constants */
#define MINUTE(t) ((interval_t)(t * 60000000000LL))
/*! @brief Number of minutes @ingroup Constants */
#define MINUTES(t) ((interval_t)(t * 60000000000LL))
/*! @brief Number of hours @ingroup Constants */
#define HOUR(t) ((interval_t)(t * 3600000000000LL))
/*! @brief Number of hours @ingroup Constants */
#define HOURS(t) ((interval_t)(t * 3600000000000LL))
/*! @brief Number of days @ingroup Constants */
#define DAY(t) ((interval_t)(t * 86400000000000LL))
/*! @brief Number of days @ingroup Constants */
#define DAYS(t) ((interval_t)(t * 86400000000000LL))
/*! @brief Number of weeks @ingroup Constants */
#define WEEK(t) ((interval_t)(t * 604800000000000LL))
/*! @brief Number of weeks @ingroup Constants */
#define WEEKS(t) ((interval_t)(t * 604800000000000LL))

/*! @brief Time earlier than all other times @ingroup Constants */
#define NEVER ((interval_t)LLONG_MIN)
/*! @brief Smallest microstep @ingroup Constants */
#define NEVER_MICROSTEP 0u
/*! @brief Time greater than all other times @ingroup Constants */
#define FOREVER ((interval_t)LLONG_MAX)
/*! @brief Largest microstep @ingroup Constants */
#define FOREVER_MICROSTEP UINT_MAX
/*! @brief Tag earlier than all other tags @ingroup Constants */
#define NEVER_TAG                                                                                                      \
  (tag_t) { .time = NEVER, .microstep = NEVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
/*! @brief Initializer for tag earlier than all other tags @ingroup Constants */
#define NEVER_TAG_INITIALIZER {NEVER, NEVER_MICROSTEP}
/*! @brief Tag later than all other tags @ingroup Constants */
#define FOREVER_TAG                                                                                                    \
  (tag_t) { .time = FOREVER, .microstep = FOREVER_MICROSTEP }
// Need a separate initializer expression to comply with some C compilers
/*! @brief Initializer for tag later than all other tags @ingroup Constants */
#define FOREVER_TAG_INITIALIZER {FOREVER, FOREVER_MICROSTEP}
/*! @brief Zero tag @ingroup Constants */
#define ZERO_TAG (tag_t){.time = 0LL, .microstep = 0u}

/**
 * @brief Expression that is true if physical time since start exceeds the duration.
 * @ingroup API
 * @param start The start time.
 * @param duration The duration.
 */
#define CHECK_TIMEOUT(start, duration) (lf_time_physical() > ((start) + (duration)))

/*! @brief The number of nanoseconds in one second @ingroup Constants */
#define BILLION ((instant_t)1000000000LL)

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

////////////////  Type definitions

/**
 * @brief Time instant.
 * @ingroup API
 *
 * Both physical and logical times are represented using this typedef.
 */
typedef int64_t instant_t;

/**
 * @brief Interval of time.
 * @ingroup API
 */
typedef int64_t interval_t;

/**
 * @brief Microstep.
 * @ingroup API
 */
typedef uint32_t microstep_t;

/**
 * @brief A tag is a time, microstep pair.
 * @ingroup API
 */
typedef struct {
  instant_t time;
  microstep_t microstep;
} tag_t;

////////////////  Functions

/**
 * @brief Return the current tag, a logical time, microstep pair.
 * @ingroup API
 *
 * @param env A pointer to the environment from which we want the current tag.
 */
tag_t lf_tag(void* env);

/**
 * @brief Add two tags.
 * @ingroup API
 *
 * If either tag has has NEVER or FOREVER in its time field, then
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
 * @brief Return the sum of an interval and an instant, saturating on overflow and underflow.
 * @ingroup API
 *
 * @param a
 * @param b
 * @return instant_t
 */
instant_t lf_time_add(instant_t a, interval_t b);

/**
 * @brief Return an instant minus an interval, saturating on overflow and underflow.
 * @ingroup API
 *
 * @param a
 * @param b
 * @return instant_t
 */
instant_t lf_time_subtract(instant_t a, interval_t b);

/**
 * @brief Compare two tags.
 * @ingroup API
 *
 * Return -1 if the first is less than
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
 * @brief Delay a tag by the specified time interval to realize the "after" keyword.
 * @ingroup API
 *
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
 * @brief Return the latest tag strictly less than the specified tag plus the
 * interval, unless tag is NEVER or interval is negative (including NEVER),
 * @ingroup Internal
 *
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
 * @brief Return the greatest tag earlier than the given tag.
 * @ingroup Internal
 *
 * If the given tag is `FOREVER_TAG` or `NEVER_TAG`, however, just return the given tag.
 * @param tag The tag.
 */
tag_t lf_tag_latest_earlier(tag_t tag);

/**
 * @brief Return the current logical time in nanoseconds.
 * @ingroup Internal
 *
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 *
 * @param env The environment from which we want the current logical time.
 * @return A time instant.
 */
instant_t lf_time_logical(void* env);

/**
 * @brief Return the elapsed logical time in nanoseconds
 * since the start of execution.
 * @ingroup Internal
 *
 * @param env The environment from which we want the elapsed logical time.
 * @return A time interval.
 */
interval_t lf_time_logical_elapsed(void* env);

/**
 * @brief Return the current physical time in nanoseconds.
 * @ingroup API
 *
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 *
 * @return A time instant.
 */
instant_t lf_time_physical(void);

/**
 * @brief Return the elapsed physical time in nanoseconds.
 * @ingroup API
 *
 * This is the time returned by lf_time_physical(void) minus the
 * physical start time as measured by lf_time_physical(void) when
 * the program was started.
 */
instant_t lf_time_physical_elapsed(void);

/**
 * @brief Return the physical and logical time of the start of execution in nanoseconds.
 * @ingroup API
 *
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 *
 * @return A time instant.
 */
instant_t lf_time_start(void);

/**
 * @brief For user-friendly reporting of time values, the buffer length required.
 * @ingroup API
 *
 * This is calculated as follows, based on 64-bit time in nanoseconds:
 * Maximum number of weeks is 15,250
 * Maximum number of days is 6
 * Maximum number of hours is 23
 * Maximum number of minutes is 59
 * Maximum number of seconds is 59
 * Maximum number of nanoseconds is 999,999,999
 * Maximum number of microsteps is 4,294,967,295
 * Total number of characters for the above is 24.
 * Text descriptions and spaces add an additional 30,
 * for a total of 54. One more allows for a null terminator.
 * Round up to a power of two.
 */
#define LF_TIME_BUFFER_LENGTH 64

/**
 * @brief Store into the specified buffer a string giving a human-readable
 * rendition of the specified time.
 * @ingroup API
 *
 * The buffer must have length at least
 * equal to @ref LF_TIME_BUFFER_LENGTH. The format is:
 * ```
 *    x weeks, x d, x hr, x min, x s, x unit
 * ```
 * where each `x` is a string of numbers with commas inserted if needed
 * every three numbers and `unit` is ns, us, or
 * ms.
 * @param buffer The buffer into which to write the string.
 * @param time The time to write.
 * @return The number of characters written (not counting the null terminator).
 */
size_t lf_readable_time(char* buffer, instant_t time);

/**
 * @brief Print a non-negative time value in nanoseconds with commas separating thousands
 * into the specified buffer.
 * @ingroup API
 *
 * Ideally, this would use the locale to
 * use periods if appropriate, but I haven't found a sufficiently portable
 * way to do that.
 *
 * @param buffer A buffer long enough to contain a string like "9,223,372,036,854,775,807".
 * @param time A time value.
 * @return The number of characters written (not counting the null terminator).
 */
size_t lf_comma_separated_time(char* buffer, instant_t time);

#endif // TAG_H
