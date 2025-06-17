/**
 * @file socket_common.h
 * @brief Common socket operations and utilities for federated Lingua Franca programs.
 * @ingroup Federated
 *
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Peter Donovan
 *
 * This file provides common socket operations and utilities used in federated Lingua Franca programs.
 * It includes functions for creating and managing TCP/UDP sockets, handling connections,
 * and performing read/write operations with proper error handling and retry mechanisms.
 * The file also defines various constants for timeouts, retry intervals, and port configurations.
 */
#ifndef SOCKET_COMMON_H
#define SOCKET_COMMON_H

#include "low_level_platform.h"

/**
 * @brief The number of federates.
 * @ingroup Federated
 *
 * This defaults to 1.
 */
#ifndef NUMBER_OF_FEDERATES
#define NUMBER_OF_FEDERATES 1
#endif

/**
 * @brief The amount of time to wait after a failed socket read or write before trying again.
 * @ingroup Federated
 *
 * This defaults to 100 ms.
 */
#define DELAY_BETWEEN_SOCKET_RETRIES MSEC(100)

/**
 * @brief The timeout time in ns for TCP operations.
 * @ingroup Federated
 *
 * Default value is 10 secs.
 */
#define TCP_TIMEOUT_TIME SEC(10)

/**
 * @brief The timeout time in ns for UDP operations.
 * @ingroup Federated
 *
 * Default value is 1 sec.
 */
#define UDP_TIMEOUT_TIME SEC(1)

/**
 * @brief Time between a federate's attempts to connect to the RTI.
 * @ingroup Federated
 */
#define CONNECT_RETRY_INTERVAL MSEC(500)

/**
 * @brief Bound on the number of retries to connect to the RTI.
 * @ingroup Federated
 *
 * A federate will retry every CONNECT_RETRY_INTERVAL nanoseconds until
 * CONNECTION_TIMEOUT expires.
 */
#define CONNECT_TIMEOUT MINUTES(1)

/**
 * @brief Maximum number of port addresses that a federate will try to connect to the RTI on.
 * @ingroup Federated
 *
 * If you are using automatic ports begining at DEFAULT_PORT, this puts an upper bound
 * on the number of RTIs that can be running on the same host.
 */
#define MAX_NUM_PORT_ADDRESSES 16u

/**
 * @brief Time to wait before re-attempting to bind to a port.
 * @ingroup Federated
 *
 * When a process closes, the network stack typically waits between 30 and 120
 * seconds before releasing the port.  This is to allow for delayed packets so
 * that a new process does not receive packets from a previous process.
 * Here, we limit the retries to 60 seconds.
 */
#define PORT_BIND_RETRY_INTERVAL SEC(1)

/**
 * @brief Number of attempts to bind to a port before giving up.
 * @ingroup Federated
 */
#define PORT_BIND_RETRY_LIMIT 60

/**
 * @brief Default port number for the RTI.
 * @ingroup Federated
 *
 * Unless a specific port has been specified by the LF program in the "at"
 * for the RTI or on the command line, when the RTI starts up, it will attempt
 * to open a socket server on this port.
 */
#define DEFAULT_PORT 15045u

/**
 * @brief Byte identifying that the federate or the RTI has failed.
 * @ingroup Federated
 */
#define MSG_TYPE_FAILED 25

/**
 * @brief Type of socket.
 * @ingroup Federated
 */
typedef enum socket_type_t { TCP, UDP } socket_type_t;

/**
 * @brief Create an IPv4 TCP socket with Nagle's algorithm disabled.
 * @ingroup Federated
 *
 * This uses TCP_NODELAY and Delayed ACKs disabled with TCP_QUICKACK.
 * It exits application on any error.
 *
 * @return The socket ID (a file descriptor).
 */
int create_real_time_tcp_socket_errexit();

/**
 * @brief Create a TCP server that listens for socket connections.
 * @ingroup Federated
 *
 * If the specified port number is greater than zero, this function will attempt to acquire that port.
 * If the specified port number is zero, and the increment_port_on_retry is true, it will attempt to acquire
 * DEFAULT_PORT. If it fails to acquire DEFAULT_PORT, then it will increment the port number from DEFAULT_PORT on each
 * attempt until it has incremented MAX_NUM_PORT_ADDRESSES times, at which point it will cycle around and begin again
 * with DEFAULT_PORT.
 * If the port number is zero, and the increment_port_on_retry is false, it delegates to the operating system to provide
 * an available port number.
 * If acquiring the port fails, then this function will repeatedly attempt up to PORT_BIND_RETRY_LIMIT times with a
 * delay of PORT_BIND_RETRY_INTERVAL in between each try.
 *
 * @param port The port number to use or 0 to let the OS pick or 1 to start trying at DEFAULT_PORT.
 * @param final_socket Pointer to the returned socket descriptor on which accepting connections will occur.
 * @param final_port Pointer to the final port the server will use.
 * @param sock_type Type of the socket, TCP or UDP.
 * @param increment_port_on_retry Boolean to retry port increment.
 * @return 0 for success, -1 for failure.
 */
