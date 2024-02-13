/* Platform API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of California at Berkeley.

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
 * Platform API support for the C target of Lingua Franca.
 * This file detects the platform on which the C compiler is being run
 * (e.g. Windows, Linux, Mac) and conditionally includes platform-specific
 * files that define core datatypes and function signatures for Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tag.h"
#include <assert.h>
#include "lf_atomic.h"

// Forward declarations
typedef struct environment_t environment_t;

/**
 * @brief Notify of new event.
 * @param env Environment in which we are executing.
 */
int lf_notify_of_event(environment_t* env);

/**
 * @brief Enter critical section within an environment.
 * @param env Environment in which we are executing.
 */
int lf_critical_section_enter(environment_t* env);

/**
 * @brief Leave a critical section within an environment.
 * @param env Environment in which we are executing.
 */
int lf_critical_section_exit(environment_t* env);



#if defined(PLATFORM_ARDUINO)
    #include "platform/lf_arduino_support.h"
#elif defined(PLATFORM_ZEPHYR)
    #include "platform/lf_zephyr_support.h"
#elif defined(PLATFORM_NRF52)
    #include "platform/lf_nrf52_support.h"
#elif defined(PLATFORM_RP2040)
    #include "platform/lf_rp2040_support.h"
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   // Windows platforms
   #include "lf_windows_support.h"
#elif __APPLE__
    // Apple platforms
    #include "lf_macos_support.h"
#elif __linux__
    // Linux
    #include "lf_linux_support.h"
#elif __unix__ // all unices not caught above
    // Unix
    #include "lf_POSIX_threads_support.h"
#elif defined(_POSIX_VERSION)
    // POSIX
    #include "lf_POSIX_threads_support.h"
#elif defined(__riscv) || defined(__riscv__)
    // RISC-V (see https://github.com/riscv/riscv-toolchain-conventions)
    #error "RISC-V not supported"
#else
#error "Platform not supported"
#endif

#define LF_TIMEOUT 1


// To support the single-threaded runtime, we need the following functions. They
//  are not required by the threaded runtime and is thus hidden behind a #ifdef.
#if defined (LF_SINGLE_THREADED)
    typedef void lf_mutex_t;
    /** 
     * @brief Disable interrupts with support for nested calls
     * @return 0 on success
     */
    int lf_disable_interrupts_nested();
    /**
     * @brief  Enable interrupts after potentially multiple callse to `lf_disable_interrupts_nested`
     * @return 0 on success
     */
    int lf_enable_interrupts_nested();

    /**
     * @brief Notify sleeping single-threaded context of new event
     * @return 0 on success
     */
    int _lf_single_threaded_notify_of_event();
#else 
// For platforms with threading support, the following functions
// abstract the API so that the LF runtime remains portable.

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores();

/**
 * Returns the thread ID of the calling thread
 * 
 */
lf_thread_t lf_thread_self();

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 *
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments);

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return if thread_return
 * is not NULL.
 * @param thread The thread.
 * @param thread_return A pointer to where to store the exit status of the thread.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_join(lf_thread_t thread, void** thread_return);


// The following API introduce the ability to change how the LF workers are sheduled
// by the underlying thread scheduling. This API is still experimental and future
// changes are expected.

#define LF_SCHED_MAX_PRIORITY 99
#define LF_SCHED_MIN_PRIORITY 0
/**
 * @brief The thread scheduling policies. 
 * 
 */
typedef enum {
    LF_SCHED_FAIR, // Non real-time scheduling policy. Corresponds to SCHED_OTHER
    LF_SCHED_TIMESLICE, // Real-time, time-slicing priority-based policty. Corresponds to SCHED_RR.
    LF_SCHED_PRIORITY, // Real-time, priority-only based scheduling. Corresponds to SCHED_FIFO.
} lf_scheduling_policy_type_t;

typedef struct {
    lf_scheduling_policy_type_t policy;
} lf_scheduling_policy_t;

typedef struct {
    lf_scheduling_policy_t base;
    int priority;
    interval_t time_slice;
} lf_scheduling_policy_timeslice_t;

typedef struct {
    lf_scheduling_policy_t base;
    int priority;
} lf_scheduling_policy_priority_t;

/**
 * This pins a lf_thread to a specific CPU
 * 
 * @param thread The thread 
 * @param cpu_number the CPU ID
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_set_cpu(lf_thread_t thread, int cpu_number);

/**
 * Sets the priority of a thread. Priority ranges from 0 to 99 where a higher
 * number indicates higher priority. Setting the priority of a thread only
 * makes sense if the thread is scheduled with LF_SCHED_TIMESLICE or LF_THREAD_PRIORITY
 *
 * @param thread The thread.
 * @param priority The priority.
 * @return int 0 on success, platform-specific error otherwise
 */
int lf_thread_set_priority(lf_thread_t thread, int priority);

/**
 * Sets the scheduling policy of a thread.
 *  
 * @return int 0 on success, platform-specific error number otherwise.
 */
int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t *policy);

/**
 * Initialize a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex);

/**
 * Lock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_lock(lf_mutex_t* mutex);

/**
 * Unlock a mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex);

/**
 * Initialize a conditional variable.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex);

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond);

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond);

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond);

/**
 * Block the current thread on the condition variable until the condition variable
 * pointed by "cond" is signaled or the time given by wakeup_time is reached. This should
 * not be used directly as it does not account for clock synchronization offsets.
 * Use `lf_clock_cond_timedwait` from clock.h instead.
 *
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise.
 */
int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time);
#endif

/**
 * Initialize the LF clock. Must be called before using other clock-related APIs.
 */
void _lf_initialize_clock(void);

/**
 * Fetch the value of an internal (and platform-specific) physical clock.
 * Ideally, the underlying platform clock should be monotonic. However, the core
 * lib enforces monotonicity at higher level APIs (see clock.h).
 * 
 * This should not be used directly as it does not apply clock synchronization
 * offsets.
 *
 * @return 0 for success, or -1 for failure
 */
int _lf_clock_gettime(instant_t* t);

/**
 * Pause execution for a given duration.
 * 
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration);

/**
 * @brief Sleep until the given wakeup time. This should not be used directly as it
 * does not account for clock synchronization offsets. See clock.h.
 *
 * This assumes the lock for the given environment is held.
 *
 * @param env The environment within which to sleep.
 * @param wakeup_time The time instant at which to wake up.
 * @return int 0 if sleep completed, or -1 if it was interrupted.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time);

/**
 * Macros for marking function as deprecated
 */
#ifdef __GNUC__
    #define DEPRECATED(X) X __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define DEPRECATED(X) __declspec(deprecated) X
#else
    #define DEPRECATED(X) X
#endif

/**
 * @deprecated version of "lf_sleep"
 */
DEPRECATED(int lf_nanosleep(interval_t sleep_duration));

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H
