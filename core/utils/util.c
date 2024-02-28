/**
 * @file
 * @author Edward A. Lee (eal@berkeley.edu)
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
 * Utility functions for managing output the user, error and warning
 * messages, logging, and debug messages. Outputs are filtered based on
 * the target "logging" parameter.
 */

#include "util.h"

#ifndef STANDALONE_RTI
#include "environment.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>     // Defines memcpy()
#include <stdarg.h>     // Defines va_list
#include <time.h>       // Defines nanosleep()
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
int _lf_my_fed_id = -1;

/**
 * If non-null, this function will be used instead of the printf to
 * print messages.
 */
print_message_function_t* print_message_function = NULL;

/** The level of messages to redirect to print_message_function. */
int print_message_level = -1;

int lf_fed_id() {
	return _lf_my_fed_id;
}

// Declaration needed to attach attributes to suppress warnings of the form:
// "warning: function '_lf_message_print' might be a candidate for 'gnu_printf'
// format attribute [-Wsuggest-attribute=format]"
void _lf_message_print(
		int is_error, const char* prefix, const char* format, va_list args, int log_level
) ATTRIBUTE_FORMAT_PRINTF(3, 0);

/**
 * Print a fatal error message. Internal function.
 */
static void lf_vprint_fatal_error(const char* format, va_list args) {
    _lf_message_print(1, "FATAL ERROR: ", format, args, LOG_LEVEL_ERROR);
}

/**
 * Internal implementation of the next few reporting functions.
 */
void _lf_message_print(
		int is_error, const char* prefix, const char* format, va_list args, int log_level
) {  // Disable warnings about format check.
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
		if (_lf_my_fed_id < 0) {
			size_t length = strlen(prefix) + strlen(format) + 32;
			message = (char*) malloc(length + 1);
			snprintf(message, length, "%s%s\n",
					prefix, format);
		} else {
#if defined STANDALONE_RTI
			size_t length = strlen(prefix) + strlen(format) + 37;
			message = (char*) malloc(length + 1);
			snprintf(message, length, "RTI: %s%s\n",
					prefix, format);
#else
			// Get the federate name from the top-level environment, which by convention is the first.
			environment_t *envs;
			_lf_get_environments(&envs);
			char* name = envs->name;
			size_t length = strlen(prefix) + strlen(format) + +strlen(name) + 32;
			message = (char*) malloc(length + 1);
			// If the name has prefix "federate__", strip that out.
			if (strncmp(name, "federate__", 10) == 0) name += 10;

			snprintf(message, length, "Fed %d (%s): %s%s\n",
					_lf_my_fed_id, name, prefix, format);
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
    va_start (args, format);
    lf_vprint(format, args);
	va_end (args);
}

void lf_vprint(const char* format, va_list args) {
    _lf_message_print(0, "", format, args, LOG_LEVEL_INFO);
}

void lf_print_log(const char* format, ...) {
	va_list args;
    va_start (args, format);
	lf_vprint_log(format, args);
	va_end (args);
}

void lf_vprint_log(const char* format, va_list args) {
    _lf_message_print(0, "LOG: ", format, args, LOG_LEVEL_LOG);
}

void lf_print_debug(const char* format, ...) {
    va_list args;
    va_start (args, format);
    lf_vprint_debug(format, args);
    va_end (args);
}

void lf_vprint_debug(const char* format, va_list args) {
    _lf_message_print(0, "DEBUG: ", format, args, LOG_LEVEL_DEBUG);
}

void lf_print_error(const char* format, ...) {
    va_list args;
    va_start (args, format);
    lf_vprint_error(format, args);
    va_end (args);
}

void lf_vprint_error(const char* format, va_list args) {
    _lf_message_print(1, "ERROR: ", format, args, LOG_LEVEL_ERROR);
}

void lf_print_warning(const char* format, ...) {
    va_list args;
    va_start (args, format);
    lf_vprint_warning(format, args);
    va_end (args);
}

void lf_vprint_warning(const char* format, va_list args) {
    _lf_message_print(1, "WARNING: ", format, args, LOG_LEVEL_WARNING);
}

void lf_print_error_and_exit(const char* format, ...) {
    va_list args;
    va_start (args, format);
    lf_vprint_fatal_error(format, args);
    va_end (args);
	fflush(stdout);
    exit(EXIT_FAILURE);
}

void lf_print_error_system_failure(const char* format, ...) {
    va_list args;
    va_start (args, format);
    lf_vprint_error(format, args);
    va_end (args);
    lf_print_error_and_exit("Error %d: %s", errno, strerror(errno));
    exit(EXIT_FAILURE);
}

void lf_register_print_function(print_message_function_t* function, int log_level) {
    print_message_function = function;
    print_message_level = log_level;
}

