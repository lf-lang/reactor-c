/**
 * @file lf_semaphore.h
 * @author Soroush Bateni
 *
 * @brief Semaphore utility for reactor C.
 * @ingroup Internal
 */

#ifndef LF_SEMAPHORE_H
#define LF_SEMAPHORE_H

#ifndef NUMBER_OF_WORKERS
#define NUMBER_OF_WORKERS 1
#endif // NUMBER_OF_WORKERS

#include "low_level_platform.h"
#include <stdlib.h>

/**
 * @brief A semaphore.
 * @ingroup Internal
 */
typedef struct {
  size_t count;
  lf_mutex_t mutex;
  lf_cond_t cond;
} lf_semaphore_t;

/**
 * @brief Create a new semaphore.
 * @ingroup Internal
 * 
 * @param count The count to start with.
 * @return lf_semaphore_t* Can be NULL on error.
 */
lf_semaphore_t* lf_semaphore_new(size_t count);

/**
 * @brief Release the 'semaphore' and add 'i' to its count.
 * @ingroup Internal
 * 
 * @param semaphore Instance of a semaphore
 * @param i The count to add.
 */
void lf_semaphore_release(lf_semaphore_t* semaphore, size_t i);

/**
 * @brief Acquire the 'semaphore'. Will block if count is 0.
 * @ingroup Internal
 * 
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_acquire(lf_semaphore_t* semaphore);

/**
 * @brief Wait on the 'semaphore' if count is 0.
 * @ingroup Internal
 * 
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_wait(lf_semaphore_t* semaphore);

/**
 * @brief Destroy the 'semaphore'.
 * @ingroup Internal
 * 
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_destroy(lf_semaphore_t* semaphore);

#endif // LF_SEMAPHORE_H
