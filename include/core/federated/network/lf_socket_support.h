#ifndef LF_SOCKET_SUPPORT_H
#define LF_SOCKET_SUPPORT_H

#include <netinet/in.h>     // IPPROTO_TCP, IPPROTO_UDP 
#include <netinet/tcp.h>    // TCP_NODELAY 

#include "net_util.h"

/**
 * The timeout time in ns for TCP operations.
 * Default value is 10 secs.
 */
#define TCP_TIMEOUT_TIME SEC(10)

/**
 * The timeout time in ns for UDP operations.
 * Default value is 1 sec.
 */
#define UDP_TIMEOUT_TIME SEC(1)

/**
 * Maximum number of port addresses that a federate will try to connect to the RTI on.
 * If you are using automatic ports begining at DEFAULT_PORT, this puts an upper bound
 * on the number of RTIs that can be running on the same host.
 */
#define MAX_NUM_PORT_ADDRESSES 16

/**
 * Time to wait before re-attempting to bind to a port.
 * When a process closes, the network stack typically waits between 30 and 120
 * seconds before releasing the port.  This is to allow for delayed packets so
 * that a new process does not receive packets from a previous process.
 * Here, we limit the retries to 60 seconds.
 */
#define PORT_BIND_RETRY_INTERVAL SEC(1)

/**
 * Number of attempts to bind to a port before giving up.
 */
#define PORT_BIND_RETRY_LIMIT 60

/**
 * Default port number for the RTI.
 * Unless a specific port has been specified by the LF program in the "at"
 * for the RTI or on the command line, when the RTI starts up, it will attempt
 * to open a socket server on this port.
 */
#define DEFAULT_PORT 15045u

#define DELAY_BETWEEN_SOCKET_RETRIES MSEC(100)

typedef struct socket_priv_t {
    int port;
    int socket_descriptor;
	int proto;
	uint16_t user_specified_port;

    // Used for the RTI, to read the connected federate's info.
    char server_hostname[INET_ADDRSTRLEN]; // Human-readable IP address and
    int32_t server_port;    // port number of the socket server of the federate
                            // if it has any incoming direct connections from other federates.
                            // The port number will be -1 if there is no server or if the
                            // RTI has not been informed of the port number.
    struct in_addr server_ip_addr; // Information about the IP address of the socket
                                // server of the federate.
} socket_priv_t;

char *get_host_name(netdrv_t *drv);
int32_t *get_port(netdrv_t *drv);
struct in_addr *get_ip_addr(netdrv_t *drv);

netdrv_t * netdrv_init();

/**
 * Create a server and enable listening for socket connections.
 * If the specified port if it is non-zero, it will attempt to acquire that port.
 * If it fails, it will repeatedly attempt up to PORT_BIND_RETRY_LIMIT times with
 * a delay of PORT_BIND_RETRY_INTERVAL in between. If the specified port is
 * zero, then it will attempt to acquire DEFAULT_PORT first. If this fails, then it
 * will repeatedly attempt up to PORT_BIND_RETRY_LIMIT times, incrementing the port
 * number between attempts, with no delay between attempts.  Once it has incremented
 * the port number MAX_NUM_PORT_ADDRESSES times, it will cycle around and begin again
 * with DEFAULT_PORT.
 *
 * @param port The port number to use or 0 to start trying at DEFAULT_PORT.
 * @param socket_type The type of the socket for the server (TCP or UDP).
 * @return The socket descriptor on which to accept connections.
 */
//TODO: Update descriptions.
int create_rti_server(netdrv_t *drv, netdrv_type_t netdrv_type);


// int create_real_time_tcp_socket_errexit();
void close_netdrvs(netdrv_t *rti_netdrv, netdrv_t *clock_netdrv);

netdrv_t *netdrv_accept(netdrv_t *rti_netdrv);

netdrv_t *accept_connection(netdrv_t * rti_netdrv);



/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_netdrv(netdrv_t *drv, unsigned char* result);

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
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv(netdrv_t *drv, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using write_to_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv_close_on_error(netdrv_t *drv, size_t num_bytes, unsigned char* buffer);

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
void write_to_netdrv_fail_on_error(
		netdrv_t *drv,
		size_t num_bytes,
		unsigned char* buffer,
		lf_mutex_t* mutex,
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
ssize_t read_from_netdrv(netdrv_t* netdrv, unsigned char* buffer, size_t buffer_length);

/**
 * Read the specified number of bytes to the specified socket using read_from_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
ssize_t read_from_netdrv_close_on_error(netdrv_t *drv, unsigned char* buffer, size_t buffer_length);

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
void read_from_netdrv_fail_on_error(netdrv_t *drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex, char* format, ...);

#endif // LF_SOCKET_SUPPORT_H
