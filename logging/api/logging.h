#include <stdarg.h>

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

/**
 * Report an informational message on stdout with a newline appended at the end.
 * If this execution is federated, then the message will be prefaced by identifying
 * information for the federate. The arguments are just like printf().
 */
void lf_print(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * Report an log message on stdout with the prefix "LOG: " and a newline appended
 * at the end. If this execution is federated, then the message will be prefaced by
 * identifying information for the federate. The arguments are just like printf().
 */
void lf_print_log(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * Report an debug message on stdout with the prefix "DEBUG: " and a newline appended
 * at the end. If this execution is federated, then the message will be prefaced by
 * identifying information for the federate. The arguments are just like printf().
 */
void lf_print_debug(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * Report an error with the prefix "ERROR: " and a newline appended
 * at the end.  The arguments are just like printf().
 */
void lf_print_error(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

/**
 * Report a warning with the prefix "WARNING: " and a newline appended
 * at the end.  The arguments are just like printf().
 */
void lf_print_warning(const char* format, ...) ATTRIBUTE_FORMAT_PRINTF(1, 2);

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
