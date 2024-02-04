#ifndef LF_ATOMICS_H
#define LF_ATOMICS_H

#include <stdint.h>
#include <stdbool.h>

int32_t lf_atomic_fetch_add32(int32_t * ptr, int32_t val);
int64_t lf_atomic_fetch_add64(int64_t * ptr, int64_t val);
int32_t lf_atomic_add_fetch32(int32_t * ptr, int32_t val);
int64_t lf_atomic_add_fetch64(int64_t * ptr, int64_t val);


bool lf_bool_compare_and_swap(bool ptr, bool value, bool newval);

int32_t lf_val32_compare_and_swap(int32_t *ptr, int32_t val, int32_t newval);
int64_t lf_val64_compare_and_swap(int64_t *ptr, int64_t val, int64_t newval);

#endif