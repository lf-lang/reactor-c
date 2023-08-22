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

// function ptr typedef
// @param void ptr
// @return void ptr
typedef void *(*lf_function_t) (void *);

// mutex primitive
// backed by hardware spinlock
typedef recursive_mutex_t lf_mutex_t;

// condition var primitive
// FIXME: repetitive implementation of semaphore
typedef semaphore_t lf_cond_t;

// thread id type
// use enum due to limited number of workers
typedef enum {
  RP2040_CORE_0 = 0,
  RP2040_CORE_1 = 1,
} lf_thread_t;

// core1 entry method
void _rp2040_core1_entry();

// atomics
int _rp2040_atomic_fetch_add(int *ptr, int value);
int _rp2040_atomic_add_fetch(int *ptr, int value);
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval);
int _rp2040_val_compare_and_swap(int *ptr, int value, int newval);


#endif // LF_THREADED
#endif // LF_PICO_SUPPORT_H
