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
 *
 * A semaphore is a synchronization primitive that maintains a count.
 * The count is decremented by acquire operations and incremented by release operations.
 * If the count would become negative, the acquire operation blocks until the count
 * becomes positive again.
 */
typedef struct {
  /**
   * @brief The current count of the semaphore.
   * This value is protected by the mutex and can be modified
   * only while holding the mutex lock.
   */
  size_t count;

  /**
   * @brief Mutex used to protect access to the count.
   * Ensures that count modifications are atomic and
   * coordinates access between multiple threads.
   */
  lf_mutex_t mutex;

  /**
   * @brief Condition variable used for blocking operations.
   * Threads waiting for the semaphore to become available
   * block on this condition variable.
   */
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
