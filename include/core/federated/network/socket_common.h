#ifndef SOCKET_COMMON_H
#define SOCKET_COMMON_H

#include "low_level_platform.h"
#define NUM_SOCKET_RETRIES 10
#define DELAY_BETWEEN_SOCKET_RETRIES MSEC(100)

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
 * Time between a federate's attempts to connect to the RTI.
 */
#define CONNECT_RETRY_INTERVAL MSEC(500)

/**
 * Bound on the number of retries to connect to the RTI.
 * A federate will retry every CONNECT_RETRY_INTERVAL seconds until
 * CONNECTION_TIMEOUT expires.
 */
#define CONNECT_TIMEOUT MINUTES(1)

/**
 * Maximum number of port addresses that a federate will try to connect to the RTI on.
 * If you are using automatic ports begining at DEFAULT_PORT, this puts an upper bound
 * on the number of RTIs that can be running on the same host.
 */
#define MAX_NUM_PORT_ADDRESSES 16u

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

typedef enum socket_type_t { TCP, UDP } socket_type_t;

/**
 * Mutex protecting socket close operations.
 */
extern lf_mutex_t socket_mutex;

/**
 * @brief Create an IPv4 TCP socket with Nagle's algorithm disabled
 * (TCP_NODELAY) and Delayed ACKs disabled (TCP_QUICKACK). Exits application
 * on any error.
 *
 * @return The socket ID (a file descriptor).
 */
int create_real_time_tcp_socket_errexit();

/**
 * Create a TCP or UDP server and enable listening for socket connections.
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
 * @param final_socket The socket descriptor on which to accept connections.
 * @param final_port The final port of the TCP or UDP socket.
 */
void create_TCP_server(uint16_t port, int* final_socket, uint16_t* final_port);
void create_UDP_server(uint16_t port, int* final_socket, uint16_t* final_port);

/**
 * This function waits for an incoming connection request on the specified server socket.
 * It blocks until a connection is successfully accepted. If an error occurs that is not
 * temporary (e.g., `EAGAIN` or `EWOULDBLOCK`), it reports the error and exits. Temporary
 * errors cause the function to retry accepting the connection.
 *
 * @param socket The server socket file descriptor that is listening for incoming connections.
 * @param client_fd A pointer to a `struct sockaddr` that will hold the client's address information.
 * @return int The file descriptor for the newly accepted socket on success, or -1 on failure
 *             (with an appropriate error message printed).
 */
int accept_socket(int socket, struct sockaddr* client_fd);

/**
 *
 * This function attempts to establish a TCP connection to the specified hostname
 * and port. It uses `getaddrinfo` to resolve the hostname and retries the connection
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
int read_from_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * Read the specified number of bytes to the specified socket using read_from_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

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
void read_from_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...);

/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_socket(int socket, unsigned char* result);

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
int write_to_socket(int socket, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using write_to_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer);

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
void write_to_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);

#endif /* SOCKET_COMMON_H */