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
 * @brief Raspberry Pi Pico support for the C target of Lingua Franca 
 * Uses the pico-sdk which targets the lower level peripheral layer. 
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

// critical section struct binding
// TODO: maybe be more precise and use nvic interrupt mask
static critical_section_t _lf_crit_sec;
// semaphore used to notify if sleep was interupted by irq 
static semaphore_t _lf_sem_irq_event;
static uint32_t _lf_num_nested_critical_sections = 0;

/**
 * Initialize the LF clock. Must be called before using other clock-related APIs.
 */
void _lf_initialize_clock(void) {
    // init stdlib peripherals
    stdio_init_all();
    // init sync structs
    critical_section_init(&_lf_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
    multicore_reset_core1();
}

/**
 * Fetch the value of an internal (and platform-specific) physical clock and
 * store it in `t`. in nanoseconds
 *
 * Ideally, the underlying platform clock should be monotonic. However, the
 * core lib tries to enforce monotonicity at higher level APIs (see tag.h).
 * TODO: might want to use the RTC
 * @return 0 for success, or -1 for failure
 */
int _lf_clock_now(instant_t* t) {
    absolute_time_t now;
    uint64_t ns_from_boot;
    now = get_absolute_time();
    ns_from_boot = to_us_since_boot(now) * 1000;
    *t = (instant_t) ns_from_boot;
    return 0; 
}

/**
 * Pause execution for a given duration.
 * 
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration) {
    if (sleep_duration < 0) {
        return -1;
    }
    sleep_us((uint64_t) (sleep_duration / 1000));
    return 0;
}

/**
 * @brief Sleep until the given wakeup time.
 * 
 * @param wakeup_time The time instant at which to wake up.
 * @return int 0 if sleep completed, or -1 if it was interrupted.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time) {
    int ret_code = 0;
    if (wakeup_time < 0) {
        ret_code = -1;
        return ret_code;
    }
    absolute_time_t target;
    // reset semaphore to 0
    // TODO: leverage the semaphore permit number 
    sem_reset(&_lf_sem_irq_event, 0);
    target = from_us_since_boot((uint64_t) (wakeup_time / 1000));
    lf_critical_section_exit(env);
    // sleep till target or return on processor event
    if(sem_acquire_block_until(&_lf_sem_irq_event, target)) {
        ret_code = -1;
    }
    lf_critical_section_enter(env);
    return ret_code;
}
/*
* Critical sections are only provided for an unthreaded, single core
* runtime. In the unthreaded runtime, all interactions with core1 are through
* physical actions and interupts outside of the runtime.
*/
#ifdef LF_UNTHREADED

/**
 * Enter a critical section where logical time and the event queue are guaranteed
 * to not change unless they are changed within the critical section.
 * this can be implemented by disabling interrupts.
 * Users of this function must ensure that lf_init_critical_sections() is
 * called first and that lf_critical_section_exit() is called later.
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: needs to be used sparingly 
 */
int lf_disable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec)) {
        return 1;
    } 
    // disables irq and spin-locks core
    if (_lf_num_nested_critical_sections++ == 0) {
        critical_section_enter_blocking(&_lf_crit_sec);
    }
    return 0;
}

/**
 * Exit the critical section entered with lf_lock_time().
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: needs to be used sparingly, find a better way for event queue
 * mutual exclusion for embedded platforms. better leverage the nvic 
 */
int lf_enable_interrupts_nested() {
    if (!critical_section_is_initialized(&_lf_crit_sec) ||
        _lf_num_nested_critical_sections <= 0) {
        return 1;
    } 
    // restores system execution state
    if (--_lf_num_nested_critical_sections == 0) {
        critical_section_exit(&_lf_crit_sec);
    }
    return 0;
}

/**
 * Notify any listeners that an event has been created.
 * The caller should call lf_critical_section_enter() before calling this function.
 * @return 0 on success, platform-specific error number otherwise.
 */
int _lf_unthreaded_notify_of_event() {
    // un-block threads that acquired this binary semaphore 
    sem_release(&_lf_sem_irq_event);
    return 0;
}
#endif //LF_UNTHREADED

// For platforms with threading support, the following functions
// abstract the API so that the LF runtime remains portable.

#ifdef LF_THREADED
#warning "Baremetal threaded support only allows two threads of execution"

// If threaded is enabled set number of workers
// Compiler warning when NUMBER_OF_WORKERS > 2
#if !defined(NUMBER_OF_WORKERS) || NUMBER_OF_WORKERS==0
#undef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 2
#endif

static lf_function_t _lf_core0_worker;
static void *_lf_core0_args;

static lf_function_t _lf_core1_worker;
static void *_lf_core1_args;


