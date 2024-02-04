#ifndef LF_ATOMICS_H
#define LF_ATOMICS_H

#include <stdint.h>
#include <stdbool.h>

int32_t lf_atomic_fetch_add32(int32_t * ptr, int32_t val);
int64_t lf_atomic_fetch_add64(int64_t * ptr, int64_t val);
int32_t lf_atomic_add_fetch32(int32_t * ptr, int32_t val);
int64_t lf_atomic_add_fetch64(int64_t * ptr, int64_t val);
bool lf_atomic_bool_compare_and_swap32(int32_t* ptr, int32_t value, int32_t newval);
bool lf_atomic_bool_compare_and_swap64(int64_t* ptr, int64_t value, int64_t newval);
int32_t lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t val, int32_t newval);
int64_t lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t val, int64_t newval);

#endif