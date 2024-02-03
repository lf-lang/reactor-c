#if defined(PLATFORM_RP2040)
/*************
Copyright (c) 2022, The University of California at Berkeley.

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
***************/

/**
 * @brief RP2040 mcu support for the C target of Lingua Franca. 
 * This utilizes the pico-sdk which provides C methods for a light runtime
 * and a hardware abstraction layer.
 * 
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 */

#include "lf_rp2040_support.h"
#include "platform.h"
#include "utils/util.h"
#include "tag.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>

/** 
 * critical section struct
 * disables external irq and core execution
 * provides mutual exclusion using hardware spin-locks
 */
static critical_section_t _lf_crit_sec;

/**
 * critical section struct for atomics implementation
 */
static critical_section_t _lf_atomics_crit_sec;

/**
 * binary semaphore for lf event notification 
 * used by external isr or second core thread.
 * used to interact with the lf runtime thread
 */
static semaphore_t _lf_sem_irq_event;

// nested critical section counter
static uint32_t _lf_num_nested_crit_sec = 0;

/**
 * Initialize basic runtime infrastructure and 
 * synchronization structs for a single-threaded runtime.
 */
void _lf_initialize_clock(void) {
    // init stdio lib
    stdio_init_all();
    // init sync structs
    critical_section_init(&_lf_crit_sec);
    critical_section_init(&_lf_atomics_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
}

/**
 * Write the time since boot in nanoseconds into 
 * the time variable pointed to by the argument
 * and return 0.
 * 
 * @param  t  pointer to the time variable to write to.
 * @return error code or 0 on success. 
 */
int _lf_clock_gettime(instant_t* t) {
    if (!t) {
        return -1;
    }
    // time struct
    absolute_time_t now;
    uint64_t ns_from_boot;

    now = get_absolute_time();
    ns_from_boot = to_us_since_boot(now) * 1000;
    *t = (instant_t) ns_from_boot;
    return 0; 
}

/**
 * Pause execution of the calling core for 
 * a nanosecond duration specified by the argument.
 * Floor the specified duration to the nearest microsecond
 * duration before sleeping and return 0.
 *
 * @param  sleep_duration  time to sleep in nanoseconds
 * @return error code or 0 on success
 */ 
int lf_sleep(interval_t sleep_duration) {
    if (sleep_duration < 0) {
        return -1;
    }
    sleep_us((uint64_t) (sleep_duration / 1000));
    return 0;
}

/**
 * Sleep until the target time since boot in nanoseconds provided
 * by the argument or return early if the binary 
 * _lf_sem_irq_event semaphore is released before the target time.
 *
 * The semaphore is released using the _lf_single_threaded_notify_of_event
 * which is called by lf_schedule in the single_threaded runtime for physical actions.
 *
 * @param  env  pointer to environment struct this runs in.
 * @param  wakeup_time  time in nanoseconds since boot to sleep until.
 * @return -1 when interrupted or 0 on successful timeout
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    int ret_code = 0;
    // return immediately
    if (wakeup_time < 0) {
        return ret_code;
    }
    // time struct
    absolute_time_t target;
    
    // reset event semaphore 
    sem_reset(&_lf_sem_irq_event, 0);
    // create us boot wakeup time
    target = from_us_since_boot((uint64_t) (wakeup_time / 1000));
    // allow interrupts
    LF_CRITICAL_SECTION_EXIT(env);
    // blocked sleep
    // return on timeout or on processor event
    if(sem_acquire_block_until(&_lf_sem_irq_event, target)) {
        ret_code = -1;
    }
    // remove interrupts
    LF_CRITICAL_SECTION_ENTER(env);
    return ret_code;
}

/**
 * Enter a critical section where the second core is disabled
 * and interrupts are disabled. Enter only if the critical section
 * hasn't previously been entered.
 *
 * @return error code or 0 on success
 */
int lf_disable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec)) {
        return 1;
    }
    // check crit sec count
    // enter non-rentrant state by disabling interrupts
    // lock second core execution
    if (_lf_num_nested_crit_sec == 0) {
        // block if associated spin lock in use
        critical_section_enter_blocking(&_lf_crit_sec);
    }
    // add crit sec count
    _lf_num_nested_crit_sec++;
    return 0;
}

/**
 * Exit a critical section which will resume second core 
 * execution and enable interrupts. 
 * Exit only if no other critical sections are left to exit.
 *
 * @return error code or 0 on success
 */
