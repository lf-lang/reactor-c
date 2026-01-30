/**
 * @file
 * @author Edward A. Lee
 * @author Soroush Bateni
 * @author Peter Donovan
 * @author Dongha Kim
 *
 * @brief Common socket operations and utilities for federated Lingua Franca programs.
 */

#include <unistd.h>      // Defines read(), write(), and close()
#include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h> // TCP_NODELAY
#include <arpa/inet.h>   // inet_ntop
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h> //va_list
#include <string.h> // strerror

#include "util.h" // LF_MUTEX_UNLOCK()
#include "logging.h"
#include "net_abstraction.h"

/** Number of nanoseconds to sleep before retrying a socket read. */
#define SOCKET_READ_RETRY_INTERVAL 1000000

// Mutex lock held while performing network abstraction shutdown and close operations.
lf_mutex_t shutdown_mutex;

int create_real_time_tcp_socket_errexit(void) {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    lf_print_error_system_failure("Could not open TCP socket.");
  }
  // Disable Nagle's algorithm which bundles together small TCP messages to
  // reduce network traffic.
  // TODO: Re-consider if we should do this, and whether disabling delayed ACKs
  // is enough.
  int flag = 1;
  int result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

  if (result < 0) {
    lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
  }

#if defined(PLATFORM_Linux)
  // Disable delayed ACKs. Only possible on Linux
  result = setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(int));

  if (result < 0) {
    lf_print_error_system_failure("Failed to disable Nagle algorithm on socket server.");
  }
#endif // Linux

  return sock;
}

/**
 * Set the socket timeout options.
 * @param socket_descriptor The file descriptor of the socket on which to set options.
 * @param timeout_time A pointer to a `struct timeval` that specifies the timeout duration
 *                     for socket operations (receive and send).
 */
static void set_socket_timeout_option(int socket_descriptor, struct timeval* timeout_time) {
  // Set the option for this socket to reuse the same address
  int true_variable = 1; // setsockopt() requires a reference to the value assigned to an option
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &true_variable, sizeof(int32_t)) < 0) {
    lf_print_error("Failed to set SO_REUSEADDR option on the socket: %s.", strerror(errno));
  }
  // Set the timeout on the socket so that read and write operations don't block for too long
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)timeout_time, sizeof(*timeout_time)) < 0) {
    lf_print_error("Failed to set SO_RCVTIMEO option on the socket: %s.", strerror(errno));
  }
  if (setsockopt(socket_descriptor, SOL_SOCKET, SO_SNDTIMEO, (const char*)timeout_time, sizeof(*timeout_time)) < 0) {
    lf_print_error("Failed to set SO_SNDTIMEO option on the socket: %s.", strerror(errno));
  }
}

/**
 * Assign a port to the socket, and bind the socket.
 *
 * @param socket_descriptor The file descriptor of the socket to be bound to an address and port.
 * @param specified_port The port number to bind the socket to.
 * @return The final port number used.
 */
static int set_socket_bind_option(int socket_descriptor, uint16_t specified_port) {
  // Server file descriptor.
  struct sockaddr_in server_fd;
  // Zero out the server address structure.
  bzero((char*)&server_fd, sizeof(server_fd));
  uint16_t used_port = specified_port;
  server_fd.sin_family = AF_INET;         // IPv4
  server_fd.sin_addr.s_addr = INADDR_ANY; // All interfaces, 0.0.0.0.
  server_fd.sin_port = htons(used_port);  // Convert the port number from host byte order to network byte order.

  int result = bind(socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));

  // Try repeatedly to bind to a port.

  // Set the global server port.
  if (specified_port == 0) {
    // Need to retrieve the port number assigned by the OS.
    struct sockaddr_in assigned;
    socklen_t addr_len = sizeof(assigned);
    if (getsockname(socket_descriptor, (struct sockaddr*)&assigned, &addr_len) < 0) {
      lf_print_error_and_exit("Federate failed to retrieve assigned port number.");
    }
    used_port = ntohs(assigned.sin_port);
  }
  if (result != 0) {
    lf_print_error_and_exit("Failed to bind the socket. Port %d is not available. ", used_port);
  }
  lf_print_debug("Socket is bound to port %d.", used_port);
  return used_port;
}

