/**
 * @file net_abstraction.h
 * @brief Public network abstraction API (net_abstraction) interface.
 * @ingroup Network
 *
 * @author Dongha Kim
 * @author Hokeun Kim
 *
 * This header defines the interface for network abstractions used by federated Lingua Franca programs.
 * It abstracts network operations behind a small API that supports server/client creation,
 * connection management, robust read/write with error handling, graceful shutdown, and
 * querying local/peer addressing information.
 */

#ifndef NET_ABSTRACTION_H
#define NET_ABSTRACTION_H

#include "socket_common.h"

/**
 * @brief Pointer to whatever data structure is used to maintain the state of a network connection or service.
 * @ingroup Network
 *
 * Pointer to whatever data structure is used to maintain the state of a network connection or service.
 * For example, this could point to an integer socket identifier if TCP is being used for network connections.
 */
typedef void* net_abstraction_t;

/**
 * @brief Pointer to a data structure containing parameters for establishing a network connection or service.
 * @ingroup Network
 *
 * This type is an void pointer used to pass implementation-specific
 * connection parameters to the network abstraction layer.
 * For example, it may point to a socket_connection_parameters_t structure
 * when using a socket-based network implementation.
 */
typedef void* net_params_t;

/**
 * @brief Allocate and initialize a network abstraction handle.
 * @ingroup Network
 *
 * Allocate memory for the network abstraction.
 * @return net_abstraction_t Initialized network abstraction.
 */
net_abstraction_t initialize_net();

/**
 * @brief Create a server network abstraction that will accept incoming connections.
 * @ingroup Network
 *
 * Create a network abstraction server. For example, this might be a server socket that accepts connections.
 *
 * @param net_abs Server's network abstraction as returned by `initialize_net`.
 * @return int 0 for success, -1 for failure.
 */
int create_server(net_abstraction_t net_abs);

/**
 * @brief Accept an incoming connection on a server network abstraction.
 * @ingroup Network
 *
 * Wait for an incoming connection request on the specified server network abstraction.
 * The implementation should include three steps.
 * 1. Wait for the incoming connection request. This should block until the connection is successfully accepted.
 * 2. Initialize a new network abstraction and link it with the accepted connection.
 * 3. Save the information in the connected network abstraction, such as the address of the connected peer, for future
 * querying address.
 *
 * @param server_chan The server network abstraction that is listening for incoming connections.
 * @return net_abstraction_t The network abstraction for the newly accepted connection on success, or NULL on failure
 */
net_abstraction_t accept_net(net_abstraction_t server_chan);

/**
 * @brief Initialize a client network abstraction for connecting to a server.
 * @ingroup Network
 *
 * Using the initialized network abstraction, create a client network abstraction ready to connect to a server.
 *
 * @param net_abs The initialized network abstraction as returned by `initialize_net`.
 */
void create_client(net_abstraction_t net_abs);

/**
 * @brief Create and connect a client network abstraction.
 * @ingroup Network
 *
 * Create a new network abstraction and establish a client-side connection to the specified server.
 *
 * @param params Pointer to implementation-specific connection parameters.
 * @return A connected network abstraction on success, or NULL on failure.
 */
net_abstraction_t connect_to_net(net_params_t* params);

/**
 * @brief Read a fixed number of bytes from a network abstraction.
 * @ingroup Network
 *
 * Read the specified number of bytes from the specified network abstraction into the specified buffer.
 * If an error occurs during reading, return -1 and set errno to indicate the cause.
 * If the read succeeds in reading the specified number of bytes, return 0.
 * If an EOF occurs before reading the specified number of bytes, return 1.
 *
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @return 0 for success, 1 for EOF, and -1 for an error.
 */
int read_from_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read bytes and close the network abstraction on error.
 * @ingroup Network
 *
 * Uses read_from_net and closes the channel if an error occurs.
 *
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int read_from_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Read bytes from a network abstraction and fail (exit) on error.
 * @ingroup Network
 *
 * Read the specified number of bytes from the specified network abstraction into the
 * specified buffer. If a disconnect or an EOF occurs during this
 * reading, then if format is non-null, report an error and exit.
 * If format is null, then report the error, but do not exit.
 * This function takes a formatted string and additional optional arguments
 * similar to printf(format, ...) that is appended to the error messages.
 *
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to read.
 * @param buffer The buffer into which to put the bytes.
 * @param format A printf-style format string, followed by arguments to
 *  fill the string, or NULL to not exit with an error message.
 */
void read_from_net_fail_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer, char* format, ...);

/**
 * @brief Write a fixed number of bytes to a network abstraction.
 * @ingroup Network
 *
 * Write the specified number of bytes to the specified network abstraction using write_to_net
 * and close the network abstraction if an error occurs.
 * If an error occurs, return -1 and set errno to indicate the cause. If the write succeeds, return 0.
 * This function retries until the specified number of bytes have been written or an error occurs.
 *
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write bytes to a network abstraction and close on error.
 * @ingroup Network
 *
 * Uses write_to_net and closes the channel if an error occurs.
 *
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @return 0 for success, -1 for failure.
 */
int write_to_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer);

/**
 * @brief Write bytes to a network abstraction and fail (exit) on error.
 * @ingroup Network
 *
 * Write the specified number of bytes to the specified network abstraction using
 * write_to_net_close_on_error and exit with an error code if an error occurs.
 * If the mutex argument is non-NULL, release the mutex before exiting.  If the
 * format argument is non-null, then use it an any additional arguments to form
 * the error message using printf conventions. Otherwise, print a generic error
 * message.
 * @param net_abs The network abstraction.
 * @param num_bytes The number of bytes to write.
 * @param buffer The buffer from which to get the bytes.
 * @param mutex If non-NULL, the mutex to unlock before exiting.
 * @param format A format string for error messages, followed by any number of
 *  fields that will be used to fill the format string as in printf, or NULL
 *  to print a generic error message.
 */
void write_to_net_fail_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                char* format, ...);

/**
 * @brief Check whether a network connection is open.
 * @ingroup Network
 *
 * Returns true if the network abstraction represents an active, open
 * connection. Returns false if the connection has not been opened,
 * has been closed, or has encountered an error.
 *
 * @param net_abs Network abstraction.
 * @return true if the connection is open, false otherwise.
 */
bool is_net_open(net_abstraction_t net_abs);

/**
 * @brief Gracefully shut down and close a network abstraction.
 * @ingroup Network
 *
 * If read_before_closing is false, call shutdown() with SHUT_RDWR and then close(). If true, call shutdown() with
 * SHUT_WR, then read() until EOF and discard received bytes before closing.
 *
 * @param net_abs The network abstraction to shut down and close.
 * @param net_abs The network abstraction to shutdown and close.
 * @param read_before_closing If true, read until EOF before closing the network abstraction.
 * @return int Returns 0 on success, -1 on failure (errno will indicate the error).
 */
int shutdown_net(net_abstraction_t net_abs, bool read_before_closing);

#endif /* NET_ABSTRACTION_H */
