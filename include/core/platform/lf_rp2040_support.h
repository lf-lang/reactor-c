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

#ifndef LF_SINGLE_THREADED
#warning "Threaded support on rp2040 is still experimental"

typedef recursive_mutex_t lf_mutex_t;
typedef struct {
    semaphore_t sema;
    lf_mutex_t* mutex;
} lf_cond_t;
typedef int lf_thread_t;


/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr`
 */
int _rp2040_atomic_fetch_add(int *ptr, int value);
/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _rp2040_atomic_add_fetch(int *ptr, int value);

/**
 * @brief Compare and swap for boolaen value.
 * If `*ptr` is equal to `value` then overwrite it
 * with `newval`. If not do nothing. Retruns true on overwrite.
 */
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval);

/**
 * @brief Compare and swap for integers. If `*ptr` is equal
 * to `value`, it is updated to `newval`. The function returns
 * the original value of `*ptr`.
 */
int  _rp2040_val_compare_and_swap(int *ptr, int value, int newval);


#endif // LF_THREADED

#endif // LF_PICO_SUPPORT_H
