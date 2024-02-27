/**
 * @file platform.c
 * @author Peter Donovan (peter@xronos.com)
 * @brief A variant of the platform abstraction whose ABI is
 * platform-independent.
 * @version 0.1
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2024
 */
#include <stdlib.h>

#include "low_level_platform.h"
#include "platform.h"

// MUTEXES *********************************************************************

lf_platform_mutex_ptr_t lf_platform_mutex_new() {
    lf_platform_mutex_ptr_t mutex = (lf_platform_mutex_ptr_t) malloc(sizeof(lf_mutex_t));
    if (mutex) lf_mutex_init(mutex);
    return mutex;
};
void lf_platform_mutex_free(lf_platform_mutex_ptr_t mutex) {
    free((void*) mutex);
}
int lf_platform_mutex_lock(lf_platform_mutex_ptr_t mutex) {
    return lf_mutex_lock((lf_mutex_t*) mutex);
}
int lf_platform_mutex_unlock(lf_platform_mutex_ptr_t mutex) {
    return lf_mutex_unlock((lf_mutex_t*) mutex);
}
