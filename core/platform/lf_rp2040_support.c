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
#include <pico/util/queue.h>
#include <pico/sync.h>

// unthreaded statics
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

// threaded statics
/**
 * binary semaphore used to synchronize 
 * used by thread join
 */
static mutex_t _lf_core_sync;
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

    critical_section_init(&_lf_crit_sec);
    sem_init(&_lf_sem_irq_event, 0, 1);
    // only init sync mutex in multicore
#ifdef LF_THREADED
    mutex_init(&_lf_core_sync);
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
    int64_t ns_from_boot;

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

#ifdef LF_THREADED
// FIXME: add validator check that invalidates threaded rp2040 
// lf programs with more than 2 workers set
#undef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 2

static lf_function_t _lf_core0_worker, _lf_core1_worker;
static void *_lf_core0_args, *_lf_core1_args;


void _rp2040_core1_entry() {
    // lock sync lock
    mutex_enter_blocking(&_lf_core_sync);
    void *res = _lf_core1_worker(_lf_core1_args);
    // unlock sync lock
    mutex_exit(&_lf_core_sync);
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
      LF_PRINT_DEBUG("rp2040: invalid thread create id");
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
      // lock sync lock
      mutex_enter_blocking(&_lf_core_sync);
      break;
    default:
      LF_PRINT_DEBUG("rp2040: invalid thread join id");
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
    // reference to mutex
    cond->mutex = mutex;
    // init queue, use core num as debug info
    queue_init(&cond->signal, sizeof(uint32_t), 2);
    return 0;
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    // release all permit
    // add to queue, non blocking
    uint32_t core = get_core_num();
    if (!queue_try_add(&cond->signal, &core)) {
        return -1; 
    }
    return queue_try_add(&cond->signal, &core) ? 0 : -1;
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond) {
    // release permit
    // add to queue, non blocking
    uint32_t core = get_core_num();
    return queue_try_add(&cond->signal, &core) ? 0 : -1;
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond) {
    uint32_t cur_core, queue_core;
    cur_core = get_core_num();
    // unlock mutex
    // todo: atomically unlock
    lf_mutex_unlock(cond->mutex);
    // queue remove blocking
    queue_remove_blocking(&cond->signal, &queue_core);
    // debug: check calling core
    if (cur_core == queue_core) {
        LF_PRINT_DEBUG("rp2040: self core cond release");
    }
    lf_mutex_lock(cond->mutex);
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
    uint32_t cur_core, queue_core;
    cur_core = get_core_num();
    // unlock mutex
    lf_mutex_unlock(cond->mutex);
    absolute_time_t target, now;
    // check timeout
    if (absolute_time_ns < 0) {
        return LF_TIMEOUT;
    }
    // target and cur time
    target = from_us_since_boot((uint64_t) (absolute_time_ns / 1000));
    now = get_absolute_time();
    while (!queue_try_remove(&cond->signal, &queue_core)) {
        // todo: cases where this might overflow
        if (absolute_time_diff_us(now, target) < 0) {
            lf_mutex_lock(cond->mutex);
            return LF_TIMEOUT; 
        }
        now = get_absolute_time();
    }
    if (cur_core == queue_core) {
        LF_PRINT_DEBUG("rp2040: self core cond release");
    }
    lf_mutex_lock(cond->mutex);
    return 0;
}

/**
 * Atomics for the rp2040 platform.
 * note: uses the same implementation as the zephyr platform 
 * TODO: explore more efficent options
 */
/**
 * @brief Add `value` to `*ptr` and return original value of `*ptr` 
 */
int _rp2040_atomic_fetch_add(int *ptr, int value) {
    lf_disable_interrupts_nested();
    int res = *ptr;
    *ptr += value;
    lf_enable_interrupts_nested();
    return res;
}
/**
 * @brief Add `value` to `*ptr` and return new updated value of `*ptr`
 */
int _rp2040_atomic_add_fetch(int *ptr, int value) {
    lf_disable_interrupts_nested();
    int res = *ptr + value;
    *ptr = res;
    lf_enable_interrupts_nested();
    return res;
}

/**
 * @brief Compare and swap for boolaen value.
 * If `*ptr` is equal to `value` then overwrite it 
 * with `newval`. If not do nothing. Retruns true on overwrite.
 */
bool _rp2040_bool_compare_and_swap(bool *ptr, bool value, bool newval) {
    lf_disable_interrupts_nested();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_enable_interrupts_nested();
    return res;
}

/**
 * @brief Compare and swap for integers. If `*ptr` is equal
 * to `value`, it is updated to `newval`. The function returns
 * the original value of `*ptr`.
 */
int  _rp2040_val_compare_and_swap(int *ptr, int value, int newval) {
    lf_disable_interrupts_nested();
    int res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    lf_enable_interrupts_nested();
    return res;
}

#endif //LF_THREADED
#endif // PLATFORM_RP2040