void _rp2040_core1_entry() {
    void *res = _lf_core1_worker(_lf_core1_args);
    // use fifo and send result
    // after worker exit fill fifo with result and block
    while(multicore_fifo_wready()) {
        multicore_fifo_push_blocking((uint32_t) res);
    }
}

/**
 * @brief Get the number of cores on the host machine.
 * pico has two physical cores and runs only two worker threads 
 */
int lf_available_cores() {
    return 2;
}

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: learn more about function pointers and resolving this interface
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    // this method can only be invoked twice.
    // each case for each core is specially handled
    static uint32_t call_cnt = 0;
    if (call_cnt >= 2) { 
        return -1;
    }
    switch (call_cnt) {
        case 0:
            _lf_core0_worker = (lf_function_t) lf_thread;
            _lf_core0_args = arguments;
            *thread = CORE_0;
            // cant launch first core worker
            break;
        case 1:
            _lf_core1_worker = (lf_function_t) lf_thread;
            _lf_core1_args = arguments;
            *thread = CORE_1;
            // launch second core worker
            multicore_launch_core1(&_rp2040_core1_entry);
            break;
        default:
            return -1;
    } 
    call_cnt++;
    return 0;
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return if thread_return
 * is not NULL.
 * @param thread The thread.
 * @param thread_return A pointer to where to store the exit status of the thread.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_join(lf_thread_t thread, void** thread_return) {
    /// TODO: implement
    /// run the core0 worker method here till completion and fill thread return
    switch(thread) {
        case CORE_0:
            // start core0 worker, block until completion
            *thread_return = _lf_core0_worker(_lf_core0_args);
            break;
        case CORE_1:
            // use multicore fifo
            // remove any extraneous messages from fifo
            multicore_fifo_drain();
            // block until thread return value received from fifo
            *thread_return = (void *) multicore_fifo_pop_blocking();
            break;
        default:
            return -1;
    }
    return 0;
}

/**
 * Initialize a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex) {
    mutex_init(mutex);
    return 0;
}

/**
 * Lock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: should this block?
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    if (!mutex_is_initialized(mutex)) {
        return -1;
    }
    mutex_enter_blocking(mutex);
    return 0;
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    if (!mutex_is_initialized(mutex)) {
        return -1;
    }
    mutex_exit(mutex);
    return 0;
}

/**
 * Initialize a conditional variable.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    if (!mutex_is_initialized(mutex)) {
        return -1;
    }
    cond->mut = mutex; 
    cond_init(&(cond->cv));
    return 0;
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    cond_broadcast(&(cond->cv));
    return 0;
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    cond_signal(&(cond->cv));
    return 0;
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond) {
    mutex_enter_blocking(cond->mut);
    cond_wait(&(cond->cv), cond->mut);
    mutex_exit(cond->mut);
    return 0;
}

/**
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 *
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise.
 */
int lf_cond_timedwait(lf_cond_t* cond, instant_t absolute_time_ns) {
    absolute_time_t target;
    target = from_us_since_boot((uint64_t) (absolute_time_ns / 1000));
    mutex_enter_blocking(cond->mut);
    if (!cond_wait_until(&(cond->cv), cond->mut, target)) {
        mutex_exit(cond->mut);
        return LF_TIMEOUT; 
    }
    mutex_exit(cond->mut);
    return 0;
}

void cond_init(cond_t *cond) {
    lock_init(&cond->core, next_striped_spin_lock_num());
    cond->waiter = LOCK_INVALID_OWNER_ID;
    cond->broadcast_count = 0;
    cond->signaled = false;
    __mem_fence_release();
}

