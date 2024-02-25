/**
 * @file platform.h
 * @author Peter Donovan (peter@xronos.com)
 * @brief Platform API for runtime plugins to use while sharing implementation
 * source code and binaries with the core and with each other.
 * @version 0.1
 * @date 2024-01-29
 *
 * @copyright Copyright (c) 2024
 */

/**
 * @brief Pointer to the platform-specific implementation of a mutex.
 */
typedef void* lf_platform_mutex_ptr_t;
/**
 * @brief Create a new mutex and return (a pointer to) it.
 */
lf_platform_mutex_ptr_t lf_platform_mutex_new();
/**
 * @brief Free all resources associated with the provided mutex.
 */
void lf_platform_mutex_free(lf_platform_mutex_ptr_t mutex);
/**
 * @brief Acquire the given mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_platform_mutex_lock(lf_platform_mutex_ptr_t mutex);
/**
 * @brief Release the given mutex.
 *
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_platform_mutex_unlock(lf_platform_mutex_ptr_t mutex);

/**
 * @brief The ID of the current thread. The only guarantee is that these IDs will be a contiguous range of numbers starting at 0.
 */
int lf_thread_id();
