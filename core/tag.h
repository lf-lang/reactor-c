/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Hou Seng (Steven) Wong
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Header file for tag functions for Lingua Franca programs.
 */

#ifndef TAG_H
#define TAG_H

#include "platform.h"
#include "limits.h"

#ifdef MICROSECOND_TIME
/* Conversion of time to microseconds. */
#define NSEC(t) (t / 1000L)
#define NSECS(t) (t / 1000L)
#define USEC(t) (t * 1L)
#define USECS(t) (t * 1L)
#define MSEC(t) (t * 1000L)
#define MSECS(t) (t * 1000L)
#define SEC(t) (t * 1000000L)
#define SECS(t) (t * 1000000L)
#define SECOND(t)  (t * 1000000000L)
#define SECONDS(t) (t * 1000000000L)
#define MINUTE(t)  (t * 1000000000L)
#define MINUTES(t) (t * 1000000000L)
#define HOUR(t)   (t * 60000000000L)
#define HOURS(t)  (t * 60000000000L)
#define DAY(t)  (t * 3600000000000L)
#define DAYS(t) (t * 3600000000000L)
#define WEEK(t)   (t * 86400000000000L)
#define WEEKS(t)  (t * 86400000000000L)
#else
/* Conversion of time to nanoseconds. */
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
#endif

#ifdef BIT_32
#define NEVER LONG_MIN
#define FOREVER LONG_MAX
#define NEVER_TAG (tag_t) { .time = LONG_MIN, .microstep = 0u }
// Need a separate initializer expression to comply with some C compilers
#define NEVER_TAG_INITIALIZER { LONG_MIN,  0u }
#define FOREVER_TAG (tag_t) { .time = LONG_MAX, .microstep = UINT_MAX }
// Need a separate initializer expression to comply with some C compilers
#define FOREVER_TAG_INITIALIZER { LONG_MAX,  UINT_MAX }
#else
#define NEVER LLONG_MIN
#define FOREVER LLONG_MAX
#define NEVER_TAG (tag_t) { .time = LLONG_MIN, .microstep = 0u }
// Need a separate initializer expression to comply with some C compilers
#define NEVER_TAG_INITIALIZER { LLONG_MIN,  0u }
#define FOREVER_TAG (tag_t) { .time = LLONG_MAX, .microstep = UINT_MAX }
// Need a separate initializer expression to comply with some C compilers
#define FOREVER_TAG_INITIALIZER { LLONG_MAX,  UINT_MAX }
#endif

// Convenience for converting times
#define BILLION 1000000000LL

/**
 * Global physical clock offset.
 * Initially set according to the RTI's clock in federated
 * programs.
 */
extern interval_t _lf_time_physical_clock_offset;

/**
 * A test offset that is applied to the clock.
 * The clock synchronization algorithm must correct for this offset.
 * This offset is especially useful to test clock synchronization on the
 * same machine.
 */
extern interval_t _lf_time_test_physical_clock_offset;

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time. This is applied to the physical clock
 * to get a more meaningful and universal time.
 * 
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
extern interval_t _lf_time_epoch_offset;

/**
 * A tag is a time, microstep pair.
 */
typedef struct {
    instant_t time;
    microstep_t microstep;
} tag_t;

/**
 * A tag interval indicates the
 * pairwise difference of two tags.
 */
typedef tag_t tag_interval_t;

/**
 * Return the current tag, a logical time, microstep pair.
 */
tag_t lf_tag(void* self);

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
 * Return the current logical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * 
 * @return A time instant.
 */
instant_t lf_time_logical(void* self);


/**
 * Return the elapsed logical time in nanoseconds
 * since the start of execution.
 * @return A time interval.
 */
interval_t lf_time_logical_elapsed(void *self);


/**
 * Return the current physical time in nanoseconds.
 * On many platforms, this is the number of nanoseconds
 * since January 1, 1970, but it is actually platform dependent.
 * @return A time instant.
 */
instant_t lf_time_physical(void);


/**
 * Return the elapsed physical time in nanoseconds.
 * This is the time returned by get_physical_time(void) minus the
 * physical start time as measured by get_physical_time(void) when
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
 * After calling this, the value returned by get_physical_time(void)
 * and get_elpased_physical_time(void) will have this specified offset
 * added to what it would have returned before the call.
 */
void lf_set_physical_clock_offset(interval_t offset);

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

/**
 * Set a fixed offset to the physical clock.
 * After calling this, the value returned by get_physical_time()
 * and get_elpased_physical_time() will have this specified offset
 * added to what it would have returned before the call.
 */
void lf_set_physical_clock_offset(interval_t offset);

/**
 * For C++ compatibility, take a volatile tag_t and return a non-volatile
 * variant.
 */
#ifdef __cplusplus
tag_t _lf_convert_volatile_tag_to_nonvolatile(tag_t volatile const& vtag);
#else
/**
 * @note This is an undefined behavior in C and should
 *  be used with utmost caution. See Section 6.7.2 of the C99 standard.
 */
tag_t _lf_convert_volatile_tag_to_nonvolatile(tag_t volatile vtag);
#endif

#endif // TAG_H