bool __time_critical_func(cond_wait_until)(cond_t *cond, mutex_t *mtx, absolute_time_t until) {
    bool success = true;
    lock_owner_id_t caller = lock_get_caller_owner_id();
    uint32_t save = save_and_disable_interrupts();
    // Acquire the mutex spin lock
    spin_lock_unsafe_blocking(mtx->core.spin_lock);
    assert(lock_is_owner_id_valid(mtx->owner));
    assert(caller == mtx->owner);

    // Mutex and cond spin locks can be the same as spin locks are attributed
    // using `next_striped_spin_lock_num()`. To avoid any deadlock, we only
    // acquire the condition variable spin lock if it is different from the
    // mutex spin lock
    bool same_spinlock = mtx->core.spin_lock == cond->core.spin_lock;

    // Acquire the condition variable spin_lock
    if (!same_spinlock) {
        spin_lock_unsafe_blocking(cond->core.spin_lock);
    }

    mtx->owner = LOCK_INVALID_OWNER_ID;

    uint64_t current_broadcast = cond->broadcast_count;

    if (lock_is_owner_id_valid(cond->waiter)) {
        // Release the mutex but without restoring interrupts and notify.
        if (!same_spinlock) {
            spin_unlock_unsafe(mtx->core.spin_lock);
        }

        // There is a valid owner of the condition variable: we are not the
        // first waiter.
        // First iteration: notify
        lock_internal_spin_unlock_with_notify(&cond->core, save);
        save = spin_lock_blocking(cond->core.spin_lock);
        // Further iterations: wait
        do {
            if (!lock_is_owner_id_valid(cond->waiter)) {
                break;
            }
            if (cond->broadcast_count != current_broadcast) {
                break;
            }
            if (is_at_the_end_of_time(until)) {
                lock_internal_spin_unlock_with_wait(&cond->core, save);
            } else {
                if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&cond->core, save, until)) {
                    // timed out
                    success = false;
                    break;
                }
            }
            save = spin_lock_blocking(cond->core.spin_lock);
        } while (true);
    } else {
        // Release the mutex but without restoring interrupts
        if (!same_spinlock) {
            uint32_t disabled_ints = save_and_disable_interrupts();
            lock_internal_spin_unlock_with_notify(&mtx->core, disabled_ints);
        }
    }

    if (success && cond->broadcast_count == current_broadcast) {
        cond->waiter = caller;

        // Wait for the signal
        do {
            if (cond->signaled) {
                cond->waiter = LOCK_INVALID_OWNER_ID;
                cond->signaled = false;
                break;
            }
            if (is_at_the_end_of_time(until)) {
                lock_internal_spin_unlock_with_wait(&cond->core, save);
            } else {
                if (lock_internal_spin_unlock_with_best_effort_wait_or_timeout(&cond->core, save, until)) {
                    // timed out
                    cond->waiter = LOCK_INVALID_OWNER_ID;
                    success = false;
                    break;
                }
            }
            save = spin_lock_blocking(cond->core.spin_lock);
        } while (true);
    }

    // We got the signal (or timed out)

    if (lock_is_owner_id_valid(mtx->owner)) {
        // Acquire the mutex spin lock and release the core spin lock.
        if (!same_spinlock) {
            spin_lock_unsafe_blocking(mtx->core.spin_lock);
            spin_unlock_unsafe(cond->core.spin_lock);
        }

        // Another core holds the mutex.
        // First iteration: notify
        lock_internal_spin_unlock_with_notify(&mtx->core, save);
        save = spin_lock_blocking(mtx->core.spin_lock);
        // Further iterations: wait
        do {
            if (!lock_is_owner_id_valid(mtx->owner)) {
                break;
            }
            // We always wait for the mutex.
            lock_internal_spin_unlock_with_wait(&mtx->core, save);
            save = spin_lock_blocking(mtx->core.spin_lock);
        } while (true);
    } else {
        // Acquire the mutex spin lock and release the core spin lock
        // with notify but without restoring interrupts
        if (!same_spinlock) {
            spin_lock_unsafe_blocking(mtx->core.spin_lock);
            uint32_t disabled_ints = save_and_disable_interrupts();
            lock_internal_spin_unlock_with_notify(&cond->core, disabled_ints);
        }
    }

    // Eventually hold the mutex.
    mtx->owner = caller;

    // Restore the interrupts now
    spin_unlock(mtx->core.spin_lock, save);

    return success;
}

bool __time_critical_func(cond_wait_timeout_ms)(cond_t *cond, mutex_t *mtx, uint32_t timeout_ms) {
    return cond_wait_until(cond, mtx, make_timeout_time_ms(timeout_ms));
}

bool __time_critical_func(cond_wait_timeout_us)(cond_t *cond, mutex_t *mtx, uint32_t timeout_us) {
    return cond_wait_until(cond, mtx, make_timeout_time_us(timeout_us));
}

void __time_critical_func(cond_wait)(cond_t *cond, mutex_t *mtx) {
    cond_wait_until(cond, mtx, at_the_end_of_time);
}

void __time_critical_func(cond_signal)(cond_t *cond) {
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    if (lock_is_owner_id_valid(cond->waiter)) {
        // We have a waiter, we can signal.
        cond->signaled = true;
        lock_internal_spin_unlock_with_notify(&cond->core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}

void __time_critical_func(cond_broadcast)(cond_t *cond) {
    uint32_t save = spin_lock_blocking(cond->core.spin_lock);
    if (lock_is_owner_id_valid(cond->waiter)) {
        // We have a waiter, we can broadcast.
        cond->signaled = true;
        cond->broadcast_count++;
        lock_internal_spin_unlock_with_notify(&cond->core, save);
    } else {
        spin_unlock(cond->core.spin_lock, save);
    }
}

#endif // LF_THREADED
#endif // PLATFORM_RP2040

