/**
 * @file logging_macros.h
 * @brief Logging macros for the logging API.
 * @ingroup Internal
 *
 * Non-C implementations (which cannot benefit from the C preprocessor) should
 * ignore this file, or merely use it as a suggestion for similar behavior
 * that they should implement using whatever metaprogramming facilities their
 * implementation provides in place of the preprocessor.
 */
#ifndef LOGGING_MACROS_H
#define LOGGING_MACROS_H
#include "logging.h"
#include <stdbool.h>

/** Default log level. */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// To prevent warnings "conditional expression is constant", we define static booleans
// here instead of directly testing LOG_LEVEL in the if statements in the macros below.
static const bool _lf_log_level_is_log = LOG_LEVEL >= LOG_LEVEL_LOG;
static const bool _lf_log_level_is_debug = LOG_LEVEL >= LOG_LEVEL_DEBUG;

/**
 * @brief A macro used to print useful logging information.
 * @ingroup API
 *
 * In contrast to the function @ref lf_print_log, this macro avoids the overhead if logging is disabled.
 *
 * It can be enabled by setting the target property 'logging' to 'LOG' or
 * by defining LOG_LEVEL to LOG_LEVEL_LOG or LOG_LEVEL_DEBUG in the top-level preamble.
 * The input to this macro is exactly like printf: (format, ...).
 * "LOG: " is prepended to the beginning of the message and a newline is appended to the end of the message.
 *
 * @note This macro is non-empty even if LOG_LEVEL is not defined in
 * user-code. This is to ensure that the compiler will still parse
 * the predicate inside (...) to prevent LF_PRINT_LOG statements
 * to fall out of sync with the rest of the code. This should have
 * a negligible impact on performance if compiler optimization
 * (e.g., -O2 for gcc) is used as long as the arguments passed to
 * it do not themselves incur significant overhead to evaluate.
 */
#define LF_PRINT_LOG(format, ...)                                                                                      \
  do {                                                                                                                 \
    if (_lf_log_level_is_log) {                                                                                        \
      lf_print_log(format, ##__VA_ARGS__);                                                                             \
    }                                                                                                                  \
  } while (0)

/**
 * @brief A macro used to print useful debug information.
 * @ingroup API
 *
 * In contrast to the function @ref lf_print_debug, this macro avoids the overhead if logging is disabled.
 *
 * It can be enabled by setting the target property 'logging' to 'DEBUG' or
 * by defining LOG_LEVEL to LOG_LEVEL_DEBUG in the top-level preamble.
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
#define LF_PRINT_DEBUG(format, ...)                                                                                    \
  do {                                                                                                                 \
    if (_lf_log_level_is_debug) {                                                                                      \
      lf_print_debug(format, ##__VA_ARGS__);                                                                           \
    }                                                                                                                  \
  } while (0)

#if defined(NDEBUG)
#define LF_ASSERT(condition, format, ...) (void)(condition)
#define LF_ASSERTN(condition, format, ...) (void)(condition)
#define LF_ASSERT_NON_NULL(pointer) (void)(pointer)
#else

/**
 * @brief Assert that a condition is true.
 * @ingroup API
 *
 * This will verify that the condition is true and call @ref lf_print_error_and_exit if it is not true.
 * The remaining arguments are passed to @ref lf_print_error_and_exit as the format string and arguments.
 *
 * This is optimized to execute the condition argument but not
 * check the result if the NDEBUG flag is defined.
 * The NDEBUG flag will be defined if the user specifies `build-type: Release`
 * in the target properties of the LF program.
 *
 * @param condition The condition to verify.
 * @param format The format string to pass to @ref lf_print_error_and_exit.
 * @param ... The arguments to pass to @ref lf_print_error_and_exit.
 */
#define LF_ASSERT(condition, format, ...)                                                                              \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      lf_print_error_and_exit("`" format "`. Failed assertion in %s:%d(%s):(" #condition ") != true`", ##__VA_ARGS__,  \
                              __FILE__, __LINE__, __func__);                                                           \
    }                                                                                                                  \
  } while (0)

/**
 * @brief Assert that a condition is false.
 * @ingroup API
 *
 * This will verify that the condition is false and call @ref lf_print_error_and_exit if it is not false.
 * The remaining arguments are passed to @ref lf_print_error_and_exit as the format string and arguments.
 *
 * This is optimized to execute the condition argument but not check the result if the NDEBUG flag is defined.
 * The NDEBUG flag will be defined if the user specifies `build-type: Release` in the target properties of the LF
 * program.
 *
 * @param condition The condition to verify.
 * @param format The format string to pass to @ref lf_print_error_and_exit.
 * @param ... The arguments to pass to @ref lf_print_error_and_exit.
 */
#define LF_ASSERTN(condition, format, ...)                                                                             \
  do {                                                                                                                 \
    if (condition) {                                                                                                   \
      lf_print_error_and_exit("`" format "`. Failed assertion in %s:%d(%s):(" #condition ") != false`", ##__VA_ARGS__, \
                              __FILE__, __LINE__, __func__);                                                           \
    }                                                                                                                  \
  } while (0)

/**
 * @brief Assert that a pointer is not NULL.
 * @ingroup API
 *
 * This will verify that the pointer is non-NULL and call @ref lf_print_error_and_exit if it is NULL.
 *
 * This differs from @ref LF_ASSERT in that it does nothing at all if the NDEBUG flag is defined.
 *
 * @param pointer The pointer to verify.
 */

#define LF_ASSERT_NON_NULL(pointer)                                                                                    \
  do {                                                                                                                 \
    if (!(pointer)) {                                                                                                  \
      lf_print_error_and_exit("`Out of memory?` Assertion failed in %s:%d(%s):`" #pointer " == NULL`", __FILE__,       \
                              __LINE__, __func__);                                                                     \
    }                                                                                                                  \
  } while (0)
#endif // NDEBUG

/**
 * @brief Check that a condition is true.
 * @ingroup API
 *
 * This will verify that the condition is true and call @ref lf_print_error_and_exit if it is not true.
 * The remaining arguments are passed to @ref lf_print_error_and_exit as the format string and arguments.
 * This is just like `LF_ASSERT`, except that it is not optimized away when the `NDEBUG` flag is defined.
 *
 * @param condition The condition to verify.
 * @param format The format string to pass to @ref lf_print_error_and_exit.
 * @param ... The arguments to pass to @ref lf_print_error_and_exit.
 */
#define LF_TEST(condition, format, ...)                                                                                \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      lf_print_error_and_exit("`" format "`. Failed assertion in %s:%d(%s):(" #condition ") != true`", ##__VA_ARGS__,  \
                              __FILE__, __LINE__, __func__);                                                           \
    }                                                                                                                  \
  } while (0)

#endif // LOGGING_MACROS_H