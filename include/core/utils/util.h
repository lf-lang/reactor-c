/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 *
 * @section LICENSE
Copyright (c) 2020, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * @section DESCRIPTION
 * Header file for utility types and functions for Lingua Franca programs.
 */

#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>   // Defines va_list
#include <stdbool.h>
#include <stdint.h>   // Defines int64_t

// To silence warnings about a function being a candidate for format checking
// with gcc, add an attribute.
// The arguments are the position of the format string (starting with 1)
// and the start of the remaining arguments, or 0 for vprintf style functions.
#if defined(__GNUC__)
#define ATTRIBUTE_FORMAT_PRINTF(f, s) __attribute__((format (printf, f, s)))
#else
#define ATTRIBUTE_FORMAT_PRINTF(f, s)
#endif

/**
 * Holds generic statistical data
 */
typedef struct lf_stat_ll {
    int64_t average;
    int64_t standard_deviation;
    int64_t variance;
    int64_t max;
} lf_stat_ll;

/**
 * A handy macro that can concatenate three strings.
 * Useful in the LF_PRINT_DEBUG macro and lf_print_error
 * functions that want to concatenate a "DEBUG: " or
 * "ERROR: " to the beginning of the message and a
 * new line format \n at the end.
 */
#define CONCATENATE_THREE_STRINGS(__string1, __string2, __string3) __string1 __string2 __string3

/**
 * Macro for extracting the level from the index of a reaction.
 * A reaction that has no upstream reactions has level 0.
 * Other reactions have a level that is the length of the longest
 * upstream chain to a reaction with level 0 (inclusive).
 * This is used, along with the deadline, to sort reactions
 * in the reaction queue. It ensures that reactions that are
 * upstream in the dependence graph execute before reactions
 * that are downstream.
 */
#define LF_LEVEL(index) (index & 0xffffLL)

/** Utility for finding the maximum of two values. */
#ifndef LF_MAX
#define LF_MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif

/** Utility for finding the minimum of two values. */
#ifndef LF_MIN
#define LF_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#endif

/**
 * LOG_LEVEL is set in generated code to 0 through 4 if the target
 * logging property is error, warning, info, log, or debug.
 * The default level is info (2). Currently, 0, 1, and 2 are
 * treated identically and lf_print_error, lf_print_warning, and lf_print
 * all result in printed output.
 * If log is set (3), then LOG_DEBUG messages
 * will be printed as well.
 * If debug is set (4), the LF_PRINT_DEBUG messages will
 * be printed as well.
 */
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_LOG 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_ALL 255

/** Default log level. */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

/**
 * The ID of this federate. For a non-federated execution, this will
 * be -1.  For a federated execution, it will be assigned when the generated function
 * _lf_initialize_trigger_objects() is called.
 * @see xtext/org.icyphy.linguafranca/src/org/icyphy/generator/CGenerator.xtend.
 */
extern int _lf_my_fed_id;

/**
 * Return the federate ID or -1 if this program is not part of a federation.
 */
int lf_fed_id(void);

/**
 * Report an informational message on stdout with a newline appended at the end.
 * If this execution is federated, then the message will be prefaced by identifying
 * information for the federate. The arguments are just like printf().
 */
