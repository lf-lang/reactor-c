#if defined LF_SINGLE_THREADED && !defined(PLATFORM_ARDUINO)
/**
 * @file lf_os_single_threaded_support.c
 * @author Marten Lohstroh (marten@berkeley.edu)
 * @brief Implementation of platform functions to ensure safe concurrent
 * access to a critical section, which are unnecessary in an OS-supported
 * single-threaded runtime and therefore are left blank.
 * @note This file is only to be used in conjuction with an OS-supported
 * single-threaded runtime. If threads are enabled, this file will fail
 * to compile. If threads are needed, use a multi-threaded runtime instead.
 * @copyright BSD 2-Clause License (see LICENSE.md)
 */

#if defined(_THREADS_H) || defined(_PTHREAD_H)
    #error Usage of threads in the single-threaded runtime is not safe.
#endif

/**
 * @brief Single-threaded support under a OS is a special case in which we assume
 * only a single execution context. Other threads scheduling physical actions
 * are not a use-case. ISRs scheduling physical actions are also not a use-case.
 * 
 * @return int 
 */
int lf_disable_interrupts_nested() {
    return 0;
}

int lf_enable_interrupts_nested() {
    return 0;
}

int _lf_single_threaded_notify_of_event() {
    return 0;
}

#endif
