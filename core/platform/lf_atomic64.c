#include "lf_atomic.h"
#include "platform.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
int32_t lf_atomic_fetch_add32(int32_t *ptr, int32_t value) { 
    return InterlockedExchangeAdd(ptr, value);
}
int64_t lf_atomic_fetch_add64(int64_t *ptr, int64_t value) {
    return InterlockedExchangeAdd64(ptr, value);
}
int lf_atomic_add_fetch32(int32_t *ptr, int32_t value) {
    return InterlockedAdd(ptr, value)
}
int64_t lf_atomic_add_fetch64(int64_t *ptr, int64_t value) {
    return InterlockedAdd64(ptr, value)
}
bool lf_atomic_bool_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return  (InterlockedCompareExchange(ptr, newval, oldval) == oldval)
}
bool lf_atomic_bool_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return  (InterlockedCompareExchange64(ptr, newval, oldval) == oldval)
}
int  lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return InterlockedCompareExchange(ptr, newval, oldval)
}
int64_t  lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return InterlockedCompareExchange64(ptr, newval, oldval)
}
#elif defined(__GNUC__) || defined(__clang__)
int32_t lf_atomic_fetch_add32(int32_t *ptr, int32_t value) { 
    return __sync_fetch_and_add(ptr, value)
}
int64_t lf_atomic_fetch_add64(int64_t *ptr, int64_t value) {
    return __sync_fetch_and_add(ptr, value)
}
int lf_atomic_add_fetch32(int32_t *ptr, int32_t value) {
    return __sync_add_and_fetch(ptr, value)
}
int64_t lf_atomic_add_fetch64(int64_t *ptr, int64_t value) {
    return __sync_add_and_fetch(ptr, value)
}
bool lf_atomic_bool_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return __sync_bool_compare_and_swap(ptr, oldval, newval);
}
bool lf_atomic_bool_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return __sync_bool_compare_and_swap(ptr, oldval, newval);
}
int  lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
    return __sync_val_compare_and_swap(ptr, oldval, newval)
}
int64_t  lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
    return __sync_val_compare_and_swap(ptr, oldval, newval)
}

#else
#error "Compiler not supported"
#endif