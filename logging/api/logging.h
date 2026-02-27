/**
 * @file logging.h
 * @brief Logging API for the C target of Lingua Franca.
 * @ingroup API
 *
 * @author Edward A. Lee
 * @author Peter Donovan
 *
 * This file defines the logging API used throughout the Lingua Franca runtime.
 * It provides functions for different log levels (error, warning, info, log, debug)
 * and allows for custom message handling through function registration.
 */
#include <stdarg.h>

// To silence warnings about a function being a candidate for format checking
// with gcc, add an attribute.
// The arguments are the position of the format string (starting with 1)
// and the start of the remaining arguments, or 0 for vprintf style functions.
/// \cond INTERNAL
#if defined(__GNUC__)
#define ATTRIBUTE_FORMAT_PRINTF(f, s) __attribute__((format(printf, f, s)))
#else
#define ATTRIBUTE_FORMAT_PRINTF(f, s)
#endif
/// \endcond

/**
 * @brief Error log level, which is the lowest log level.
 * @ingroup API
 *
 * This is the lowest log level.
 *
 * @note LOG_LEVEL is set in generated code to 0 through 4 if the target
 * logging property is error, warning, info, log, or debug.
 * The default level is info (2). Currently, 0, 1, and 2 are
 * treated identically and lf_print_error, lf_print_warning, and lf_print
 * LOG_LEVEL is set in generated code to 0 through 4 if the target
 * logging property is error, warning, info, log, or debug.
 * The default level is info (2). Currently, 0, 1, and 2 are
 * treated identically and lf_print_error, lf_print_warning, and lf_print
 * all result in printed output.
 * If log is set (3), then @ref lf_print_log messages will be printed as well.
 * If debug is set (4), the @ref lf_print_debug messages will be printed as well.
 */
#define LOG_LEVEL_ERROR 0

/**
 * @brief Warning log level.
 * @ingroup API
 *
 * @see LOG_LEVEL_ERROR
 */
#define LOG_LEVEL_WARNING 1

/**
 * @brief Warning log level.
 * @ingroup API
 *
 * @see LOG_LEVEL_ERROR
 */
#define LOG_LEVEL_INFO 2

/**
 * @brief Log log level.
 * @ingroup API
 *
 * @see LOG_LEVEL_ERROR
 */
#define LOG_LEVEL_LOG 3

/**
 * @brief Debug log level.
 * @ingroup API
 *
 * @see LOG_LEVEL_ERROR
 */
#define LOG_LEVEL_DEBUG 4

/**
 * @brief All log levels.
 * @ingroup API
 *
 * @see LOG_LEVEL_ERROR
 */
#define LOG_LEVEL_ALL 255

/**
 * @brief Report an informational message on stdout with a newline appended at the end.
 * @ingroup API
 *
 * If this execution is federated, then the message will be prefaced by identifying
 * information for the federate. The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report an log message on stdout with the prefix "LOG: " and a newline appended at the end.
 * @ingroup API
 *
 * If this execution is federated, then the message will be prefaced by identifying
 * information for the federate. The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_log(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report an debug message on stdout with the prefix "DEBUG: " and a newline appended at the end.
 * @ingroup API
 *
 * If this execution is federated, then the message will be prefaced by identifying
 * information for the federate. The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_debug(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report an error with the prefix "ERROR: " and a newline appended at the end.
 * @ingroup API
 *
 * The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_error(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report a warning with the prefix "WARNING: " and a newline appended at the end.
 * @ingroup API
 *
 * The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_warning(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report an error with the prefix "ERROR: " and a newline appended at the end, then exit with the failure code
 * EXIT_FAILURE.
 * @ingroup API
 *
 * The arguments are just like printf().
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_error_and_exit(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * @brief Report an error and exit just like lf_print_error_and_exit(), but also print the system error message
 * associated with the error.
 * @ingroup API
 *
 * @param format The format string to print.
 * @param ... The arguments to print.
 */
void lf_print_error_system_failure(const char* format, ...);

/**
 * @brief Message print function type.
 * @ingroup API
 *
 * The arguments passed to one of these print functions are a printf-style format strin
 * followed by a printf-style argument list collected into a va_list (variable argument list).
 */
typedef void(print_message_function_t)(const char*, va_list);

/**
 * @brief Register a function to display messages.
 * @ingroup API
 *
 * After calling this, all messages passed to the above print functions will be printed using
 * the specified function rather than printf if their log level is greater than the specified level.
 * The level should be one of LOG_LEVEL_ERROR, LOG_LEVEL_WARNING, LOG_LEVEL_INFO, LOG_LEVEL_LOG, or LOG_LEVEL_DEBUG.
 *
 * @param function The print message function or NULL to revert to using printf.
 * @param log_level The level of messages to redirect.
 */
void lf_register_print_function(print_message_function_t* function, int log_level);
