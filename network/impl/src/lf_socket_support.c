#include <stdlib.h>    // malloc()
#include <string.h>    // strerror()
#include <errno.h>     // errno
#include <unistd.h>    // read() write()
#include <arpa/inet.h> // inet_ntop

#include "net_driver.h"
#include "socket_common.h"
#include "util.h"

static socket_priv_t* get_socket_priv_t(netdrv_t* drv) { return (socket_priv_t*)drv->priv; }

static int get_peer_address(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
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

  // Server initialization.
  priv->port = 0;
  priv->user_specified_port = 0;
  priv->socket_descriptor = -1;

  // Federate initialization
  strncpy(priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  priv->server_ip_addr.s_addr = 0;
  priv->server_port = -1;

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

void free_netdrv(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  free(priv);
  free(drv);
}

int create_server(netdrv_t* drv, bool increment_port_on_retry) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return create_socket_server(priv->user_specified_port, &priv->socket_descriptor, &priv->port, TCP,
                              increment_port_on_retry);
}

netdrv_t* accept_netdrv(netdrv_t* server_drv, netdrv_t* rti_drv) {
  socket_priv_t* serv_priv = get_socket_priv_t(server_drv);
  int rti_socket;
  if (rti_drv == NULL) {
    rti_socket = -1;
  } else {
    socket_priv_t* rti_priv = get_socket_priv_t(rti_drv);
    rti_socket = rti_priv->socket_descriptor;
  }
  netdrv_t* fed_netdrv = initialize_netdrv();
  socket_priv_t* fed_priv = get_socket_priv_t(fed_netdrv);

  int sock = accept_socket(serv_priv->socket_descriptor, rti_socket);
  if (sock == -1) {
    free_netdrv(fed_netdrv);
    return NULL;
  }
  fed_priv->socket_descriptor = sock;
  // Get the peer address from the connected socket_id.
  if (get_peer_address(fed_netdrv) != 0) {
    lf_print_error("RTI failed to get peer address.");
  };
  return fed_netdrv;
}

void create_client(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  priv->socket_descriptor = create_real_time_tcp_socket_errexit();
}

int connect_to_netdrv(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return connect_to_socket(priv->socket_descriptor, priv->server_hostname, priv->server_port);
}

int read_from_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return read_from_socket(priv->socket_descriptor, num_bytes, buffer);
}

int read_from_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  int read_failed = read_from_netdrv(drv, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_descriptor, false);
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
  socket_priv_t* priv = get_socket_priv_t(drv);
  return write_to_socket(priv->socket_descriptor, num_bytes, buffer);
}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  int result = write_to_netdrv(drv, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_descriptor, false);
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

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return peek_from_socket(priv->socket_descriptor, result);
}

int shutdown_netdrv(netdrv_t* drv, bool read_before_closing) {
  if (drv == NULL) {
    lf_print("Socket already closed.");
    return 0;
  }
  socket_priv_t* priv = get_socket_priv_t(drv);
  int ret = shutdown_socket(&priv->socket_descriptor, read_before_closing);
  if (ret != 0) {
    lf_print_error("Failed to shutdown socket.");
  }
  free_netdrv(drv);
  return ret;
}

int32_t get_my_port(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return priv->port;
}

int32_t get_server_port(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return priv->server_port;
}

struct in_addr* get_ip_addr(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return &priv->server_ip_addr;
}

char* get_server_hostname(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return priv->server_hostname;
}

int get_socket_id(netdrv_t* drv) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  return priv->socket_descriptor;
}

void set_server_port(netdrv_t* drv, int32_t port) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  priv->server_port = port;
}

void set_server_host_name(netdrv_t* drv, const char* hostname) {
  socket_priv_t* priv = get_socket_priv_t(drv);
  memcpy(priv->server_hostname, hostname, INET_ADDRSTRLEN);
}
