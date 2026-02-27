#if !defined(LF_SINGLE_THREADED)
/**
 * @brief Semaphore utility for reactor C.
 *
 * @author Soroush Bateni
 */

#include "lf_semaphore.h"
#include <assert.h>
#include "util.h" // Defines macros LF_MUTEX_LOCK, etc.

/**
 * @brief Create a new semaphore.
 *
 * @param count The count to start with.
 * @return lf_semaphore_t* Can be NULL on error.
 */
lf_semaphore_t* lf_semaphore_new(size_t count) {
  lf_semaphore_t* semaphore = (lf_semaphore_t*)malloc(sizeof(lf_semaphore_t));
  LF_MUTEX_INIT(&semaphore->mutex);
  LF_COND_INIT(&semaphore->cond, &semaphore->mutex);
  semaphore->count = count;
  return semaphore;
}

/**
 * @brief Release the 'semaphore' and add 'i' to its count.
 *
 * @param semaphore Instance of a semaphore
 * @param i The count to add.
 */
void lf_semaphore_release(lf_semaphore_t* semaphore, size_t i) {
  assert(semaphore != NULL);
  LF_MUTEX_LOCK(&semaphore->mutex);
  semaphore->count += i;
  lf_cond_broadcast(&semaphore->cond);
  LF_MUTEX_UNLOCK(&semaphore->mutex);
}

/**
 * @brief Acquire the 'semaphore'. Will block if count is 0.
 *
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_acquire(lf_semaphore_t* semaphore) {
  assert(semaphore != NULL);
  LF_MUTEX_LOCK(&semaphore->mutex);
  while (semaphore->count == 0) {
    lf_cond_wait(&semaphore->cond);
  }
  semaphore->count--;
  LF_MUTEX_UNLOCK(&semaphore->mutex);
}

/**
 * @brief Wait on the 'semaphore' if count is 0.
 *
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_wait(lf_semaphore_t* semaphore) {
  assert(semaphore != NULL);
  LF_MUTEX_LOCK(&semaphore->mutex);
  while (semaphore->count == 0) {
    lf_cond_wait(&semaphore->cond);
  }
  LF_MUTEX_UNLOCK(&semaphore->mutex);
}

/**
 * @brief Destroy the 'semaphore'.
 *
 * @param semaphore Instance of a semaphore.
 */
void lf_semaphore_destroy(lf_semaphore_t* semaphore) {
  assert(semaphore != NULL);
  free(semaphore);
}
#endif
