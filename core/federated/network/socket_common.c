#include <arpa/inet.h> /* htons */
#include <errno.h>
#include <linux/if.h> /* IFNAMSIZ */
#include <netinet/ether.h>
#include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h> // TCP_NODELAY
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
// #include "socket_common.h"

socket_priv_t* socket_priv_init() {
  socket_priv_t* priv = malloc(sizeof(socket_priv_t));
  if (!priv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(priv, 0, sizeof(socket_priv_t));

  // TODO: Remove when it works.
  // priv->port = 0;
  // priv->socket_descriptor = 0;
  // priv->user_specified_port = 0;
  // priv->server_ip_addr.s_addr = 0;

  // federate initialization
  strncpy(priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  priv->server_port = -1;
}

/**
 * @brief Create an IPv4 TCP socket with Nagle's algorithm disabled
 * (TCP_NODELAY) and Delayed ACKs disabled (TCP_QUICKACK). Exits application
 * on any error.
 *
 * @return The socket ID (a file descriptor).
 */
int create_real_time_tcp_socket_errexit() {
  // Timeout time for the communications of the server
  struct timeval timeout_time = {.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    lf_print_error_system_failure("Could not open TCP socket.");
  }
  // Disable Nagle's algorithm which bundles together small TCP messages to
  // reduce network traffic.
  // TODO: Re-consider if we should do this, and whether disabling delayed ACKs
  // is enough.
  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0) {
    lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int32_t)) < 0) {
    lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
  }
  // Set the timeout on the socket so that read and write operations don't block for too long
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
  }
  if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("RTI failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
  }

#if defined(PLATFORM_Linux)
  // Disable delayed ACKs. Only possible on Linux
  if (setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(int)) < 0) {
    lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
  }
#endif // Linux

  return sock;
}

// This only creates TCP servers not UDP.
int create_server(netdrv_t* drv, server_type_t server_type, uint16_t port) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  // Federate always has a specified port. The RTI can get a specified port by user input.
  uint16_t specified_port = port;
  if (specified_port == 0 && server_type == RTI) {
    port = DEFAULT_PORT;
  }

  // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
  priv->socket_descriptor = create_real_time_tcp_socket_errexit();

  // Server file descriptor.
  struct sockaddr_in server_fd;
  // Zero out the server address structure.
  bzero((char*)&server_fd, sizeof(server_fd));

  server_fd.sin_family = AF_INET;         // IPv4
  server_fd.sin_addr.s_addr = INADDR_ANY; // All interfaces, 0.0.0.0.
  // Convert the port number from host byte order to network byte order.
  server_fd.sin_port = htons(port);

  int result = bind(priv->socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));
  // Try repeatedly to bind to a port.
  int count = 1;

  while (result != 0 && count++ < PORT_BIND_RETRY_LIMIT) {
    if (specified_port == 0) {
      lf_print_warning("Failed to get port %d.", port);
      port++;
      if (port >= DEFAULT_PORT + MAX_NUM_PORT_ADDRESSES)
        port = DEFAULT_PORT;
      lf_print_warning("Try again with port %d.", port);
      server_fd.sin_port = htons(port);
      // Do not sleep.
    } else {
      lf_print("RTI failed to get port %d. Will try again.", port);
      lf_sleep(PORT_BIND_RETRY_INTERVAL);
    }
    result = bind(priv->socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));
  }
  if (result != 0) {
    lf_print_error_and_exit("Failed to bind the socket. Port %d is not available. ", port);
  }
  // Enable listening for socket connections.
  // The second argument is the maximum number of queued socket requests,
  // which according to the Mac man page is limited to 128.
  listen(priv->socket_descriptor, 128);

  // Set the port into priv->port.
  if (specified_port == 0 && server_type == FED) {
    // Need to retrieve the port number assigned by the OS.
    struct sockaddr_in assigned;
    socklen_t addr_len = sizeof(assigned);
    if (getsockname(priv->socket_descriptor, (struct sockaddr*)&assigned, &addr_len) < 0) {
      lf_print_error_and_exit("Failed to retrieve assigned port number.");
    }
    priv->port = ntohs(assigned.sin_port);
  } else {
    priv->port = port;
  }
  return 1;
}

// Returns clock sync UDP socket.
int create_clock_sync_server(uint16_t* clock_sync_port) {
  // Create UDP socket.
  int socket_descriptor = -1;
  socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  // Set the appropriate timeout time for the communications of the server
  struct timeval timeout_time =
      (struct timeval){.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
  if (socket_descriptor < 0) {
    lf_print_error_system_failure("Failed to create RTI socket.");
  }

  // Set the option for this socket to reuse the same address
  int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &true_variable, sizeof(int32_t)) < 0) {
    lf_print_error("RTI failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
  }
  // Set the timeout on the socket so that read and write operations don't block for too long
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("RTI failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
  }
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout_time, sizeof(timeout_time)) < 0) {
    lf_print_error("RTI failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
  }

  // Server file descriptor.
  struct sockaddr_in server_fd;
  // Zero out the server address structure.
  bzero((char*)&server_fd, sizeof(server_fd));

  uint16_t port = RTI_DEFAULT_UDP_PORT;   // Default UDP port.
  server_fd.sin_family = AF_INET;         // IPv4
  server_fd.sin_addr.s_addr = INADDR_ANY; // All interfaces, 0.0.0.0.
  // Convert the port number from host byte order to network byte order.
  server_fd.sin_port = htons(port);

  int result = bind(socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));

  // Try repeatedly to bind to a port. If no specific port is specified, then
  // increment the port number each time.

  int count = 1;
  while (result != 0 && count++ < PORT_BIND_RETRY_LIMIT) {
    lf_print_warning("RTI failed to get port %d.", port);
    port++;
    if (port >= RTI_DEFAULT_UDP_PORT + MAX_NUM_PORT_ADDRESSES)
      port = RTI_DEFAULT_UDP_PORT;
    lf_print_warning("RTI will try again with port %d.", port);
    server_fd.sin_port = htons(port);
    result = bind(socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));
  }
  if (result != 0) {
    lf_print_error_and_exit("Failed to bind the RTI socket. Port %d is not available. ", port);
  }

  // Update port number.
  *clock_sync_port = port;
  // No need to listen on the UDP socket

  return socket_descriptor;
}
