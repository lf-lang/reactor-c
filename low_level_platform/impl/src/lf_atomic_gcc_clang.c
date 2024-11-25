#if defined(PLATFORM_Linux) || defined(PLATFORM_Darwin)
#if defined(__GNUC__) || defined(__clang__)
/**
 * @author Soroush Bateni
 * @author Erling Rennemo Jellum
 * @copyright (c) 2023
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief Implements the atomics API using GCC/Clang APIs.
 */

#include "platform/lf_atomic.h"
#include "low_level_platform.h"

int lf_atomic_fetch_add(int* ptr, int value) { return __sync_fetch_and_add(ptr, value); }
int64_t lf_atomic_fetch_add64(int64_t* ptr, int64_t value) { return __sync_fetch_and_add(ptr, value); }
int lf_atomic_add_fetch(int* ptr, int value) { return __sync_add_and_fetch(ptr, value); }
int64_t lf_atomic_add_fetch64(int64_t* ptr, int64_t value) { return __sync_add_and_fetch(ptr, value); }
bool lf_atomic_bool_compare_and_swap(int* ptr, int oldval, int newval) {
  return __sync_bool_compare_and_swap(ptr, oldval, newval);
}
bool lf_atomic_bool_compare_and_swap64(int64_t* ptr, int64_t oldval, int64_t newval) {
  return __sync_bool_compare_and_swap(ptr, oldval, newval);
}
int lf_atomic_val_compare_and_swap(int* ptr, int oldval, int newval) {
  return __sync_val_compare_and_swap(ptr, oldval, newval);
}
int64_t lf_atomic_val_compare_and_swap64(int64_t* ptr, int64_t oldval, int64_t newval) {
  return __sync_val_compare_and_swap(ptr, oldval, newval);
}

#endif
#endif
