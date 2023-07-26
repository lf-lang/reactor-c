#if defined(LF_THREADED) && !defined(PLATFORM_ARDUINO)
#include "platform.h"
#include "lf_C11_threads_support.h"

#include <threads.h>
#include <stdlib.h>
#include <stdint.h> // For fixed-width integral types

int lf_thread_create(lf_thread_t* thread, void *(*lf_thread) (void *), void* arguments) {
    return thrd_create((thrd_t*)thread, (thrd_start_t)lf_thread, arguments);
}

int lf_thread_join(lf_thread_t thread, void** thread_return) {
    // thrd_join wants the second argument to be an int* rather than a void**
    return thrd_join((thrd_t)thread, (int*)thread_return);
}

int lf_mutex_init(lf_mutex_t* mutex) {
    // Set up a timed and recursive mutex (default behavior)
    return mtx_init((mtx_t*)mutex, mtx_timed | mtx_recursive);
}

int lf_mutex_lock(lf_mutex_t* mutex) {
    return mtx_lock((mtx_t*) mutex);
}

int lf_mutex_unlock(lf_mutex_t* mutex) {
    return mtx_unlock((mtx_t*) mutex);
}

int lf_cond_init(lf_cond_t* cond, lf_mutex_t* mutex) {
    cond->mutex = mutex;
    return cnd_init((cnd_t*)&cond->condition);
}

int lf_cond_broadcast(lf_cond_t* cond) {
    return cnd_broadcast((cnd_t*)&cond->condition);
}

int lf_cond_signal(lf_cond_t* cond) {
    return cnd_signal((cnd_t*)&cond->condition);
}

int lf_cond_wait(lf_cond_t* cond) {
    return cnd_wait((cnd_t*)&cond->condition, (mtx_t*)cond->mutex);
}

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
#endif
