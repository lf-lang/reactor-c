/**
 * @author Erling Rennemo Jellum
 * @copyright (c) 2023
 * License: <a href="https://github.com/lf-lang/reactor-c/blob/main/LICENSE.md">BSD 2-clause</a>
 * @brief This file defines the LF atomic API. These functions will have platform-dependent implementations.
 */
#ifndef LF_ATOMICS_H
#define LF_ATOMICS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Atomically fetch a 32bit integer from memory and add a value to it.
 * Return the value that was previously in memory.
 *
 * @param ptr A pointer to the memory location.
 * @param val The value to be added.
 * @return The value previously in memory.
 */
int32_t lf_atomic_fetch_add32(int32_t * ptr, int32_t val);

/**
 * @brief Atomically fetch 64-bit integer from memory and add a value to it.
 * Return the value that was previously in memory.
 *
 * @param ptr A pointer to the memory location.
 * @param val The value to be added.
 * @return The value previously in memory.
 */
int64_t lf_atomic_fetch_add64(int64_t * ptr, int64_t val);

/**
 * @brief Atomically fetch a 32-bit integer from memory and add a value to it.
 * Return the new value of the memory.
 *
 * @param ptr A pointer to the memory location.
 * @param val The value to be added.
 * @return The new value in memory.
 */
int32_t lf_atomic_add_fetch32(int32_t * ptr, int32_t val);

/**
 * @brief Atomically fetch a 64-bit integer from memory and add a value to it.
 * Return the new value of the memory.
 *
 * @param ptr A pointer to the memory location.
 * @param val The value to be added.
 * @return The new value in memory.
 */
int64_t lf_atomic_add_fetch64(int64_t * ptr, int64_t val);

/**
 * @brief Atomically perform a compare-and-swap operation on a 32 bit integer in
 * memory. If the value in memory is equal to `oldval` replace it with `newval`
 * and return true. If not return false.
 *
 * @param ptr A pointer to the memory location.
 * @param oldval The value to compare with.
 * @param newval The value to swap in.
 * @return Whether a swap was performed or not.
 */
bool lf_atomic_bool_compare_and_swap32(int32_t* ptr, int32_t oldval, int32_t newval);

/**
 * @brief Atomically perform a compare-and-swap operation on a 64 bit integer in
 * memory. If the value in memory is equal to `oldval` replace it with `newval`
 * and return true. If not return false.
 *
 * @param ptr A pointer to the memory location.
 * @param oldval The value to compare with.
 * @param newval The value to swap in.
 * @return Whether a swap was performed or not.
 */
bool lf_atomic_bool_compare_and_swap64(int64_t* ptr, int64_t oldval, int64_t newval);

/**
 * @brief Atomically perform a compare-and-swap operation on a 32 bit integer in
 * memory. If the value in memory is equal to `oldval` replace it with `newval`.
 * Return the content of the memory before the potential swap operation is
 * performed.
 *
 * @param ptr A pointer to the memory location.
 * @param oldval The value to compare with.
 * @param newval The value to swap in.
 * @return The value in memory prior to the swap.
 */
int32_t lf_atomic_val_compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval);

/**
 * @brief Atomically perform a compare-and-swap operation on a 64 bit integer in
 * memory. If the value in memory is equal to `oldval` replace it with `newval`.
 * Return the content of the memory before the potential swap operation is
 * performed.
 *
 * @param ptr A pointer to the memory location.
 * @param oldval The value to compare with.
 * @param newval The value to swap in.
 * @return The value in memory prior to the swap.
 */
int64_t lf_atomic_val_compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval);

#endif