int create_server(uint16_t port, int* final_socket, uint16_t* final_port, socket_type_t sock_type,
                  bool increment_port_on_retry);

/**
 * @brief Wait for an incoming connection request on the specified server socket.
 * @ingroup Federated
 *
 * This blocks until a connection is successfully accepted. If an error occurs that is not
 * temporary (e.g., `EAGAIN` or `EWOULDBLOCK`), it reports the error and exits. Temporary
 * errors cause the function to retry accepting the connection.
 *
 * If the `rti_socket` is not -1, this function checks whether the specified socket is still open.
 * If it is not open, then this function returns -1.
 * This is useful for federates to determine whether they are still connected to the federation
 * and to stop waiting when they are not.
 *
 * @param socket The server socket file descriptor that is listening for incoming connections.
 * @param rti_socket The rti socket for the federate to check if it is still open.
 * @return The file descriptor for the newly accepted socket on success, or -1 on failure
 *             (with an appropriate error message printed).
 */

int accept_socket(int socket, int rti_socket);

/**
 * @brief Attempt to establish a TCP connection to the specified hostname and port.
 * @ingroup Federated
 *
 * Attempt to establish a TCP connection to the specified hostname
 * and port. This function uses `getaddrinfo` to resolve the hostname and retries the connection
 * periodically if it fails. If the specified port is 0, it iterates through a range
 * of default ports starting from `DEFAULT_PORT`. The function will stop retrying
 * if the `CONNECT_TIMEOUT` is reached.
 *
 * @param sock The socket file descriptor that has already been created (using `socket()`).
 * @param hostname The hostname or IP address of the server to connect to.
 * @param port The port number to connect to. If 0 is specified, a default port range will be used.
 * @return 0 on success, -1 on failure, and `errno` is set to indicate the specific error.
 */
int connect_to_socket(int sock, const char* hostname, int port);

/**
 * @brief Read the specified number of bytes from the specified socket into the specified buffer.
 * @ingroup Federated
 *
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
int read_from_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read the specified number of bytes to the specified socket using @ref read_from_socket.
 * @ingroup Federated
 *
 * It closes the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 *
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read the specified number of bytes from the specified socket into the specified
 * buffer and close the socket if an error occurs.
 * @ingroup Federated
 *
 * If a disconnect or an EOF occurs during this reading, then if format is non-null,
 * report an error and exit.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 * @param socket The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 */
void read_from_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...);

/**
 * @brief Without blocking, peek at the specified socket.
 * @ingroup Federated
 *
 * If there is anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs, return -1.
 *
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_socket(int socket, unsigned char* result);

/**
 * @brief Write the specified number of bytes to the specified socket from the specified buffer.
 * @ingroup Federated
 *
 * If an error occurs, return -1 and set errno to indicate the cause of the error.
 * If the write succeeds, return 0.
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
int write_to_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write the specified number of bytes to the specified socket.
 * @ingroup Federated
 *
 * This uses @ref write_to_socket and closes the socket if an error occurs.
 * If an error occurs, this will change the socket ID pointed to by the first argument to -1 and will return -1.
 *
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write the specified number of bytes to the specified socket.
 * @ingroup Federated
 *
 * This uses @ref write_to_socket_close_on_error and exits with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If the format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 *
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);

/**
 * @brief Initialize shutdown mutex.
 * @ingroup Federated
 *
 * This is used to synchronize the shutdown of the federate.
 */
void init_shutdown_mutex(void);

/**
 * @brief Shutdown and close the socket.
 * @ingroup Federated
 *
 * If read_before_closing is false, it just immediately calls shutdown() with SHUT_RDWR and close().
 * If read_before_closing is true, it calls shutdown with SHUT_WR, only disallowing further writing.
 * Then, it calls read() until EOF is received, and discards all received bytes.
 *
 * @param socket Pointer to the socket descriptor to shutdown and close.
 * @param read_before_closing If true, read until EOF before closing the socket.
 * @return int 0 for success and -1 for an error.
 */
int shutdown_socket(int* socket, bool read_before_closing);

#endif /* SOCKET_COMMON_H */
