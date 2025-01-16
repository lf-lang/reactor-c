#include <stdlib.h> // malloc()
#include <string.h> // strerror()
#include <errno.h>  // errno
#include <unistd.h> // read() write()

#include "net_driver.h"
#include "socket_common.h"
#include "util.h"
// #include "lf_socket_support.h"

netdrv_t* initialize_netdrv() {
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (drv == NULL) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  // Initialize priv.
  socket_priv_t* priv = malloc(sizeof(socket_priv_t));
  if (priv == NULL) {
    lf_print_error_and_exit("Falied to malloc socket_priv_t.");
  }
  // Initialize to zero.
  //   memset(priv, 0, sizeof(socket_priv_t));

  // Server initialization.
  priv->port = 0;
  priv->socket_descriptor = -1;

  // Federate initialization
  strncpy(priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  priv->server_port = -1;

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

int create_server_(netdrv_t* drv, server_type_t serv_type) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  int socket_descriptor;
  struct timeval timeout_time;
  // Create an IPv4 socket for TCP.
  socket_descriptor = create_real_time_tcp_socket_errexit();
  // Set the timeout time for the communications of the server
  timeout_time = (struct timeval){.tv_sec = TCP_TIMEOUT_TIME / BILLION, .tv_usec = (TCP_TIMEOUT_TIME % BILLION) / 1000};
  if (socket_descriptor < 0) {
    lf_print_error("Failed to create TCP socket.");
    return -1;
  }
  set_socket_timeout_option(socket_descriptor, &timeout_time);
  bool increment_port_on_retry = (serv_type == RTI) ? true : false;

  int used_port = set_socket_bind_option(socket_descriptor, priv->user_specified_port, increment_port_on_retry);
  // Enable listening for socket connections.
  // The second argument is the maximum number of queued socket requests,
  // which according to the Mac man page is limited to 128.
  if (listen(socket_descriptor, 128)) {
    lf_print_error("Failed to listen on %d socket: %s.", socket_descriptor, strerror(errno));
    return -1;
  }
  priv->socket_descriptor = socket_descriptor;
  priv->port = used_port;
  return 0;
}

netdrv_t* accept_netdrv(netdrv_t* server_drv, netdrv_t* rti_drv) {
  socket_priv_t* serv_priv = (socket_priv_t*)server_drv->priv;
  netdrv_t* fed_netdrv = initialize_netdrv();
  socket_priv_t* fed_priv = (socket_priv_t*)fed_netdrv->priv;

  struct sockaddr client_fd;
  // Wait for an incoming connection request.
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a federate connects.
  int socket_id = -1;
  while (true) {
    // When close(socket) is called, the accept() will return -1.
    socket_id = accept(serv_priv->socket_descriptor, &client_fd, &client_length);
    if (socket_id >= 0) {
      // Got a socket
      break;
    } else if (socket_id < 0 && (errno != EAGAIN || errno != EWOULDBLOCK || errno != EINTR)) {
      lf_print_warning("Failed to accept the socket. %s.", strerror(errno));
      break;
    } else if (errno == EPERM) {
      lf_print_error_system_failure("Firewall permissions prohibit connection.");
    } else {
      // For the federates, it should check if the rti_socket is still open, before retrying accept().
      socket_priv_t* rti_priv = (socket_priv_t*)rti_drv->priv;
      if (rti_priv->socket_descriptor != -1) {
        if (check_socket_closed(rti_priv->socket_descriptor)) {
          break;
        }
      }
      // Try again
      lf_print_warning("Failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }
  fed_priv->socket_descriptor = socket_id;
  return fed_netdrv;
}

int read_from_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  int socket = priv->socket_descriptor;
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

int read_from_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  int read_failed = read_from_netdrv(drv, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_netdrv(drv, false);
    return -1;
  }
  return 0;
}

void read_from_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                    char* format, ...) {
  va_list args;
  int read_failed = read_from_netdrv_close_on_error(drv, num_bytes, buffer);
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

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  int socket = priv->socket_descriptor;
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

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  int result = write_to_netdrv(drv, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_netdrv(drv, false);
  }
  return result;
}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {
  va_list args;
  int result = write_to_netdrv_close_on_error(drv, num_bytes, buffer);
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

int shutdown_netdrv(netdrv_t* drv, bool read_before_closing) {
  socket_priv_t* priv = (socket_priv_t*)drv->priv;
  if (!read_before_closing) {
    if (shutdown(priv->socket_descriptor, SHUT_RDWR)) {
      lf_print_log("On shutdown socket, received reply: %s", strerror(errno));
      return -1;
    }
  } else {
    // Signal the other side that no further writes are expected by sending a FIN packet.
    // This indicates the write direction is closed. For more details, refer to:
    // https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
    if (shutdown(priv->socket_descriptor, SHUT_WR)) {
      lf_print_log("Failed to shutdown socket: %s", strerror(errno));
      return -1;
    }

    // Wait for the other side to send an EOF or encounter a socket error.
    // Discard any incoming bytes. Normally, this read should return 0, indicating the peer has also closed the
    // connection.
    // This compensates for delayed ACKs and scenarios where Nagle's algorithm is disabled, ensuring the shutdown
    // completes gracefully.
    unsigned char buffer[10];
    while (read(priv->socket_descriptor, buffer, 10) > 0)
      ;
  }
  // NOTE: In all common TCP/IP stacks, there is a time period,
  // typically between 30 and 120 seconds, called the TIME_WAIT period,
  // before the port is released after this close. This is because
  // the OS is preventing another program from accidentally receiving
  // duplicated packets intended for this program.
  if (close(priv->socket_descriptor)) {
    lf_print_log("Error while closing socket: %s\n", strerror(errno));
    return -1;
  }
  priv->socket_descriptor = -1;
  return 0;
}
