#ifndef NETDRIVER_H
#define NETDRIVER_H

#include "low_level_platform.h"

#include "socket_common.h"

#if defined(COMM_TYPE_TCP)
#include "lf_socket_support.h"
#elif defined(COMM_TYPE_MQTT)
#include "lf_mqtt_support.h"
#elif defined(COMM_TYPE_SST)
#include "lf_sst_support.h"
#endif

typedef enum netdrv_type_t { NETDRV, UDP } netdrv_type_t;

// Just doing 0 for RTI, 1 for FED
// typedef enum server_type_t { RTI, FED } server_type_t; 

typedef struct netdrv_t {
  void* priv;
  unsigned int read_remaining_bytes;
  int federate_id;
  const char* federation_id;
} netdrv_t;


void close_netdrv(netdrv_t* drv);

netdrv_t* netdrv_init(int federate_id, const char* federation_id);

// Port will be NULL on MQTT.
int create_server(netdrv_t* drv, int server_type, uint16_t port);


void create_client(netdrv_t* drv);

// Returns socket number of clock_sync_server.
int create_clock_sync_server(uint16_t* clock_sync_port);

netdrv_t* establish_communication_session(netdrv_t* netdrv);

/**
 * Write the specified number of bytes to the specified socket from the
 * specified buffer. If an error occurs, return -1 and set errno to indicate
 * the cause of the error. If the write succeeds, return 0.
 * This function repeats the attempt until the specified number of bytes
 * have been written or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by
 * DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return The number of bytes written.
 */
int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using write_to_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using
 * write_to_socket_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);

/**
 * Read the specified number of bytes from the specified socket into the specified buffer.
 * If an error occurs during this reading, return -1 and set errno to indicate
 * the cause of the error. If the read succeeds in reading the specified number of bytes,
 * return 0. If an EOF occurs before reading the specified number of bytes, return 1.
 * This function repeats the read attempt until the specified number of bytes
 * have been read, an EOF is read, or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by DELAY_BETWEEN_SOCKET_RETRIES.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an error.
 */
ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length);

/**
 * Read the specified number of bytes to the specified socket using read_from_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length);

/**
 * Read the specified number of bytes from the specified socket into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 * @return The number of bytes read, or 0 if an EOF is received, or
 *  a negative number for an error.
 */
void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...);

// Temporary //////////////////
char* get_host_name(netdrv_t* drv);
int32_t get_my_port(netdrv_t* drv);
int32_t get_port(netdrv_t* drv);
struct in_addr* get_ip_addr(netdrv_t* drv);

void set_host_name(netdrv_t* drv, const char* hostname);
void set_port(netdrv_t* drv, int port);
void set_specified_port(netdrv_t* drv, int port);
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr);

void netdrv_free(netdrv_t* drv);

int netdrv_connect(netdrv_t* drv);

/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result);

////////////////////////////

#endif /* NETDRIVER_H */
