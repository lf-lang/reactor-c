/**
 * @file net_driver.h
 * @brief Public network channel API (netchan) interface.
 * @ingroup Network
 *
 * @author Dongha Kim
 * @author Hokeun Kim
 *
 * This header defines the interface for network channels used by federated Lingua Franca programs.
 * It abstracts network operations behind a small API that supports server/client creation,
 * connection management, robust read/write with error handling, graceful shutdown, and
 * querying local/peer addressing information.
 */

#ifndef NET_DRIVER_H
#define NET_DRIVER_H

#include "socket_common.h"

typedef void* netchan_t;

/**
 * @brief Allocate and initialize a network channel handle.
 * @ingroup Network
 *
 * Allocate memory for the network channel.
 * @return netchan_t Initialized network channel.
 */
netchan_t initialize_netchan();

/**
 * @brief Create a server network channel that will accept incoming connections.
 * @ingroup Network
 *
 * Create a netchannel server. This is such as a server socket which accepts connections.
 * However this is only the creation of the server network channel.
 *
 * @param chan Server's network channel.
 * @param increment_port_on_retry Whether to increment the port on retry if binding fails.
 * @return int 0 for success, -1 for failure.
 */
int create_server(netchan_t chan, bool increment_port_on_retry);

/**
 * @brief Accept an incoming connection on a server network channel.
 * @ingroup Network
 *
 * Wait for an incoming connection request on the specified server network channel.
 * The implementation should include three steps.
 * 1. Initialize the network channel of the connected federate.
 * 2. Wait for the incoming connection request. This should block until the connection is successfully accepted.
 * 3. Save the information in the connected network channel, such as the address of the connected peer, for future
 * querying address.
 *
 * @param server_chan The server network channel that is listening for incoming connections.
 * @param rti_chan The rti's network channel to check if it is still open.
 * @return netchan_t The network channel for the newly accepted connection on success, or NULL on failure
 */
netchan_t accept_netchan(netchan_t server_chan, netchan_t rti_chan);

/**
 * @brief Initialize a client network channel for connecting to a server.
 * @ingroup Network
 *
 * Using the initialized network channel, create a client network channel ready to connect to a server.
 *
 * @param chan The initialized network channel.
 */
void create_client(netchan_t chan);

/**
 * @brief Connect a client network channel to its configured server.
 * @ingroup Network
 *
 * Connect to the server network channel. The server's connection information,
 * such as the port and address should be set before calling this function.
 *
 * @param chan network channel to connect.
 * @return 0 for success, -1 on failure, and `errno` is set to indicate the specific error.
 */
int connect_to_netchan(netchan_t chan);

/**
 * @brief Read a fixed number of bytes from a network channel.
 * @ingroup Network
 *
 * Read the specified number of bytes from the specified network channel into the specified buffer.
 * If an error occurs during reading, return -1 and set errno to indicate the cause.
 * If the read succeeds in reading the specified number of bytes, return 0.
 * If an EOF occurs before reading the specified number of bytes, return 1.
 *
 * @param chan The network channel.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an error.
 */
