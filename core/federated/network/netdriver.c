#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "netdriver.h"
#include "util.h"

netdrv_t* initialize_common_netdrv(int federate_id, const char* federation_id) {
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (!drv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(drv, 0, sizeof(netdrv_t));
  drv->read_remaining_bytes = 0;
  drv->federate_id = federate_id;
  drv->federation_id = federation_id;
  return drv;
}

/**
 * Write the specified number of bytes to the specified netdriver using write_to_netdriver
 * and close the netdriver if an error occurs. If an error occurs, this will change the
 * netdriver pointed to by the first argument to -1 and will return -1.
 * @param netdriver Pointer to the netdriver.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  int bytes_written = write_to_netdrv(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    // Netdrv has probably been closed from the other side.
    // Shut down and close the netdrv from this side.
    close_netdrv(drv);
  }
  return bytes_written;
}

/**
 * Write the specified number of bytes to the specified netdriver using
 * write_to_netdriver_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param drv Pointer to the netdriver.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {
  va_list args;
  int bytes_written = write_to_netdrv_close_on_error(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error("Failed to write to netdriver. Closing it.");
    }
  }
}

/**
 * Read the specified number of bytes to the specified netdriver using read_from_netdriver
 * and close the netdriver if an error occurs. If an error occurs, this will change the
 * netdriver pointed to by the first argument to -1 and will return -1.
 * @param netdriver Pointer to the netdriver.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  ssize_t bytes_read = read_from_netdrv(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    close_netdrv(drv);
    return -1;
  }
  return bytes_read;
}

/**
 * Read the specified number of bytes from the specified netdriver into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 * @param netdriver The netdriver.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 * @return The number of bytes read, or 0 if an EOF is received, or
 *  a negative number for an error.
 */
void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {
  va_list args;
  ssize_t bytes_read = read_from_netdrv_close_on_error(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    // Read failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error_system_failure("Failed to read from netdrv.");
    }
  }
}
