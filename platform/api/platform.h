/**
 * @file platform.h
 * @author Peter Donovan
 *
 * @brief Platform API for runtime plugins to use while sharing implementation
 * source code and binaries with the core and with each other.
 * @ingroup Platform
 */

/**
 * @brief Pointer to the platform-specific implementation of a mutex.
 * @ingroup Platform
 */
typedef void* lf_platform_mutex_ptr_t;

/**
 * @brief Create a new mutex and return (a pointer to) it.
 * @ingroup Platform
 */
lf_platform_mutex_ptr_t lf_platform_mutex_new();

/**
 * @brief Free all resources associated with the provided mutex.
 * @ingroup Platform
 */
void lf_platform_mutex_free(lf_platform_mutex_ptr_t mutex);

/**
 * @brief Acquire the given mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * @ingroup Platform
 */
int lf_platform_mutex_lock(lf_platform_mutex_ptr_t mutex);

/**
 * @brief Release the given mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 * @ingroup Platform
 */
int lf_platform_mutex_unlock(lf_platform_mutex_ptr_t mutex);

/// \cond INTERNAL  // Doxygen conditional.
// The following is defined in low_level_platform.h, so ask Doxygen to ignore this.

/**
 * @brief The ID of the current thread.
 *
 * The only guarantee is that these IDs will be a contiguous range of numbers
 * starting at 0.
 */
int lf_thread_id();

/// \endcond // INTERNAL
