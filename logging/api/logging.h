// NON-ABI ************************************************************
// This section includes preprocessor code that is closely tied to the ABI, but
// is not part of the ABI itself. Non-C implementations (which cannot benefit
// from the C preprocessor) should ignore this section, or merely use it as a
// suggestion for similar behavior that they should implement using whatever
// metaprogramming facilities their implementation provides in place of the
// preprocessor.

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

/** Default log level. */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

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
 * Assertion handling. LF_ASSERT can be used as a shorthand for verifying
 * a condition and calling `lf_print_error_and_exit` if it is not true.
 * The LF_ASSERT version requires that the condition evaluate to true
 * (non-zero), whereas the LF_ASSERTN version requires that the condition
 * evaluate to false (zero).
 * These are optimized to execute the condition argument but not
 * check the result if the NDEBUG flag is defined.
 * The NDEBUG flag will be defined if the user specifies `build-type: Release`
 * in the target properties of the LF program.
 *
 * LF_ASSERT_NON_NULL can be used to verify that a pointer is not NULL.
 * It differs from LF_ASSERT in that it does nothing at all if the NDEBUG flag is defined.
 */
#if defined(NDEBUG)
#define LF_ASSERT(condition, format, ...) (void)(condition)
#define LF_ASSERTN(condition, format, ...) (void)(condition)
#define LF_ASSERT_NON_NULL(pointer)
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
#define LF_ASSERT_NON_NULL(pointer) \
    do { \
        if (!(pointer)) { \
            lf_print_error_and_exit("Assertion failed: pointer is NULL Out of memory?."); \
        } \
    } while(0)
#endif // NDEBUG

// ABI ****************************************************************
#include <stdarg.h>
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
