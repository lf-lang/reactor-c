/**
 * @file
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @brief Platform API support for the C target of Lingua Franca.
 * @copyright (c) 2020-2024, The University of California at Berkeley.
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 *
 * This file detects the platform on which the C compiler is being run
 * (e.g. Windows, Linux, Mac) and conditionally includes platform-specific
 * files that define core datatypes and function signatures for Lingua Franca.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tag.h"
#include <assert.h>
#include "platform/lf_atomic.h"

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
   #include "platform/lf_windows_support.h"
#elif __APPLE__
    // Apple platforms
    #include "platform/lf_macos_support.h"
#elif __linux__
    // Linux
    #include "platform/lf_linux_support.h"
#elif __unix__ // all unices not caught above
    // Unix
    #include "platform/lf_POSIX_threads_support.h"
#elif defined(_POSIX_VERSION)
    // POSIX
    #include "platform/lf_POSIX_threads_support.h"
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

    int lf_mutex_unlock(lf_mutex_t* mutex);
    int lf_mutex_init(lf_mutex_t* mutex);
    int lf_mutex_lock(lf_mutex_t* mutex);
#else
// For platforms with threading support, the following functions
// abstract the API so that the LF runtime remains portable.

/**
 * @brief Get the number of cores on the host machine.
 */
int lf_available_cores();

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in thread_id.
 *
 * @return 0 on success, platform-specific error number otherwise.
 *
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments);

/**
 * @brief Helper function for creating a thread.
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

/**
 * @brief Cross-platform version of the C11 thread_local keyword.
 */
#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif // thread_local

/**
 * @brief The ID of the current thread. The only guarantee is that these IDs will be a contiguous range of numbers starting at 0.
 */
int lf_thread_id();

/**
 * @brief Initialize the thread ID for the current thread.
 */
void initialize_lf_thread_id();
#endif  // !defined(LF_SINGLE_THREADED)

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
