/**
Pico Support
**/

/**
 * @brief pico support for reactor-c
 */

#ifndef LF_PICO_SUPPORT_H
#define LF_PICO_SUPPORT_H

#include <inttypes.h>
#include <pico/stdlib.h>
#include <pico/sync.h>

// Define PRINTF_TIME and PRINTF_MICROSTEP, which are the printf
// codes (like the d in %d to print an int) for time and microsteps.
// To use these, specify the printf as follows:
//     printf("Time: " PRINTF_TIME "\n", time_value);
// On most platforms, time is an signed 64-bit number (int64_t) and
// the microstep is an unsigned 32-bit number (uint32_t).
// Sadly, in C, there is no portable to print such numbers using
// printf without getting a warning on some platforms.
// On each platform, the code for printf if given by the macros
// PRId64 and PRIu32 defined in inttypes.h.  Hence, here, we import
// inttypes.h, then define PRINTF_TIME and PRINTF_MICROSTEP.
// If you are targeting a platform that uses some other type
// for time and microsteps, you can simply define
// PRINTF_TIME and PRINTF_MICROSTEP directly in the same file that
// defines the types instant_t, interval_t, and microstep_t.
// TODO: int 64 print format for pico 
#define PRINTF_TIME "%" PRId32
#define PRINTF_MICROSTEP "%" PRIu32

//#define instant_t int64_t
//#define interval_t int64_t
//#define microstep_t uint32_t

// For convenience, the following string can be inserted in a printf
// format for printing both time and microstep as follows:
//     printf("Tag is " PRINTF_TAG "\n", time_value, microstep);
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"

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
// TODO: define interval_t and instant_t
// interval as u64_t
// instant -> rpi pico docs on time module
#define _LF_TIMEOUT 1

#ifdef LF_THREADED
typedef recursive_mutex_t lf_mutex_t;
typedef semaphore_t lf_cond_t;
/// TODO: figure out thread representation
typedef uint32_t lf_thread_t;

/// TODO: atomic definitions

#endif // LF_THREADED 
#endif // LF_PICO_SUPPORT_H