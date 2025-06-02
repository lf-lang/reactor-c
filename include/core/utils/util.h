/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @brief Core utility functions for Lingua Franca.
 * @ingroup Internal
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h> // Defines va_list
#include <stdbool.h>
#include <stdint.h> // Defines int64_t

#include "logging_macros.h"

/**
 * @brief A handy macro that can concatenate three strings.
 * @ingroup Internal
 * Useful in the LF_PRINT_DEBUG macro and lf_print_error
 * functions that want to concatenate a "DEBUG: " or
 * "ERROR: " to the beginning of the message and a
 * new line format \n at the end.
 */
#define CONCATENATE_THREE_STRINGS(__string1, __string2, __string3) __string1 __string2 __string3

/**
 * @brief Macro for extracting the level from the index of a reaction.
 * @ingroup Internal
 * A reaction that has no upstream reactions has level 0.
 * Other reactions have a level that is the length of the longest
 * upstream chain to a reaction with level 0 (inclusive).
 * This is used, along with the deadline, to sort reactions
 * in the reaction queue. It ensures that reactions that are
 * upstream in the dependence graph execute before reactions
 * that are downstream.
 */
#define LF_LEVEL(index) (index & 0xffffLL)

/**
 * @brief Utility for finding the maximum of two values.
 * @ingroup Internal
 */
#ifndef LF_MAX
#define LF_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

/**
 * @brief Utility for finding the minimum of two values.
 * @ingroup Internal
 */
#ifndef LF_MIN
#define LF_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

/**
 * @brief The ID of this federate.
 * @ingroup Internal
 * For a non-federated execution, this will be -1.
 * For a federated execution, it will be assigned when the generated function
 * _lf_initialize_trigger_objects() is called.
 * @see xtext/org.icyphy.linguafranca/src/org/icyphy/generator/CGenerator.xtend.
 */
extern uint16_t _lf_my_fed_id;

/**
 * @brief Return the federate ID or -1 if this program is not part of a federation.
 * @ingroup Internal
 */
uint16_t lf_fed_id(void);

/**
 * @brief varargs alternative of "lf_print"
 * @ingroup Internal
 */
void lf_vprint(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief varargs alternative of "lf_print_log"
 * @ingroup Internal
 */
void lf_vprint_log(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief varargs alternative of "lf_print_debug"
 * @ingroup Internal
 */
void lf_vprint_debug(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief Print the error defined by the errno variable with the
 * specified message as a prefix, then exit with error code 1.
 * @ingroup Internal
 * @param msg The prefix to the message.
 */
void error(const char* msg);

/**
 * @brief varargs alternative of "lf_print_error"
 * @ingroup Internal
 */
void lf_vprint_error(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief varargs alternative of "lf_print_warning"
 * @ingroup Internal
 */
void lf_vprint_warning(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief varargs alternative of "lf_print_error_and_exit"
 * @ingroup Internal
 */
void lf_vprint_error_and_exit(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * @brief Initialize mutex with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param mutex Pointer to the mutex to initialize.
 */
#define LF_MUTEX_INIT(mutex) LF_ASSERTN(lf_mutex_init(mutex), "Mutex init failed.")

/**
 * @brief Lock mutex with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param mutex Pointer to the mutex to lock.
 */
#define LF_MUTEX_LOCK(mutex) LF_ASSERTN(lf_mutex_lock(mutex), "Mutex lock failed.")

/**
 * @brief Unlock mutex with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param mutex Pointer to the mutex to unlock.
 */
#define LF_MUTEX_UNLOCK(mutex) LF_ASSERTN(lf_mutex_unlock(mutex), "Mutex unlock failed.")

/**
 * @brief Initialize condition variable with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param cond Pointer to the condition variable to initialize.
 * @param mutex Pointer to the mutex to associate with the condition variable.
 */
#define LF_COND_INIT(cond, mutex) LF_ASSERTN(lf_cond_init(cond, mutex), "Condition variable init failed.")

/**
 * @brief Signal a condition variable with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param cond Pointer to the condition variable.
 */
#define LF_COND_SIGNAL(cond) LF_ASSERTN(lf_cond_signal(cond), "Condition variable signal failed.")

/**
 * @brief Broadcast a condition variable with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param cond Pointer to the condition variable.
 */
#define LF_COND_BROADCAST(cond) LF_ASSERTN(lf_cond_broadcast(cond), "Condition variable broadcast failed.")

/**
 * @brief Wait on a condition variable with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param cond Pointer to the condition variable.
 */
#define LF_COND_WAIT(cond) LF_ASSERTN(lf_cond_wait(cond), "Condition variable wait failed.")

/**
 * @brief Enter critical section with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param env Pointer to the environment.
 */
#define LF_CRITICAL_SECTION_ENTER(env) LF_ASSERT(!lf_critical_section_enter(env), "Could not enter critical section")

/**
 * @brief Exit critical section with error checking.
 * @ingroup Internal
 * This is optimized away if the NDEBUG flag is defined.
 * @param env Pointer to the environment.
 */
#define LF_CRITICAL_SECTION_EXIT(env) LF_ASSERT(!lf_critical_section_exit(env), "Could not exit critical section")

#endif /* UTIL_H */
