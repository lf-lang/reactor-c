#ifndef NET_DRIVER_H
#define NET_DRIVER_H

#include "socket_common.h"

typedef struct netdrv_t {
  void* priv;
} netdrv_t;

/**
 * Allocate memory for the network driver.
 * @return netdrv_t* Initialized network driver.
 */
netdrv_t* initialize_netdrv();

/**
 * Create a netdriver server. This is such as a server socket which accepts connections. However this is only the
 * creation of the server netdriver.
 *
 * @param drv Server's network driver.
 * @param serv_type Type of server, RTI or FED.
 * @return int 0 for success, -1 for failure.
 */
int create_server(netdrv_t* drv, bool increment_port_on_retry);

/**
 * Wait for an incoming connection request on the specified server network driver.
 * The implementation should include three steps.
 * 1. Initialize the network driver of the connected federate.
 * 2. Wait for the incoming connection request. This should block until the connection is successfully accepted.
 * 3. Save the information in the connected network driver, such as the address of the connected peer, for future
 * querying address.
 *
 * @param server_drv The server network driver that is listening for incoming connections.
 * @param rti_drv The rti's network driver to check if it is still open.
 * @return netdrv_t* The network driver for the newly accepted connection on success, or NULL on failure
 */
netdrv_t* accept_netdrv(netdrv_t* server_drv, netdrv_t* rti_drv);

/**
 * Using the initialized network driver, create a client network driver ready to connect to a server.
 *
 * @param drv The initialized network driver.
 */
void create_client(netdrv_t* drv);

/**
 * Connect to the server network driver. The server's connection information, such as the port and address should be set
 * before calling this function.
 *
 * @param drv Network driver to connect.
 * @return int 0 on success, -1 on failure, and `errno` is set to indicate the specific error.
 */
int connect_to_netdrv(netdrv_t* drv);

/**
 * Read the specified number of bytes from the specified socket into the specified buffer.
 * If an error occurs during this reading, return -1 and set errno to indicate
 * the cause of the error. If the read succeeds in reading the specified number of bytes,
 * return 0. If an EOF occurs before reading the specified number of bytes, return 1.
 * This function repeats the read attempt until the specified number of bytes
 * have been read, an EOF is read, or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by DELAY_BETWEEN_SOCKET_RETRIES.
 * @param drv The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an error.
 */
int read_from_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Read the specified number of bytes to the specified socket using read_from_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param socket Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Read the specified number of bytes from the specified socket into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If the mutex argument is non-NULL, release the mutex before exiting.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 * @param drv The socket ID.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 * @return The number of bytes read, or 0 if an EOF is received, or
 *  a negative number for an error.
 */
void read_from_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...);

/**
 * Write the specified number of bytes to the specified socket from the
 * specified buffer. If an error occurs, return -1 and set errno to indicate
 * the cause of the error. If the write succeeds, return 0.
 * This function repeats the attempt until the specified number of bytes
 * have been written or an error occurs. Specifically, errors EAGAIN,
 * EWOULDBLOCK, and EINTR are not considered errors and instead trigger
 * another attempt. A delay between attempts is given by
 * DELAY_BETWEEN_SOCKET_RETRIES.
 * @param drv The socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using write_to_socket
 * and close the socket if an error occurs. If an error occurs, this will change the
 * socket ID pointed to by the first argument to -1 and will return -1.
 * @param drv Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

/**
 * Write the specified number of bytes to the specified socket using
 * write_to_netdrv_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param drv Pointer to the socket ID.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result);

/**
 * @brief Gracefully shuts down and closes a socket, optionally reading until EOF.
 * Shutdown and close the socket. If read_before_closing is false, it just immediately calls shutdown() with SHUT_RDWR
 * and close(). If read_before_closing is true, it calls shutdown with SHUT_WR, only disallowing further writing. Then,
 * it calls read() until EOF is received, and discards all received bytes.
 * @param drv Pointer to the socket descriptor to shutdown and close.
 * @param read_before_closing If true, read until EOF before closing the socket.
 * @return int Returns 0 on success, -1 on failure (errno will indicate the error).
 */
int shutdown_netdrv(netdrv_t* drv, bool read_before_closing);

/**
 * Get the open port number from the network driver.
 * This is used when the federate sends a MSG_TYPE_ADDRESS_ADVERTISEMENT to the RTI, informing its port number. The RTI
 * will save this port number, and send it to the other federate in a MSG_TYPE_ADDRESS_QUERY_REPLY message.
 *
 * @param drv Network driver instance
 * @return The port number of a server network driver.
 */
int32_t get_my_port(netdrv_t* drv);

/**
 * Get the port number of the connected peer.
 * This is used by the RTI, when there is a request from the federate to the RTI, for the MSG_TYPE_ADDRESS_QUERY
 * message.
 *
 * @param drv Network driver instance
 * @return Port number of the connected peer.
 */
int32_t get_server_port(netdrv_t* drv);

/**
 * Get the IP address of the connected peer.
 *
 * @param drv Network driver instance
 * @return Pointer to the server IP address
 */
struct in_addr* get_ip_addr(netdrv_t* drv);

/**
 * Get the hostname of the connected peer.
 *
 * @param drv Network driver instance
 * @return Pointer to the server hostname
 */
char* get_server_hostname(netdrv_t* drv);

/**
 * Set the user specified port to the created network driver.
 *
 * @param drv Network driver instance
 * @param port The user specified port
 */
void set_my_port(netdrv_t* drv, int32_t port);

/**
 * Set server port number to the target network driver. The federate and RTI receives the port number fr on aom another
 * federate MSG_TYPE_ADDRESS_ADVERTISEMENT message. This function is used to set the network driver's target server port
 * number. The
 *
 * @param drv Network driver instance
 * @param port The target server's port
 */
void set_server_port(netdrv_t* drv, int32_t port);

/**
 * Set the target server's hostname to the network driver.
 *
 * @param drv Network driver instance
 * @param hostname The target server's hostname
 */
void set_server_hostname(netdrv_t* drv, const char* hostname);

#endif /* NET_DRIVER_H */
