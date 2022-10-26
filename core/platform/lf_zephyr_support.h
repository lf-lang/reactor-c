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

#ifdef NUMBER_OF_WORKERS
typedef struct k_mutex _lf_mutex_t;
typedef struct k_condvar _lf_cond_t;
typedef k_tid_t _lf_thread_t;

extern _lf_mutex_t mutex;
extern _lf_cond_t event_q_changed;

// Atomics
int _zephyr_atomic_fetch_add(int *ptr, int value);
int _zephyr_atomic_add_fetch(int *ptr, int value);
bool _zephyr_bool_compare_and_swap(bool *ptr, bool value, bool newval);
bool _zephyr_val_compare_and_swap(int *ptr, int value, int newval);

#endif

#define _LF_TIMEOUT 1


#endif // LF_ZEPHYR_SUPPORT_H
