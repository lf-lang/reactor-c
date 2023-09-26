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
 * binary semaphore for lf event notification 
 * used by external isr or second core thread.
 * used to interact with the lf runtime thread
 */
static semaphore_t _lf_sem_irq_event;

// nested critical section counter
static uint32_t _lf_num_nested_crit_sec = 0;
 
/**
 * binary semaphore used to synchronize 
 * used by thread join
 */
static semaphore_t _lf_sem_core_sync;
/**
 * track number of threads created 
 * error on value greater than 2
 */
static uint8_t _lf_thread_cnt = 0;

/**
 * Initialize basic runtime infrastructure and 
 * synchronization structs for an unthreaded runtime.
 */
void _lf_initialize_clock(void) {
    // init stdio lib
    // for debug printf
    stdio_init_all();

#ifdef LF_UNTHREADED
    critical_section_init(&_lf_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
#endif //LF_UNTHREADED

#ifdef LF_THREADED
    sem_init(&_lf_sem_core_sync, 0, 1);
#endif //LF_THREADED
}

/**
 * Write the time since boot in nanoseconds into 
 * the time variable pointed to by the argument
 * and return 0.
 * 
 * @param  t  pointer to the time variable to write to.
 * @return error code or 0 on success. 
 */
int _lf_clock_now(instant_t* t) {
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
 * The semaphore is released using the _lf_unthreaded_notify_of_event
 * which is called by lf_schedule in the unthreaded runtime for physical actions.
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
    lf_critical_section_exit(env);
    // blocked sleep
    // return on timeout or on processor event
    if(sem_acquire_block_until(&_lf_sem_irq_event, target)) {
        ret_code = -1;
    }
    // remove interrupts
    lf_critical_section_enter(env);
    return ret_code;
}

#ifdef LF_UNTHREADED
/**
 * The single thread RP2040 platform support treats second core
 * routines similar to external interrupt routine threads.
 * 
 * Second core activity is disabled at the same times as
 * when interrupts are disabled. 
 */

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

/**
 * Release the binary event semaphore to notify
 * the runtime of a physical action being scheduled.
 *
 * @return error code or 0 on success
 */
int _lf_unthreaded_notify_of_event() {
    // notify main sleep loop of event
    sem_release(&_lf_sem_irq_event);
    return 0;
}
#endif //LF_UNTHREADED

#ifdef LF_THREADED
// FIXME: add validator check that invalidates threaded rp2040 
// lf programs with more than 2 workers set
#undef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 2

static lf_function_t _lf_core0_worker, _lf_core1_worker;
static void *_lf_core0_args, *_lf_core1_args;


void _rp2040_core1_entry() {
    void *res = _lf_core1_worker(_lf_core1_args);
    // notify of completion
    sem_release(&_lf_sem_core_sync);
}

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores() {
    return 2;
}

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 *
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    if (_lf_thread_cnt == 0) {
        *thread = RP2040_CORE_0;
        _lf_core0_worker = (lf_function_t) lf_thread;
        _lf_core0_args = arguments;
    } else if (_lf_thread_cnt == 1) {
        *thread = RP2040_CORE_1;
        _lf_core1_worker = (lf_function_t) lf_thread;
        _lf_core1_args = arguments;
        multicore_launch_core1(_rp2040_core1_entry);
    } else {
      // invalid thread
      LF_PRINT_DEBUG("RP2040 threaded: tried to create invalid core thread");
      return -1;
    }
    _lf_thread_cnt++;
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
  switch(thread) {
    case RP2040_CORE_0:
      // run core0 worker
      // block until completion
      *thread_return = _lf_core0_worker(_lf_core0_args);
      break;
    case RP2040_CORE_1:
      // block until core sync semaphore released
      sem_acquire_blocking(&_lf_sem_core_sync);
      break;
    default:
      LF_PRINT_DEBUG("RP2040 threaded: tried to join invalid core thread");
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
    recursive_mutex_init(mutex);
    return 0;
}
  
/**
 * Lock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    recursive_mutex_enter_blocking(mutex);
    return 0;
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    recursive_mutex_exit(mutex);
    return 0;
}


// FIXME: bugged since cond variables
// have different behavior compared to a sempahore

/**
 * Initialize a conditional variable.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    // set max permits to number of cores
    sem_init(cond, 0, 2);
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    // release all permits
    while(sem_release(cond));
    return 0;
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    if(sem_release(cond)) { return 0; }
    return -1;
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

#endif //LF_THREADED
#endif // PLATFORM_RP2040

