#ifndef LF_ZEPHYR_SUPPORT_H
#define LF_ZEPHYR_SUPPORT_H

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC
#include <stdbool.h>

#include <zephyr/kernel.h>

#define TARGET_EMBEDDED
#define PRINTF_TIME "%" PRIu64
#define PRINTF_MICROSTEP "%" PRIu32
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"

#define calloc k_calloc
#define malloc k_malloc
#define free(x) k_free(x)
// FIXME: What to do about realloc?
#define realloc assert(false)
/**
 * Time instant. Both physical and logical times are represented
 * using this typedef.
 */
typedef int64_t _instant_t;

/**
 * Interval of time.
 */
typedef int64_t _interval_t;

/**
 * Microstep instant.
 */
typedef uint32_t _microstep_t;



#endif // LF_ZEPHYR_SUPPORT_H
