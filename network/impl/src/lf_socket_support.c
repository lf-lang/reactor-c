#include <stdlib.h> // malloc
#include <string.h> // strerror
#include <errno.h>  // errno

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