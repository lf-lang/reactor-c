#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
/**
 * @author Soroush Bateni
 * @author Erling Rennemo Jellum
 * @copyright (c) 2023
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implements the atomic API for Windows machines.
 */

#include "lf_atomic.h"
#include <windows.h>

int32_t lf_atomic_fetch_add32(int32_t *ptr, int32_t value) { 
    return InterlockedExchangeAdd(ptr, value);
}
int64_t lf_atomic_fetch_add64(int64_t *ptr, int64_t value) {
    return InterlockedExchangeAdd64(ptr, value);
}
int32_t lf_atomic_add_fetch32(int32_t *ptr, int32_t value) {
    return InterlockedAdd(ptr, value);
}
int64_t lf_atomic_add_fetch64(int64_t *ptr, int64_t value) {
    return InterlockedAdd64(ptr, value);
}
bool lf_atomic_bool_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return (InterlockedCompareExchange(ptr, newval, oldval) == oldval);
}
bool lf_atomic_bool_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return (InterlockedCompareExchange64(ptr, newval, oldval) == oldval);
}
int32_t  lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return InterlockedCompareExchange(ptr, newval, oldval);
}
int64_t  lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return InterlockedCompareExchange64(ptr, newval, oldval);
}
#endif
