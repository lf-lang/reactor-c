#include "logging.h"

/**
 * Non-C implementations (which cannot benefit from the C preprocessor) should
 * ignore this file, or merely use it as a suggestion for similar behavior
 * that they should implement using whatever metaprogramming facilities their
 * implementation provides in place of the preprocessor.
*/

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
            lf_print_error_and_exit("`" format "`. Failed assertion in %s:%d(%s):(" #condition \
            ") != true`", ##__VA_ARGS__, __FILE__, __LINE__, __func__); \
		} \
	} while(0)
#define LF_ASSERTN(condition, format, ...) \
	do { \
		if (condition) { \
            lf_print_error_and_exit("`" format "`. Failed assertion in %s:%d(%s):(" #condition \
            ") != false`", ##__VA_ARGS__, __FILE__, __LINE__, __func__); \
		} \
	} while(0)
#define LF_ASSERT_NON_NULL(pointer) \
    do { \
        if (!(pointer)) { \
            lf_print_error_and_exit("`Out of memory?` Assertion failed in %s:%d(%s):`" #pointer " == NULL`",\
            __FILE__, __LINE__, __func__); \
        } \
    } while(0)
#endif // NDEBUG
