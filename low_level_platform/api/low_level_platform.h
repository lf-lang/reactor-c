/**
 * @file low_level_platform.h
 * @brief Platform API support for the C target of Lingua Franca.
 *
 * @ingroup Platform
 *
 * @author Soroush Bateni
 *
 * This file defines functions that need to be implemented for any new platform
 * (operating system or bare-metal SDK).
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
 * @ingroup Platform
 * @param env Environment in which we are executing.
 */
int lf_notify_of_event(environment_t* env);

/**
 * @brief Enter critical section within an environment.
 * @ingroup Platform
 * @param env Environment in which we are executing.
 */
int lf_critical_section_enter(environment_t* env);

/**
 * @brief Leave a critical section within an environment.
 * @ingroup Platform
 * @param env Environment in which we are executing.
 */
int lf_critical_section_exit(environment_t* env);

#if defined(PLATFORM_ARDUINO)
#include "platform/lf_arduino_support.h"
#elif defined(PLATFORM_ZEPHYR)
#include "platform/lf_zephyr_support.h"
#elif defined(PLATFORM_NRF52)
#include "platform/lf_nrf52_support.h"
#elif defined(PLATFORM_PATMOS)
#include "platform/lf_patmos_support.h"
#elif defined(PLATFORM_RP2040)
#include "platform/lf_rp2040_support.h"
#elif defined(PLATFORM_FLEXPRET)
#include "platform/lf_flexpret_support.h"
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

// Worker priorities range from 0 to 99 where 99 is the highest priority.
#define LF_SCHED_MAX_PRIORITY 99
#define LF_SCHED_MIN_PRIORITY 0

// To support the single-threaded runtime, we need the following functions. They
//  are not required by the threaded runtime and is thus hidden behind a #ifdef.
#if defined(LF_SINGLE_THREADED)

// For unthreaded platforms, the mutex functions below are implemented in reactor.c
// and do nothing, returning 0.
typedef void* lf_mutex_t;
int lf_mutex_unlock(lf_mutex_t* mutex);
int lf_mutex_init(lf_mutex_t* mutex);
int lf_mutex_lock(lf_mutex_t* mutex);

/**
 * @brief Disable interrupts with support for nested calls
 * @ingroup Platform
 * @return 0 on success
 */
int lf_disable_interrupts_nested(void);

/**
 * @brief  Enable interrupts after potentially multiple callse to `lf_disable_interrupts_nested`
 * @ingroup Platform
 * @return 0 on success
 */
int lf_enable_interrupts_nested(void);

/**
 * @brief Notify sleeping single-threaded context of new event
 * @ingroup Platform
 * @return 0 on success
 */
int _lf_single_threaded_notify_of_event(void);

#else // defined(LF_SINGLE_THREADED)

// For platforms with threading support, the following functions
// abstract the API so that the LF runtime remains portable.

/**
 * @brief Get the number of cores on the host machine.
 * @ingroup Platform
 */
int lf_available_cores(void);

/**
 * @brief Return the lf_thread_t of the calling thread.
 * @ingroup Platform
 */
lf_thread_t lf_thread_self(void);

/**
 * @brief Create a new thread.
 * @ingroup Platform
 *
 * Start execution of the specified function with the passed arguments.
 * The new thread handle is stored in thread_id.
 * @param thread A pointer to where to store the handle.
 * @param lf_thread The function to invoke.
 * @param arguments The arguments to pass to the function.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_create(lf_thread_t* thread, void* (*lf_thread)(void*), void* arguments);

/**
 * @brief Wait for the specified thread to exit.
 * @ingroup Platform
 *
 * Make the calling thread wait for termination of the specified thread.  The
 * exit status of the thread is stored in thread_return if thread_return
 * is not NULL.
 * @param thread The thread.
 * @param thread_return A pointer to where to store the exit status of the thread.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_join(lf_thread_t thread, void** thread_return);

/**
 * @brief The thread scheduling policies.
 * @ingroup Platform
 */
typedef enum {
  LF_SCHED_FAIR,      // Non real-time scheduling policy. Corresponds to SCHED_OTHER
  LF_SCHED_TIMESLICE, // Real-time, time-slicing priority-based policty. Corresponds to SCHED_RR.
  LF_SCHED_PRIORITY,  // Real-time, priority-only based scheduling. Corresponds to SCHED_FIFO.
} lf_scheduling_policy_type_t;

/**
 * @brief A struct supporting thread scheduling policies.
 * @ingroup Platform
 */
typedef struct {
  lf_scheduling_policy_type_t policy; // The scheduling policy
  int priority;                       // The priority, if applicable
  interval_t time_slice;              // The time-slice allocated, if applicable.
} lf_scheduling_policy_t;

/**
 * @brief Pin a thread to a specific CPU.
 * @ingroup Platform
 *
 * @param thread The thread
 * @param cpu_number the CPU ID
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_thread_set_cpu(lf_thread_t thread, size_t cpu_number);

/**
 * @brief Set the priority of a thread.
 * @ingroup Platform
 *
 * Priority ranges from @ref LF_SCHED_MIN_PRIORITY to @ref LF_SCHED_MAX_PRIORITY, where a higher
 * number indicates higher priority. Setting the priority of a thread only
 * makes sense if the thread is scheduled with @ref LF_SCHED_TIMESLICE or @ref LF_SCHED_PRIORITY.
 *
 * @param thread The thread.
 * @param priority The priority.
 * @return int 0 on success, platform-specific error otherwise
 */