void lf_print(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * varargs alternative of "lf_print"
 */
void lf_vprint(const char* format, va_list args)  ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * Report an log message on stdout with the prefix "LOG: " and a newline appended
 * at the end. If this execution is federated, then the message will be prefaced by
 * identifying information for the federate. The arguments are just like printf().
 */
void lf_print_log(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * varargs alternative of "lf_print_log"
 */
void lf_vprint_log(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * A macro used to print useful logging information. It can be enabled
 * by setting the target property 'logging' to 'LOG' or
 * by defining LOG_LEVEL to LOG_LEVEL_LOG or
 * LOG_LEVEL_DEBUG in the top-level preamble.
 * The input to this macro is exactly like printf: (format, ...).
 * "LOG: " is prepended to the beginning of the message
 * and a newline is appended to the end of the message.
 *
 * @note This macro is non-empty even if LOG_LEVEL is not defined in
 * user-code. This is to ensure that the compiler will still parse
 * the predicate inside (...) to prevent LF_PRINT_LOG statements
 * to fall out of sync with the rest of the code. This should have
 * a negligible impact on performance if compiler optimization
 * (e.g., -O2 for gcc) is used as long as the arguments passed to
 * it do not themselves incur significant overhead to evaluate.
 */
#define LF_PRINT_LOG(format, ...) \
            do { if(LOG_LEVEL >= LOG_LEVEL_LOG) { \
                    lf_print_log(format, ##__VA_ARGS__); \
                } } while (0)

/**
 * Report an debug message on stdout with the prefix "DEBUG: " and a newline appended
 * at the end. If this execution is federated, then the message will be prefaced by
 * identifying information for the federate. The arguments are just like printf().
 */
void lf_print_debug(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * varargs alternative of "lf_print_debug"
 */
void lf_vprint_debug(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * A macro used to print useful debug information. It can be enabled
 * by setting the target property 'logging' to 'DEBUG' or
 * by defining LOG_LEVEL to 2 in the top-level preamble.
 * The input to this macro is exactly like printf: (format, ...).
 * "DEBUG: " is prepended to the beginning of the message
 * and a newline is appended to the end of the message.
 *
 * @note This macro is non-empty even if LOG_LEVEL is not defined in
 * user-code. This is to ensure that the compiler will still parse
 * the predicate inside (...) to prevent LF_PRINT_DEBUG statements
 * to fall out of sync with the rest of the code. This should have
 * a negligible impact on performance if compiler optimization
 * (e.g., -O2 for gcc) is used as long as the arguments passed to
 * it do not themselves incur significant overhead to evaluate.
 */
#define LF_PRINT_DEBUG(format, ...) \
            do { if(LOG_LEVEL >= LOG_LEVEL_DEBUG) { \
                    lf_print_debug(format, ##__VA_ARGS__); \
                } } while (0)

/**
 * Print the error defined by the errno variable with the
 * specified message as a prefix, then exit with error code 1.
 * @param msg The prefix to the message.
 */
void error(const char *msg);

/**
 * Report an error with the prefix "ERROR: " and a newline appended
 * at the end.  The arguments are just like printf().
 */
void lf_print_error(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * varargs alternative of "lf_print_error"
 */
void lf_vprint_error(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * Report a warning with the prefix "WARNING: " and a newline appended
 * at the end.  The arguments are just like printf().
 */
void lf_print_warning(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * varargs alternative of "lf_print_warning"
 */
void lf_vprint_warning(const char* format, va_list args) ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * Report an error with the prefix "ERROR: " and a newline appended
 * at the end, then exit with the failure code EXIT_FAILURE.
 * The arguments are just like printf().
 */
void lf_print_error_and_exit(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * Report an error and exit just like lf_print_error_and_exit(), but
 * also print the system error message associated with the error.
 */
void lf_print_error_system_failure(const char* format, ...);

/**
 * varargs alternative of "lf_print_error_and_exit"
 */
void lf_vprint_error_and_exit(const char* format, va_list args)
		ATTRIBUTE_FORMAT_PRINTF(1, 0);

/**
 * Message print function type. The arguments passed to one of
 * these print functions are a printf-style format string followed
 * by a printf-style argument list collected into a va_list
 * (variable argument list).
 */
typedef void(print_message_function_t)(const char*, va_list);

/**
 * Register a function to display messages. After calling this,
 * all messages passed to the above print functions will be
 * printed using the specified function rather than printf
 * if their log level is greater than the specified level.
 * The level should be one of LOG_LEVEL_ERROR, LOG_LEVEL_WARNING,
 * LOG_LEVEL_INFO, LOG_LEVEL_LOG, or LOG_LEVEL_DEBUG.
 *
 * @param function The print message function or NULL to revert
 *  to using printf.
 * @param log_level The level of messages to redirect.
 */
void lf_register_print_function(print_message_function_t* function, int log_level);

/**
 * Assertion handling. LF_ASSERT can be used as a shorthand for verifying
 * a condition and calling `lf_print_error_and_exit` if it is not true.
 * The LF_ASSERT version requires that the condition evaluate to true
 * (non-zero), whereas the LF_ASSERTN version requires that the condition
 * evaluate to false (zero).
 * These are optimized away if the NDEBUG flag is defined.
 */
#if defined(NDEBUG)
#define LF_ASSERT(condition, format, ...) (void)(condition)
#define LF_ASSERTN(condition, format, ...) (void)(condition)
#else
#define LF_ASSERT(condition, format, ...) \
	do { \
		if (!(condition)) { \
				lf_print_error_and_exit(format, ##__VA_ARGS__); \
		} \
	} while(0)
#define LF_ASSERTN(condition, format, ...) \
	do { \
		if (condition) { \
				lf_print_error_and_exit(format, ##__VA_ARGS__); \
		} \
	} while(0)
#endif // NDEBUG

/**
 * Checking mutex locking and unlocking.
 * This is optimized away if the NDEBUG flag is defined.
 */
#define LF_MUTEX_INIT(mutex) LF_ASSERTN(lf_mutex_init(&mutex), "Mutex init failed.")

#define LF_MUTEX_LOCK(mutex) LF_ASSERTN(lf_mutex_lock(&mutex), "Mutex lock failed.")

#define LF_MUTEX_UNLOCK(mutex) LF_ASSERTN(lf_mutex_unlock(&mutex), "Mutex unlock failed.")

#define LF_COND_INIT(cond, mutex) LF_ASSERTN(lf_cond_init(&cond, &mutex), "Condition variable init failed.")

#endif /* UTIL_H */
