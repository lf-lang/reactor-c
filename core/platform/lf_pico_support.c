#ifdef PLATFORM_PICO
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
 * Uses the baremetal pico-sdk 
 * 
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 */

#include "lf_pico_support.h"
#include "platform.h"
#include "utils/util.h"
#include "tag.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sync.h>

#ifdef LF_UNTHREADED
// critical section struct binding
static critical_section_t _lf_crit_sec;
// semaphore used to notify if sleep was interupted by irq 
static semaphore_t _lf_sem_irq_event;
static uint32_t _lf_num_nested_critical_sections = 0;
#endif

/**
 * Initialize the LF clock. Must be called before using other clock-related APIs.
 */
void lf_initialize_clock(void) {
    // init stdlib peripherals
    stdio_init_all();
    // init sync structs
    critical_section_init(&_lf_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
    // TODO: use core1
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
int lf_clock_gettime(instant_t* t) {
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
int lf_sleep_until_locked(instant_t wakeup_time) {
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
    lf_critical_section_exit(&_lf_crit_sec);
    // sleep till target or return on processor event
    if(sem_acquire_block_until(&_lf_sem_irq_event, target)) {
        ret_code = -1;
    }
    lf_critical_section_enter(&_lf_crit_sec);
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
int lf_critical_section_enter() {
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
int lf_critical_section_exit() {
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
int lf_notify_of_event() {
    // un-block threads that acquired this binary semaphore 
    sem_release(&_lf_sem_irq_event);
    return 0;
}
#endif

// For platforms with threading support, the following functions
// abstract the API so that the LF runtime remains portable.

#ifdef LF_THREADED
/**
 * @brief Get the number of cores on the host machine.
 * pico has two physical cores and runs only two worker threads 
 */
int lf_available_cores() {
    return 2;
}

void _pico_core_loader() {
    // create method that executes provided
    // functions with specific through a comunicating api call
    // the api call will be used by thread create and join
    // alternatively use free-rtos an launch tasks
    /// TODO: create a dispatcher program that runs on the second core similar to rtic
    /// TODO: maybe assigning an enclave to core1 is the best path forward to avoid
    // reimplementing a threading library
}

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: learn more about function pointers and resolving this interface
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    /// TODO: wrap in secondary function that takes these arguments
    /// run that function on core1 with provided args 
    // multicore_launch_core1(lf_thread);
    // fill thread instance
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
}

/**
 * Initialize a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex) {
    recursive_mutex_init(mutex);
    return 0;
}

/**
 * Lock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * TODO: should this block?
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    if (!recursive_mutex_is_initialized(mutex)) {
        return -1;
    }
    recursive_mutex_enter_blocking(mutex);
    return 0;
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    if (!recursive_mutex_is_initialized(mutex)) {
        return -1;
    }
    recursive_mutex_exit(mutex);
    return 0;
}

/**
 * Initialize a conditional variable.
 * @return 0 on success, platform-specific error number otherwise.
 * /// TODO: mutex here not used 
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    // set max permits to number of threads
    sem_init(cond, 0, NUMBER_OF_WORKERS);
    return 0;
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    // release all permits
    while(sem_release(cond));
    // check all released
    if (sem_available(cond) != NUMBER_OF_WORKERS) {
        return -1;
    }
    return 0;
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    if(!sem_release(cond)) {
        return -1;
    }
    return 0;
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond) {
    sem_acquire_blocking(cond);
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
    if (!sem_acquire_block_until(cond, target)) {
        return LF_TIMEOUT; 
    }
    return 0;
}

#endif // LF_THREADED
#endif // PLATFORM_PICO