int lf_enable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec) ||
        _lf_num_nested_crit_sec <= 0) {
        return 1;
    }
    // remove from crit sec count
    _lf_num_nested_crit_sec--;
    // check crit sec count
    // release spin-lock
    if (_lf_num_nested_crit_sec == 0) {
        critical_section_exit(&_lf_crit_sec);
    }
    return 0;
}

#if defined(LF_SINGLE_THREADED)
/**
 * Release the binary event semaphore to notify
 * the runtime of a physical action being scheduled.
 *
 * @return error code or 0 on success
 */
int _lf_single_threaded_notify_of_event() {
    // notify main sleep loop of event
    sem_release(&_lf_sem_irq_event);
    return 0;
}

#else // LF_SINGLE_THREADED

#warning "Threaded runtime on RP2040 is still experimental"

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores() {
    // Right now, runtime creates 1 main thread and 1 worker thread
    // In the future, this may be changed to 2 (if main thread also functions
    // as a worker thread)
    return 1;
}

static void *(*thread_1) (void *);
static void* thread_1_args;
static int num_create_threads_called = 0;
static semaphore_t thread_1_done;
static void* thread_1_return;

#define MAGIC_THREAD1_ID 314159

void core1_entry() {
    thread_1_return = thread_1(thread_1_args);
    sem_release(&thread_1_done);
}

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    // make sure this fn is only called once
    if (num_create_threads_called != 0) {
        return 1;
    }
    thread_1 = lf_thread;
    thread_1_args = arguments;
    num_create_threads_called += 1;
    sem_init(&thread_1_done, 0, 1);
    multicore_launch_core1(core1_entry);
    *thread = MAGIC_THREAD1_ID;
    return 0;
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
    if (thread != MAGIC_THREAD1_ID) {
        return 1;
    }
    sem_acquire_blocking(&thread_1_done);
    sem_release(&thread_1_done); // in case join is called again
    if (thread_return) {
        *thread_return = thread_1_return;
    }
    return 0;
}

int lf_mutex_init(lf_mutex_t* mutex) {
    mutex_init(mutex);
    return 0;
}

int lf_mutex_lock(lf_mutex_t* mutex) {
    mutex_enter_blocking(mutex);
    return 0;
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
    mutex_exit(mutex);
    return 0;
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    sem_init(&(cond->sema), 0, 1);
    cond->mutex = mutex;
    return 0;
}

int lf_cond_broadcast(lf_cond_t* cond) {
    sem_reset(&(cond->sema), 1);
    return 0;
}

int lf_cond_signal(lf_cond_t* cond) {
    sem_reset(&(cond->sema), 1);
    return 0;
}

int lf_cond_wait(lf_cond_t* cond) {
    mutex_exit(cond->mutex);
    sem_acquire_blocking(&(cond->sema));
    mutex_enter_blocking(cond->mutex);
    return 0;
}

int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    absolute_time_t a = from_us_since_boot(absolute_time_ns / 1000);
    bool acquired_permit = sem_acquire_block_until(&(cond->sema), a);
    return acquired_permit ? 0 : LF_TIMEOUT;
}


// Atomics
//  Implemented by just entering a critical section and doing the arithmetic.
//  This is somewhat inefficient considering enclaves. Since we get a critical
//  section in between different enclaves


/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr`
 */
int _rp2040_atomic_fetch_add(int *ptr, int value) {
    critical_section_enter_blocking(&_lf_atomics_crit_sec);
    int res = *ptr;
    *ptr += value;
    critical_section_exit(&_lf_atomics_crit_sec);
    return res;
}
/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _rp2040_atomic_add_fetch(int *ptr, int value) {
    critical_section_enter_blocking(&_lf_atomics_crit_sec);
    int res = *ptr + value;
    *ptr = res;
    critical_section_exit(&_lf_atomics_crit_sec);
    return res;
}

/**
 * @brief Compare and swap for boolaen value.
 * If `*ptr` is equal to `value` then overwrite it
 * with `newval`. If not do nothing. Retruns true on overwrite.
 */
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval) {
    critical_section_enter_blocking(&_lf_atomics_crit_sec);
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    critical_section_exit(&_lf_atomics_crit_sec);
    return res;
}

/**
 * @brief Compare and swap for integers. If `*ptr` is equal
 * to `value`, it is updated to `newval`. The function returns
 * the original value of `*ptr`.
 */
int  _rp2040_val_compare_and_swap(int *ptr, int value, int newval) {
    critical_section_enter_blocking(&_lf_atomics_crit_sec);
    int res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    critical_section_exit(&_lf_atomics_crit_sec);
    return res;
}



#endif // LF_SINGLE_THREADED


#endif // PLATFORM_RP2040

