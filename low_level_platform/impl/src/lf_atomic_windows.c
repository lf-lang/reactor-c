#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
/**
 * @author Soroush Bateni
 * @author Erling Rennemo Jellum
 * @copyright (c) 2023
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implements the atomic API for Windows machines.
 */

#include "platform/lf_atomic.h"
#include <windows.h>

int lf_atomic_fetch_add(int* ptr, int value) { return InterlockedExchangeAdd((LONG*)ptr, (LONG)value); }
int64_t lf_atomic_fetch_add64(int64_t* ptr, int64_t value) { return InterlockedExchangeAdd64(ptr, value); }
int lf_atomic_add_fetch(int* ptr, int value) { return InterlockedAdd((LONG*)ptr, (LONG)value); }
int64_t lf_atomic_add_fetch64(int64_t* ptr, int64_t value) { return InterlockedAdd64(ptr, value); }
bool lf_atomic_bool_compare_and_swap(int* ptr, int oldval, int newval) {
  return (InterlockedCompareExchange((LONG*)ptr, (LONG)newval, (LONG)oldval) == oldval);
}
bool lf_atomic_bool_compare_and_swap64(int64_t* ptr, int64_t oldval, int64_t newval) {
  return (InterlockedCompareExchange64(ptr, newval, oldval) == oldval);
}
int lf_atomic_val_compare_and_swap(int* ptr, int oldval, int newval) {
  return InterlockedCompareExchange((LONG*)ptr, (LONG)newval, (LONG)oldval);
}
int64_t lf_atomic_val_compare_and_swap64(int64_t* ptr, int64_t oldval, int64_t newval) {
  return InterlockedCompareExchange64(ptr, newval, oldval);
}
#endif
