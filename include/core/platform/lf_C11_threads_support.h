/* C11 threads support for the C target of Lingua Franca. */

/*************
Copyright (c) 2019, The University of California at Berkeley.

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

/** \file if_c11_threads_support.c
 * C11 threads support for the C target of Lingua Franca.
 *
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 */
#ifndef LF_C11_THREADS_SUPPORT_H
#define LF_C11_THREADS_SUPPORT_H

#include <threads.h>
#include <stdlib.h>
#include <stdint.h> // For fixed-width integral types

typedef mtx_t lf_mutex_t;
typedef struct {
    lf_mutex_t* mutex;
    cnd_t condition;
} lf_cond_t;
typedef thrd_t lf_thread_t;

#define _LF_TIMEOUT thrd_timedout

/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is tored in thread.
 *
 * @return 0 on success, error number otherwise (see thrd_create()).
 */
static int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    return thrd_create((thrd_t*)thread, (thrd_start_t)lf_thread, arguments);
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return, if thread_return
 * is not NULL.
 *
 * @return 0 on success, error number otherwise (see thrd_join()).
 */
static int lf_thread_join(lf_thread_t thread, void** thread_return) {
    // thrd_join wants the second argument to be an int* rather than a void**
    return thrd_join((thrd_t)thread, (int*)thread_return);
}

/**
 * Initialize a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_init()).
 */
static int lf_mutex_init(lf_mutex_t* mutex) {
    // Set up a timed and recursive mutex (default behavior)
    return mtx_init((mtx_t*)mutex, mtx_timed | mtx_recursive);
}

/**
 * Lock a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_lock()).
 */
static int lf_mutex_lock(lf_mutex_t* mutex) {
    return mtx_lock((mtx_t*) mutex);
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_unlock()).
 */
static int lf_mutex_unlock(lf_mutex_t* mutex) {
    return mtx_unlock((mtx_t*) mutex);
}

/**
 * Initialize a conditional variable.
 *
 * @return 0 on success, error number otherwise (see cnd_init()).
 */
static int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    return cnd_init((cnd_t*)&cond->condition);
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see cnd_broadcast()).
 */
static int lf_cond_broadcast(lf_cond_t* cond) {
    return cnd_broadcast((cnd_t*)&cond->condition);
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see cnd_signal()).
 */
static int lf_cond_signal(lf_cond_t* cond) {
    return cnd_signal((cnd_t*)&cond->condition);
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, error number otherwise (see cnd_wait()).
 */
static int lf_cond_wait(lf_cond_t* cond) {
    return cnd_wait((cnd_t*)&cond->condition, (mtx_t*)cond->mutex);
}

/**
 * Block current thread on the condition variable until condition variable
 * pointed by "cond" is signaled or time pointed by "absolute_time_ns" in
 * nanoseconds is reached.
 *
 * @return 0 on success, LF_TIMEOUT on timeout, and platform-specific error
 *  number otherwise (see pthread_cond_timedwait).
 */
static int lf_cond_timedwait(lf_cond_t* cond, int64_t absolute_time_ns) {
    // Convert the absolute time to a timespec.
    // timespec is seconds and nanoseconds.
    struct timespec timespec_absolute_time
            = {(time_t)absolute_time_ns / 1000000000LL, (long)absolute_time_ns % 1000000000LL};
    int return_value = 0;
    return_value = cnd_timedwait(
        (cnd_t*)&cond->condition,
        (mtx_t*)cond->mutex,
        &timespec_absolute_time
    );
    switch (return_value) {
        case thrd_timedout:
            return_value = _LF_TIMEOUT;
            break;

        default:
            break;
    }
    return return_value;
}

#endif