int read_from_netchan(netchan_t chan, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read bytes and close the network channel on error.
 * @ingroup Network
 *
 * Uses read_from_netchan and closes the channel if an error occurs.
 *
 * @param chan The network channel.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_netchan_close_on_error(netchan_t chan, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read bytes from a network channel and fail (exit) on error.
 * @ingroup Network
 *
 * Read the specified number of bytes from the specified network channel into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 *
 * @param chan The network channel.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 */
void read_from_netchan_fail_on_error(netchan_t chan, size_t num_bytes, unsigned char* buffer, char* format, ...);

/**
 * @brief Write a fixed number of bytes to a network channel.
 * @ingroup Network
 *
 * Write the specified number of bytes to the specified network channel using write_to_netchan
 * and close the network channel if an error occurs.
 * If an error occurs, return -1 and set errno to indicate the cause. If the write succeeds, return 0.
 * This function retries until the specified number of bytes have been written or an error occurs.
 *
 * @param chan The network channel.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netchan(netchan_t chan, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write bytes to a network channel and close on error.
 * @ingroup Network
 *
 * Uses write_to_netchan and closes the channel if an error occurs.
 *
 * @param chan The network channel.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_netchan_close_on_error(netchan_t chan, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write bytes to a network channel and fail (exit) on error.
 * @ingroup Network
 *
 * Write the specified number of bytes to the specified network channel using
 * write_to_netchan_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param chan The network channel.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_netchan_fail_on_error(netchan_t chan, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...);

/**
 * @brief Check whether the network channel is closed.
 * @ingroup Network
 *
 * Checks if the network channel is still connected to the peer.
 *
 * @param chan The network channel.
 * @return true if closed, false if still open.
 */
bool check_netchan_closed(netchan_t chan);

/**
 * @brief Gracefully shut down and close a network channel.
 * @ingroup Network
 *
 * If read_before_closing is false, call shutdown() with SHUT_RDWR and then close(). If true, call shutdown() with SHUT_WR,
 * then read() until EOF and discard received bytes before closing.
 *
 * @param chan The network channel to shut down and close.
 * @param chan The network channel to shutdown and close.
 * @param read_before_closing If true, read until EOF before closing the network channel.
 * @return int Returns 0 on success, -1 on failure (errno will indicate the error).
 */
int shutdown_netchan(netchan_t chan, bool read_before_closing);

/**
 * @brief Get the server port number of this network channel.
 * @ingroup Network
 *
 * Get the open port number from the network channel.
 * This is used when the federate sends a MSG_TYPE_ADDRESS_ADVERTISEMENT to the RTI, informing its port number. The RTI
 * will save this port number, and send it to the other federate in a MSG_TYPE_ADDRESS_QUERY_REPLY message.
 *
 * @param chan The network channel.
 * @return The port number of a server network channel.
 */
int32_t get_my_port(netchan_t chan);

/**
 * @brief Get the connected peer's port number.
 * @ingroup Network
 *
 * Get the port number of the connected peer.
 * This is used by the RTI, when there is a request from the federate to the RTI, for the MSG_TYPE_ADDRESS_QUERY
 * message.
 *
 * @param chan The network channel.
 * @return Port number of the connected peer.
 */
int32_t get_server_port(netchan_t chan);

/**
 * @brief Get the connected peer's IP address.
 * @ingroup Network
 *
 * Get the IP address of the connected peer.
 *
 * @param chan The network channel.
 * @return Pointer to the server IP address.
 */
struct in_addr* get_ip_addr(netchan_t chan);

/**
 * @brief Get the connected peer's hostname.
 * @ingroup Network
 *
 * Get the hostname of the connected peer.
 *
 * @param chan The network channel.
 * @return Pointer to the server hostname.
 */
char* get_server_hostname(netchan_t chan);

/**
 * @brief Set the user-specified port for this network channel.
 * @ingroup Network
 *
 * Set the user specified port to the created network channel.
 *
 * @param chan The network channel.
 * @param port The user specified port.
 */
void set_my_port(netchan_t chan, int32_t port);

/**
 * @brief Set the target server's port number for this network channel.
 * @ingroup Network
 *
 * Set server port number to the target network channel.
 * The federate and RTI receives the port number from another
 * federate MSG_TYPE_ADDRESS_ADVERTISEMENT message.
 * This function is used to set the network channel's target server port number.
 *
 * @param chan The network channel.
 * @param port The target server's port.
 */
void set_server_port(netchan_t chan, int32_t port);

/**
 * @brief Set the target server's port number for this network channel.
 * @ingroup Network
 *
 * Set the target server's hostname to the network channel.
 *
 * @param chan The network channel.
 * @param hostname The target server's hostname.
 */
void set_server_hostname(netchan_t chan, const char* hostname);

#endif /* NET_DRIVER_H */