int create_socket_server(uint16_t port, int* final_socket, uint16_t* final_port, socket_type_t sock_type) {
  int socket_descriptor;
  struct timeval timeout_time;
  if (sock_type == TCP) {
    // Create an IPv4 socket for TCP.
    socket_descriptor = create_real_time_tcp_socket_errexit();
    // Set the timeout time for the communications of the server
    timeout_time =
        (struct timeval){.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
  } else {
    // Create a UDP socket.
    socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    timeout_time =
        (struct timeval){.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
  }
  char* type = (sock_type == TCP) ? "TCP" : "UDP";
  if (socket_descriptor < 0) {
    lf_print_error("Failed to create %s socket.", type);
    return -1;
  }
  set_socket_timeout_option(socket_descriptor, &timeout_time);
  int used_port = set_socket_bind_option(socket_descriptor, port);
  if (sock_type == TCP) {
    // Enable listening for socket connections.
    // The second argument is the maximum number of queued socket requests,
    // which according to the Mac man page is limited to 128.
    if (listen(socket_descriptor, 128)) {
      lf_print_error("Failed to listen on %d socket: %s.", socket_descriptor, strerror(errno));
      return -1;
    }
  }
  *final_socket = socket_descriptor;
  *final_port = used_port;
  return 0;
}

bool is_socket_open(int socket) {
  if (socket < 0) {
    return false;
  }
  unsigned char first_byte;
  ssize_t bytes = peek_from_socket(socket, &first_byte);
  if (bytes < 0) {
    return false;
  }
  if (bytes == 1 && first_byte == MSG_TYPE_FAILED) {
    return false;
  }
  return true;
}

int get_peer_address(socket_priv_t* priv) {
  struct sockaddr_in peer_addr;
  socklen_t addr_len = sizeof(peer_addr);
  if (getpeername(priv->socket_descriptor, (struct sockaddr*)&peer_addr, &addr_len) != 0) {
    lf_print_error("Failed to get peer address.");
    return -1;
  }
  priv->server_ip_addr = peer_addr.sin_addr;

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  // Create the human readable format and copy that into
  // the .server_hostname field of the federate.
  char str[INET_ADDRSTRLEN + 1];
  inet_ntop(AF_INET, &priv->server_ip_addr, str, INET_ADDRSTRLEN);
  strncpy(priv->server_hostname, str, INET_ADDRSTRLEN - 1); // Copy up to INET_ADDRSTRLEN - 1 characters
  priv->server_hostname[INET_ADDRSTRLEN - 1] = '\0';        // Null-terminate explicitly

  LF_PRINT_DEBUG("Got address %s", priv->server_hostname);
#endif
  return 0;
}

int accept_socket(int socket) {
  struct sockaddr client_fd;
  // Wait for an incoming connection request.
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a federate connects.
  int socket_id = -1;
  while (true) {
    // When close(socket) is called, the accept() will return -1.
    socket_id = accept(socket, &client_fd, &client_length);
    if (socket_id >= 0) {
      // Got a socket
      break;
    } else if (socket_id < 0 && (errno != EAGAIN || errno != EWOULDBLOCK || errno != EINTR)) {
      if (errno != ECONNABORTED) {
        lf_print_warning("Failed to accept the socket. %s.", strerror(errno));
      }
      break;
    } else if (errno == EPERM) {
      lf_print_error_system_failure("Firewall permissions prohibit connection.");
      return -1;
    } else {
      // Try again
      lf_print_warning("Failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }
  return socket_id;
}

int connect_to_socket(int sock, const char* hostname, int port) {
  struct addrinfo hints;
  struct addrinfo* result;
  int ret = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       /* Allow IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_protocol = IPPROTO_TCP; /* TCP protocol */
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_flags = AI_NUMERICSERV; /* Allow only numeric port numbers */

  uint16_t used_port = (port == 0) ? DEFAULT_PORT : (uint16_t)port;

  instant_t start_connect = lf_time_physical();
  // while (!_lf_termination_executed) { // Not working...
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_error("Failed to connect with timeout: " PRINTF_TIME ". Giving up.", CONNECT_TIMEOUT);
      break;
    }
    // Convert port number to string.
    char str[6];
    snprintf(str, sizeof(str), "%u", used_port);

    // Get address structure matching hostname and hints criteria, and
    // set port to the port number provided in str. There should only
    // ever be one matching address structure, and we connect to that.
    if (getaddrinfo(hostname, (const char*)&str, &hints, &result)) {
      lf_print_error("No host matching given hostname: %s", hostname);
      break;
    }
    ret = connect(sock, result->ai_addr, result->ai_addrlen);
    if (ret < 0) {
      lf_sleep(CONNECT_RETRY_INTERVAL);
      lf_print_warning("Could not connect. Will try again every " PRINTF_TIME " nanoseconds. Connecting to port %d.\n",
                       CONNECT_RETRY_INTERVAL, used_port);
      freeaddrinfo(result);
      continue;
    } else {
      break;
    }
  }
  freeaddrinfo(result);
  lf_print("Connected to %s:%d.", hostname, used_port);
  return ret;
}

int read_from_socket(int socket, size_t num_bytes, unsigned char* buffer) {
  if (socket < 0) {
    // Socket is not open.
    errno = EBADF;
    return -1;
  }
  ssize_t bytes_read = 0;
  while (bytes_read < (ssize_t)num_bytes) {
    ssize_t more = read(socket, buffer + bytes_read, num_bytes - (size_t)bytes_read);
    if (more < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      // Those error codes set by the socket indicates
      // that we should try again (@see man errno).
      LF_PRINT_DEBUG("Reading from socket %d failed with error: `%s`. Will try again.", socket, strerror(errno));
      lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
      continue;
    } else if (more < 0) {
      // A more serious error occurred.
      lf_print_error("Reading from socket %d failed. With error: `%s`", socket, strerror(errno));
      return -1;
    } else if (more == 0) {
      // EOF received.
      return 1;
    }
    bytes_read += more;
  }
  return 0;
}

ssize_t peek_from_socket(int socket, unsigned char* result) {
  ssize_t bytes_read = recv(socket, result, 1, MSG_DONTWAIT | MSG_PEEK);
  if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
    return 0;
  else
    return bytes_read;
}

int write_to_socket(int socket, size_t num_bytes, unsigned char* buffer) {
  if (socket < 0) {
    // Socket is not open.
    errno = EBADF;
    return -1;
  }
  ssize_t bytes_written = 0;
  while (bytes_written < (ssize_t)num_bytes) {
    ssize_t more = write(socket, buffer + bytes_written, num_bytes - (size_t)bytes_written);
    if (more <= 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      // The error codes EAGAIN or EWOULDBLOCK indicate
      // that we should try again (@see man errno).
      // The error code EINTR means the system call was interrupted before completing.
      LF_PRINT_DEBUG("Writing to socket %d was blocked. Will try again.", socket);
      lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
      continue;
    } else if (more < 0) {
      // A more serious error occurred.
      lf_print_error("Writing to socket %d failed. With error: `%s`", socket, strerror(errno));
      return -1;
    }
    bytes_written += more;
  }
  return 0;
}

void init_shutdown_mutex(void) { LF_MUTEX_INIT(&shutdown_mutex); }

int shutdown_socket(int* socket, bool read_before_closing) {
  LF_MUTEX_LOCK(&shutdown_mutex);
  int result = 0;
  if (*socket < 0) {
    LF_PRINT_LOG("Socket is already closed.");
  } else {
    if (!read_before_closing) {
      if (shutdown(*socket, SHUT_RDWR)) {
        LF_PRINT_LOG("On shutdown socket, received reply: %s", strerror(errno));
        result = -1;
      } // else shutdown reads and writes succeeded.
    } else {
      // Signal the other side that no further writes are expected by sending a FIN packet.
      // This indicates the write direction is closed. For more details, refer to:
      // https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
      if (shutdown(*socket, SHUT_WR)) {
        LF_PRINT_LOG("Failed to shutdown socket: %s", strerror(errno));
        result = -1;
      } else {
        // Shutdown writes succeeded.
        // Read any remaining bytes coming in on the socket until an EOF or socket error occurs.
        // Discard any incoming bytes. Normally, this read should return 0, indicating an EOF,
        // meaning that the peer has also closed the connection.
        // This compensates for delayed ACKs and scenarios where Nagle's algorithm is disabled,
        // ensuring the shutdown completes gracefully.
        unsigned char buffer[10];
        while (read(*socket, buffer, 10) > 0)
          ;
      }
    }
    // Attempt to close the socket.
    // NOTE: In all common TCP/IP stacks, there is a time period,
    // typically between 30 and 120 seconds, called the TIME_WAIT period,
    // before the port is released after this close. This is because
    // the OS is preventing another program from accidentally receiving
    // duplicated packets intended for this program.
    if (result != 0 && close(*socket)) {
      // Close failed.
      LF_PRINT_LOG("Error while closing socket: %s\n", strerror(errno));
      result = -1;
    }
    *socket = -1;
  }
  LF_MUTEX_UNLOCK(&shutdown_mutex);
  return result;
}
