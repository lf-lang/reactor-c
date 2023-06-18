#include "platform.h"
#include <threads.h>
#include <stdlib.h>
#include <stdint.h> // For fixed-width integral types


/**
 * Create a new thread, starting with execution of lf_thread
 * getting passed arguments. The new handle is stored in the thread argument.
 *
 * @return 0 on success, error number otherwise (see thrd_create()).
 */
int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    return thrd_create((thrd_t*)thread, (thrd_start_t)lf_thread, arguments);
}

/**
 * Make calling thread wait for termination of the thread.  The
 * exit status of the thread is stored in thread_return if thread_return
 * is not NULL.
 *
 * @return 0 on success, error number otherwise (see thrd_join()).
 */
int lf_thread_join(lf_thread_t thread, void** thread_return) {
    // thrd_join wants the second argument to be an int* rather than a void**
    return thrd_join((thrd_t)thread, (int*)thread_return);
}

/**
 * Initialize a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_init()).
 */
int lf_mutex_init(lf_mutex_t* mutex) {
    // Set up a timed and recursive mutex (default behavior)
    return mtx_init((mtx_t*)mutex, mtx_timed | mtx_recursive);
}

/**
 * Lock a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_lock()).
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    return mtx_lock((mtx_t*) mutex);
}

/**
 * Unlock a mutex.
 *
 * @return 0 on success, error number otherwise (see mtx_unlock()).
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    return mtx_unlock((mtx_t*) mutex);
}

/**
 * Initialize a conditional variable.
 *
 * @return 0 on success, error number otherwise (see cnd_init()).
 */
int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    return cnd_init((cnd_t*)&cond->condition);
}

/**
 * Wake up all threads waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see cnd_broadcast()).
 */
int lf_cond_broadcast(lf_cond_t* cond) {
    return cnd_broadcast((cnd_t*)&cond->condition);
}

/**
 * Wake up one thread waiting for condition variable cond.
 *
 * @return 0 on success, error number otherwise (see cnd_signal()).
 */
int lf_cond_signal(lf_cond_t* cond) {
    return cnd_signal((cnd_t*)&cond->condition);
}

/**
 * Wait for condition variable "cond" to be signaled or broadcast.
 * "mutex" is assumed to be locked before.
 *
 * @return 0 on success, error number otherwise (see cnd_wait()).
 */
int lf_cond_wait(lf_cond_t* cond) {
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
int lf_cond_timedwait(lf_cond_t* cond, int64_t absolute_time_ns) {
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
            return_value = LF_TIMEOUT;
            break;

        default:
            break;
    }
    return return_value;
}
