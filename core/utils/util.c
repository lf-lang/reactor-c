/**
 * @file
 * @author Edward A. Lee
 *
 * @brief Utility functions for managing output of messages.
 *
 * Utility functions for managing output the user, error and warning
 * messages, logging, and debug messages. Outputs are filtered based on
 * the target "logging" parameter.
 */

#include "util.h"

#include <stdio.h>

#ifndef STANDALONE_RTI
#include "environment.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h> // Defines memcpy()
#include <stdarg.h> // Defines va_list
#include <time.h>   // Defines nanosleep()
#include <stdbool.h>

#ifndef NUMBER_OF_FEDERATES
#define NUMBER_OF_FEDERATES 1
#endif

/** Number of nanoseconds to sleep before retrying a socket read. */
#define SOCKET_READ_RETRY_INTERVAL 1000000

/**
 * The ID of this federate. For a non-federated execution, this will be -1.
 * For a federated execution, it will be assigned in the generated code.
 */
uint16_t _lf_my_fed_id = UINT16_MAX; // Federate IDs are counted up from 0. If we hit this, we have too many.

/**
 * If non-null, this function will be used instead of the printf to
 * print messages.
 */
print_message_function_t* print_message_function = NULL;

/** The level of messages to redirect to print_message_function. */
int print_message_level = -1;

uint16_t lf_fed_id() { return _lf_my_fed_id; }

// Declaration needed to attach attributes to suppress warnings of the form:
// "warning: function '_lf_message_print' might be a candidate for 'gnu_printf'
// format attribute [-Wsuggest-attribute=format]"
void _lf_message_print(const char* prefix, const char* format, va_list args, int log_level)
    ATTRIBUTE_FORMAT_PRINTF(2, 0);

/**
 * Print a fatal error message. Internal function.
 */
static void lf_vprint_fatal_error(const char* format, va_list args) {
  _lf_message_print("FATAL ERROR: ", format, args, LOG_LEVEL_ERROR);
}

/**
 * Internal implementation of the next few reporting functions.
 */
void _lf_message_print(const char* prefix, const char* format, va_list args,
                       int log_level) { // Disable warnings about format check.
  // The logging level may be set either by a LOG_LEVEL #define
  // (which is code generated based on the logging target property)
  // or by a lf_register_print_function() call. Honor both. If neither
  // has been set, then assume LOG_LEVEL_INFO. If both have been set,
  // then honor the maximum.
  int print_level = -1;
#ifdef LOG_LEVEL
  print_level = LOG_LEVEL;
#endif
  if (print_level < print_message_level) {
    print_level = print_message_level;
  }
  if (print_level < 0) {
    // Neither has been set.
    print_level = LOG_LEVEL_INFO;
  }
  if (log_level <= print_level) {
    // Rather than calling printf() multiple times, we need to call it just
    // once because this function is invoked by multiple threads.
    // If we make multiple calls to printf(), then the results could be
    // interleaved between threads.
    // vprintf() is a version that takes an arg list rather than multiple args.
    char* message;
    if (_lf_my_fed_id == UINT16_MAX) {
      size_t length = strlen(prefix) + strlen(format) + 32;
      message = (char*)malloc(length + 1);
      snprintf(message, length, "%s%s\n", prefix, format);
    } else {
#if defined STANDALONE_RTI
      size_t length = strlen(prefix) + strlen(format) + 37;
      message = (char*)malloc(length + 1);
      snprintf(message, length, "RTI: %s%s\n", prefix, format);
#else
      // Get the federate name from the top-level environment, which by convention is the first.
      environment_t* envs;
      _lf_get_environments(&envs);
      char* name = envs->name;
      size_t length = strlen(prefix) + strlen(format) + +strlen(name) + 32;
      message = (char*)malloc(length + 1);
      // If the name has prefix "federate__", strip that out.
      if (strncmp(name, "federate__", 10) == 0)
        name += 10;

      snprintf(message, length, "Fed %d (%s): %s%s\n", _lf_my_fed_id, name, prefix, format);
#endif // STANDALONE_RTI
    }
    if (print_message_function == NULL) {
      // NOTE: Send all messages to stdout, not to stderr, so that ordering makes sense.
      vfprintf(stdout, message, args);
    } else {
      (*print_message_function)(message, args);
    }
    free(message);
  }
}

void lf_print(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint(format, args);
  va_end(args);
}

void lf_vprint(const char* format, va_list args) { _lf_message_print("", format, args, LOG_LEVEL_INFO); }

void lf_print_log(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_log(format, args);
  va_end(args);
}

void lf_vprint_log(const char* format, va_list args) { _lf_message_print("LOG: ", format, args, LOG_LEVEL_LOG); }

void lf_print_debug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_debug(format, args);
  va_end(args);
}

void lf_vprint_debug(const char* format, va_list args) { _lf_message_print("DEBUG: ", format, args, LOG_LEVEL_DEBUG); }

void lf_print_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_error(format, args);
  va_end(args);
}

void lf_vprint_error(const char* format, va_list args) { _lf_message_print("ERROR: ", format, args, LOG_LEVEL_ERROR); }

void lf_print_warning(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_warning(format, args);
  va_end(args);
}

void lf_vprint_warning(const char* format, va_list args) {
  _lf_message_print("WARNING: ", format, args, LOG_LEVEL_WARNING);
}

void lf_print_error_and_exit(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_fatal_error(format, args);
  va_end(args);
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void lf_print_error_system_failure(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lf_vprint_error(format, args);
  va_end(args);
  // Windows warns that strerror is deprecated but doesn't define strerror_r.
  // There seems to be no portable replacement.
  lf_print_error_and_exit("Error %d: %s", errno, strerror(errno));
  exit(EXIT_FAILURE);
}

void lf_register_print_function(print_message_function_t* function, int log_level) {
  print_message_function = function;
  print_message_level = log_level;
}
