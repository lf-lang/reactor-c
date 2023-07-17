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

// pico-sdk condition variable
typedef struct __packed_aligned
{
    lock_core_t core;
    lock_owner_id_t waiter;
    uint32_t broadcast_count;   // Overflow is unlikely
    bool signaled;
} cond_t;

// lf threading primitives
typedef void *(*lf_function_t) (void *);
typedef mutex_t lf_mutex_t;
typedef struct _lf_cond_t {
    cond_t cv;
    mutex_t *mut;
} lf_cond_t;
typedef enum {
    CORE_0,
    CORE_1,
} lf_thread_t;

void _rp2040_core1_entry();

// TODO: implement optimized versions
int _rp2040_atomic_fetch_add(int *ptr, int value);
int _rp2040_atomic_add_fetch(int *ptr, int value);
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval);
int _rp2040_val_compare_and_swap(int *ptr, int value, int newval);


/**
 * @brief Method to run on core1 of pico to load
 cond_t;

/* \brief Initialize a condition variable structure
 *  \ingroup cond
 *
 * \param cv Pointer to condition variable structure
 */
void cond_init(cond_t *cv);

/* \brief Wait on a condition variable
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast. The mutex should
 * be owned and is released atomically. It is reacquired when this function
 * returns.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 */
void cond_wait(cond_t *cv, mutex_t *mtx);

/* \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param until The time after which to return if the condition variable was
 * not signaled.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_until(cond_t *cv, mutex_t *mtx, absolute_time_t until);

/* \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param timeout_ms The timeout in milliseconds.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_timeout_ms(cond_t *cv, mutex_t *mtx, uint32_t timeout_ms);

/* \brief Wait on a condition variable with a timeout.
 *  \ingroup cond
 *
 * Wait until a condition variable is signaled or broadcast until a given
 * time. The mutex is released atomically and reacquired even if the wait
 * timed out.
 *
 * \param cv Condition variable to wait on
 * \param mtx Currently held mutex
 * \param timeout_ms The timeout in microseconds.
 * \return true if the condition variable was signaled, false otherwise
 */
bool cond_wait_timeout_us(cond_t *cv, mutex_t *mtx, uint32_t timeout_us);

/* \brief Signal on a condition variable and wake the waiter
 *  \ingroup cond
 *
 * \param cv Condition variable to signal
 */
void cond_signal(cond_t *cv);

/* \brief Broadcast a condition variable and wake every waiters
 *  \ingroup cond
 *
 * \param cv Condition variable to signal
 */
void cond_broadcast(cond_t *cv);


#endif // LF_THREADED
#endif // LF_PICO_SUPPORT_H
