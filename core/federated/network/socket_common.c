// #include <arpa/inet.h> /* htons */
// #include <errno.h>
// #include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
// #include <netinet/tcp.h> // TCP_NODELAY
// #include <netdb.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>

// #include "util.h"
// #include "net_common.h"
// #include "net_util.h"
// #include "socket_common.h"

#include <unistd.h>      // Defines read(), write(), and close()
#include <netinet/in.h>  // IPPROTO_TCP, IPPROTO_UDP
#include <netinet/tcp.h> // TCP_NODELAY
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h> //va_list
#include <string.h> // strerror

#include "util.h"
#include "socket_common.h"

#ifndef NUMBER_OF_FEDERATES
#define NUMBER_OF_FEDERATES 1
#endif

/** Number of nanoseconds to sleep before retrying a socket read. */
#define SOCKET_READ_RETRY_INTERVAL 1000000

// Mutex lock held while performing socket close operations.
// A deadlock can occur if two threads simulataneously attempt to close the same socket.
lf_mutex_t socket_mutex;

int create_real_time_tcp_socket_errexit() {
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

static void set_socket_timeout_option(int socket_descriptor, socket_type_t socket_type) {
  // Timeout time for the communications of the server
  struct timeval timeout_time;
  if (socket_type == TCP) {
    timeout_time =
        (struct timeval){.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
  } else if (socket_type == UDP) {
    // Set the appropriate timeout time
    timeout_time =
        (struct timeval){.tv_sec = UDP_TIMEOUT_TIME / BILLION, .tv_usec = (UDP_TIMEOUT_TIME % BILLION) / 1000};
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
}

static int set_socket_bind_option(int socket_descriptor, int port) {
  // Server file descriptor.
  struct sockaddr_in server_fd;
  // Zero out the server address structure.
  bzero((char*)&server_fd, sizeof(server_fd));

  uint16_t specified_port = port;
  if (specified_port == 0) {
    port = DEFAULT_PORT;
  }
  server_fd.sin_family = AF_INET;         // IPv4
  server_fd.sin_addr.s_addr = INADDR_ANY; // All interfaces, 0.0.0.0.
  // Convert the port number from host byte order to network byte order.
  server_fd.sin_port = htons(port);

  int result = bind(socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));

  // Try repeatedly to bind to a port. If no specific port is specified, then
  // increment the port number each time.

  int count = 1;
  while (result != 0 && count++ < PORT_BIND_RETRY_LIMIT) {
    if (specified_port == 0) {
      lf_print_warning("RTI failed to get port %d.", port);
      port++;
      if (port >= DEFAULT_PORT + MAX_NUM_PORT_ADDRESSES)
        port = DEFAULT_PORT;
      lf_print_warning("RTI will try again with port %d.", port);
      server_fd.sin_port = htons(port);
      // Do not sleep.
    } else {
      lf_print("RTI failed to get port %d. Will try again.", port);
      lf_sleep(PORT_BIND_RETRY_INTERVAL);
    }
    result = bind(socket_descriptor, (struct sockaddr*)&server_fd, sizeof(server_fd));
  }
  if (result != 0) {
    lf_print_error_and_exit("Failed to bind the RTI socket. Port %d is not available. ", port);
  }
}

int create_rti_server(uint16_t port, socket_type_t socket_type, int* final_socket, uint16_t* final_port) {

  // Create an IPv4 socket for TCP (not UDP) communication over IP (0).
  int socket_descriptor = -1;
  if (socket_type == TCP) {
    socket_descriptor = create_real_time_tcp_socket_errexit();
  } else if (socket_type == UDP) {
    socket_descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  }
  if (socket_descriptor < 0) {
    lf_print_error_system_failure("Failed to create RTI socket.");
  }
  set_socket_timeout_option(socket_descriptor, socket_type);
  int out_port = set_socket_bind_option(socket_descriptor, port);

  char* type = (socket_type == TCP) ? "TCP" : "UDP";
  lf_print("RTI using %s port %d.", type, port);

  *final_socket = socket_descriptor;
  *final_port = out_port;

  if (socket_type == TCP) {
    // Enable listening for socket connections.
    // The second argument is the maximum number of queued socket requests,
    // which according to the Mac man page is limited to 128.
    listen(socket_descriptor, 128);
  } else if (socket_type == UDP) {
    // No need to listen on the UDP socket
  }
}

int accept_socket(int socket, struct sockaddr* client_fd) {
  // Wait for an incoming connection request.
  uint32_t client_length = sizeof(*client_fd);
  // The following blocks until a federate connects.
  int socket_id = -1;
  while (1) {
    socket_id = accept(socket, client_fd, &client_length);
    if (socket_id >= 0) {
      // Got a socket
      break;
    } else if (socket_id < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
      lf_print_error_system_failure("RTI failed to accept the socket.");
    } else {
      // Try again
      lf_print_warning("RTI failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }
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

int read_from_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer) {
  assert(socket);
  int read_failed = read_from_socket(*socket, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown(*socket, SHUT_RDWR);
    close(*socket);
    // Mark the socket closed.
    *socket = -1;
    return -1;
  }
  return 0;
}

void read_from_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...) {
  va_list args;
  assert(socket);
  int read_failed = read_from_socket_close_on_error(socket, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    if (mutex != NULL) {
      LF_MUTEX_UNLOCK(mutex);
    }
    if (format != NULL) {
      va_start(args, format);
      lf_print_error_system_failure(format, args);
      va_end(args);
    } else {
      lf_print_error_system_failure("Failed to read from socket.");
    }
  }
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

int write_to_socket_close_on_error(int* socket, size_t num_bytes, unsigned char* buffer) {
  assert(socket);
  int result = write_to_socket(*socket, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown(*socket, SHUT_RDWR);
    close(*socket);
    // Mark the socket closed.
    *socket = -1;
  }
  return result;
}

void write_to_socket_fail_on_error(int* socket, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {
  va_list args;
  assert(socket);
  int result = write_to_socket_close_on_error(socket, num_bytes, buffer);
  if (result) {
    // Write failed.
    if (mutex != NULL) {
      LF_MUTEX_UNLOCK(mutex);
    }
    if (format != NULL) {
      va_start(args, format);
      lf_print_error_system_failure(format, args);
      va_end(args);
    } else {
      lf_print_error("Failed to write to socket. Closing it.");
    }
  }
}
