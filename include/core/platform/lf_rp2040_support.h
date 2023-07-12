/* RP2040 API support for the C target of Lingua Franca */

/**
 * @brief pico support for reactor-c
 */

#ifndef LF_RP2040_SUPPORT_H
#define LF_RP2040_SUPPORT_H

#include <pico/stdlib.h>
#include <pico/sync.h>

#define NO_TTY

// Defines for formatting time in printf for pico
#define PRINTF_TAG "(" PRINTF_TIME ", " PRINTF_MICROSTEP ")"
#define PRINTF_TIME "%lld"
#define PRINTF_MICROSTEP "%d"

#define LF_TIME_BUFFER_LENGTH 80
#define _LF_TIMEOUT 1

#ifdef LF_THREADED
typedef void *(*lf_function_t) (void *);
typedef semaphore_t lf_cond_t;
typedef recursive_mutex_t lf_mutex_t;
typedef enum {
    CORE_0,
    CORE_1,
} lf_thread_t;

/**
 * @brief Method to run on core1 of pico to load
 * second worker thread.
 */
void _rp2040_core1_entry();

/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr`
 */
int _rp2040_atomic_fetch_add(int *ptr, int value);

/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _rp2040_atomic_add_fetch(int *ptr, int value);

/**
 * @brief Compare and swap for boolean value.
 * If `*ptr` is equal to `value` then overwrite it with `newval`
 * If not do nothing. Returns true on overwrite.
 */
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval);

/**
 * @brief Compare and swap for integers. 
 * If `*ptr` is equal to `value`, it is updated to `newval`. 
 * If not do nothing. The function returns the original value of `*ptr`.
 */
int _rp2040_val_compare_and_swap(int *ptr, int value, int newval);

#endif // LF_THREADED
#endif // LF_PICO_SUPPORT_H
