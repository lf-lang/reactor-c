#include "lf_atomic.h"
#include "platform.h"
/**
 * Implements atomics for 32 bit platforms by disabling interrupts.
 */

int32_t lf_atomic_fetch_add32(int32_t *ptr, int32_t value) {
    lf_disable_interrupts_nested(); 
    int32_t res = *ptr; 
    *ptr += value; 
    lf_enable_interrupts_nested(); 
    return res;
}

int64_t lf_atomic_fetch_add64(int64_t *ptr, int64_t value) {
    lf_disable_interrupts_nested(); 
    int64_t res = *ptr; 
    *ptr += value; 
    lf_enable_interrupts_nested(); 
    return res;
}

int32_t lf_atomic_add_fetch32(int32_t *ptr, int32_t value) {
    lf_disable_interrupts_nested();
    int res = *ptr + value;
    *ptr = res;
    lf_enable_interrupts_nested();
    return res;
}

int64_t lf_atomic_add_fetch64(int64_t *ptr, int64_t value) {
    lf_disable_interrupts_nested();
    int64_t res = *ptr + value;
    *ptr = res;
    lf_enable_interrupts_nested();
    return res;
}

bool lf_atomic_bool_compare_and_swap32(int32_t *ptr, int32_t value, int32_t newval) {
    lf_disable_interrupts_nested();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_enable_interrupts_nested();
    return res;
}

bool lf_atomic_bool_compare_and_swap64(int64_t *ptr, int64_t value, int64_t newval) {
    lf_disable_interrupts_nested();
    bool res = false;
    if (*ptr == value) {
        *ptr = newval;
        res = true;
    }
    lf_enable_interrupts_nested();
    return res;
}

int32_t  lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t value, int32_t newval) {
    lf_disable_interrupts_nested();
    int res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    lf_enable_interrupts_nested();
    return res;
}

int64_t  lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t value, int64_t newval) {
    lf_disable_interrupts_nested();
    int64_t res = *ptr;
    if (*ptr == value) {
        *ptr = newval;
    }
    lf_enable_interrupts_nested();
    return res;
}