int lf_thread_set_priority(lf_thread_t thread, int priority);

/**
 * @brief Set the scheduling policy of a thread.
 * @ingroup Platform
 *
 * This is based on the scheduling
 * concept from Linux explained here: https://man7.org/linux/man-pages/man7/sched.7.html
 * A scheduling policy is specific to a thread/worker. We have three policies
 * @ref LF_SCHED_PRIORITY, which corresponds to SCHED_FIFO on Linux.
 * @ref LF_SCHED_TIMESLICE, which corresponds to SCHED_RR on Linux.
 * @ref LF_SCHED_FAIR, which corresponds to SCHED_OTHER on Linux.
 *
 * @return int 0 on success, platform-specific error number otherwise.
 */
int lf_thread_set_scheduling_policy(lf_thread_t thread, lf_scheduling_policy_t* policy);

/**
 * @brief Initialize a mutex.
 * @ingroup Platform
 *
 * @param mutex The mutex
 * @return 0 on success
 */
int lf_mutex_init(lf_mutex_t* mutex);

/**
 * @brief Lock the specified mutex.
 * @ingroup Platform
 *
 * @param mutex The mutex
 * @return 0 on success
 */
int lf_mutex_lock(lf_mutex_t* mutex);

/**
 * @brief Unlock the specified mutex.
 * @ingroup Platform
 *
 * @param mutex The mutex
 * @return 0 on success
 */
int lf_mutex_unlock(lf_mutex_t* mutex);

/**
 * @brief Initialize a conditional variable.
 * @ingroup Platform
 * @param cond The condition variable.
 * @param mutex The associated mutex.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex);

/**
 * @brief Wake up all threads waiting for condition variable cond.
 * @ingroup Platform
 * @param cond The condition variable.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_broadcast(lf_cond_t* cond);

/**
 * @brief Wake up one thread waiting for condition variable cond.
 * @ingroup Platform
 * @param cond The condition variable.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_signal(lf_cond_t* cond);

/**
 * @brief Wait for condition variable "cond" to be signaled or broadcast.
 * @ingroup Platform
 *
 * The cond->mutex is assumed to be locked when this is called.
 * @param cond The condition variable.
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_cond_wait(lf_cond_t* cond);

/**
 * @brief Block the current thread on the condition variable with a timeout.
 * @ingroup Platform
 *
 * This will block until the condition variable
 * pointed by `cond` is signaled or the time given by `wakeup_time` is reached. This should
 * not be used directly as it does not account for clock synchronization offsets.
 * Use @ref lf_clock_cond_timedwait from @ref clock.h instead.
 * @param cond The condition variable.
 * @param wakeup_time The timeout time.
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise.
 */
int _lf_cond_timedwait(lf_cond_t* cond, instant_t wakeup_time);

/*
 * Cross-platform version of the C11 thread_local keyword.
 */
#ifndef thread_local
#if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#define thread_local _Thread_local
#elif defined _WIN32 && (defined _MSC_VER || defined __ICL || defined __DMC__ || defined __BORLANDC__)
#define thread_local __declspec(thread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
#elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
#define thread_local __thread
#else
#error "Cannot define thread_local"
#endif
#endif // thread_local

/**
 * @brief Return the ID of the current thread.
 * @ingroup Platform
 *
 * The only guarantee is that these IDs will be a contiguous range of numbers
 * starting at 0.
 */
int lf_thread_id(void);

/**
 * @brief Initialize the thread ID for the current thread.
 * @ingroup Platform
 */
void initialize_lf_thread_id(void);
#endif // !defined(LF_SINGLE_THREADED)

/**
 * @brief Initialize the LF clock.
 * @ingroup Platform
 *
 * Must be called before using other clock-related APIs.
 */
void _lf_initialize_clock(void);

/**
 * @brief Get the value of an internal (and platform-specific) physical clock.
 * @ingroup Platform
 *
 * Ideally, the underlying platform clock should be monotonic. However, the core
 * lib enforces monotonicity at higher level APIs (see @ref clock.h).
 *
 * This should not be used directly as it does not apply clock synchronization
 * offsets.
 * @param t A pointer to the place to store the result.
 * @return 0 for success, or -1 for failure
 */
int _lf_clock_gettime(instant_t* t);

/**
 * @brief Pause execution for a given duration.
 * @ingroup Platform
 * @param sleep_duration The duration.
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration);

/**
 * @brief Sleep until the given wakeup time.
 * @ingroup Platform
 *
 * This should not be used directly as it
 * does not account for clock synchronization offsets. See clock.h.
 *
 * This assumes the lock for the given environment is held.
 *
 * @param env The environment within which to sleep.
 * @param wakeup_time The time instant at which to wake up.
 * @return int 0 if sleep completed, or -1 if it was interrupted.
 */
int _lf_interruptable_sleep_until_locked(environment_t* env, instant_t wakeup_time);

/*
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
