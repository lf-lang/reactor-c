/**
 * @file
 * @author Edward A. Lee
 * @author Dongha Kim
 *
 * @brief Implementation of socket interface for federated Lingua Franca programs.
 */

#include <stdlib.h>    // malloc()
#include <string.h>    // strerror()
#include <errno.h>     // errno
#include <unistd.h>    // read() write()
#include <arpa/inet.h> // inet_ntop

#include "net_abstraction.h"
#include "socket_common.h"
#include "util.h" // LF_MUTEX_UNLOCK
#include "logging.h"

net_abstraction_t initialize_net() {
  // Initialize priv.
  socket_priv_t* priv = malloc(sizeof(socket_priv_t));
  if (priv == NULL) {
    lf_print_error_and_exit("Falied to malloc socket_priv_t.");
  }

  // Server initialization.
  priv->port = 0;
  priv->user_specified_port = 0;
  priv->socket_descriptor = -1;

  // Federate initialization
  strncpy(priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  priv->server_ip_addr.s_addr = 0;
  priv->server_port = -1;

  return (net_abstraction_t)priv;
}

void free_net(net_abstraction_t net_abs) {
  if (net_abs == NULL) {
    LF_PRINT_LOG("Socket already closed.");
    return;
  }
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  free(priv);
}

int create_server(net_abstraction_t net_abs, bool increment_port_on_retry) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return create_socket_server(priv->user_specified_port, &priv->socket_descriptor, &priv->port, TCP,
                              increment_port_on_retry);
}

net_abstraction_t accept_net(net_abstraction_t server_chan) {
  LF_ASSERT_NON_NULL(server_chan);
  socket_priv_t* serv_priv = (socket_priv_t*)server_chan;

  int sock = accept_socket(serv_priv->socket_descriptor);
  if (sock != -1) {
    net_abstraction_t client_net = initialize_net();
    socket_priv_t* client_priv = (socket_priv_t*)client_net;
    client_priv->socket_descriptor = sock;
    // Get the peer address from the connected socket_id. Saving this for the address query.
    if (get_peer_address(client_priv) != 0) {
      lf_print_error("Failed to save peer address.");
    }
    return client_net;
  } else {
    return NULL;
  }
}

void create_client(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  priv->socket_descriptor = create_real_time_tcp_socket_errexit();
}

net_abstraction_t connect_to_net(net_params_t* params) {
  // Create a network abstraction.
  net_abstraction_t net = initialize_net();
  socket_priv_t* priv = (socket_priv_t*)net;
  socket_connection_parameters_t* sock_params = (socket_connection_parameters_t*)params;
  priv->server_port = sock_params->port;
  memcpy(priv->server_hostname, sock_params->server_hostname, INET_ADDRSTRLEN);
  // Create the client network abstraction.
  create_client(net);
  // Connect to the target server.
  if (connect_to_socket(priv->socket_descriptor, priv->server_hostname, priv->server_port) != 0) {
    lf_print_error("Failed to connect to socket.");
    return NULL;
  }
  return net;
}

int read_from_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return read_from_socket(priv->socket_descriptor, num_bytes, buffer);
}

int read_from_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  int read_failed = read_from_net(net_abs, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_descriptor, false);
    return -1;
  }
  return 0;
}

void read_from_net_fail_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer, char* format,
                                 ...) {
  va_list args;
  int read_failed = read_from_net_close_on_error(net_abs, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    if (format != NULL) {
      va_start(args, format);
      lf_print_error_system_failure(format, args);
      va_end(args);
    } else {
      lf_print_error_system_failure("Failed to read from socket.");
    }
  }
}

int write_to_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return write_to_socket(priv->socket_descriptor, num_bytes, buffer);
}

int write_to_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  int result = write_to_net(net_abs, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_descriptor, false);
  }
  return result;
}

void write_to_net_fail_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                char* format, ...) {
  va_list args;
  int result = write_to_net_close_on_error(net_abs, num_bytes, buffer);
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
      lf_print_error_and_exit("Failed to write to socket. Shutting down.");
    }
  }
}

bool check_net_closed(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return check_socket_closed(priv->socket_descriptor);
}

int shutdown_net(net_abstraction_t net_abs, bool read_before_closing) {
  if (net_abs == NULL) {
    LF_PRINT_LOG("Socket already closed.");
    return 0;
  }
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  int ret = shutdown_socket(&priv->socket_descriptor, read_before_closing);
  free_net(net_abs);
  return ret;
}

int32_t get_my_port(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return priv->port;
}

int32_t get_server_port(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return priv->server_port;
}

struct in_addr* get_ip_addr(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return &priv->server_ip_addr;
}

char* get_server_hostname(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return priv->server_hostname;
}

void set_my_port(net_abstraction_t net_abs, int32_t port) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  priv->port = port;
}

void set_server_port(net_abstraction_t net_abs, int32_t port) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  priv->server_port = port;
}

void set_server_hostname(net_abstraction_t net_abs, const char* hostname) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  memcpy(priv->server_hostname, hostname, INET_ADDRSTRLEN);
}
