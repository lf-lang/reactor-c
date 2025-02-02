#include <stdlib.h>    // malloc()
#include <string.h>    // strncpy()

#include "net_driver.h"
#include "lf_sst_support.h"
#include "util.h"

const char* sst_config_path; // The SST's configuration file path.

static sst_priv_t* get_sst_priv_t(netdrv_t drv) {
  if (drv == NULL) {
    lf_print_error("Network driver is already closed.");
    return NULL;
  }
  return (sst_priv_t*)drv;
}

netdrv_t initialize_netdrv() {
  // Initialize sst_priv.
  sst_priv_t* sst_priv = malloc(sizeof(sst_priv_t));
  if (sst_priv == NULL) {
    lf_print_error_and_exit("Falied to malloc sst_priv_t.");
  }
  // Initialize socket_priv.
  socket_priv_t* socket_priv = malloc(sizeof(socket_priv_t));
  if (socket_priv == NULL) {
    lf_print_error_and_exit("Falied to malloc socket_priv_t.");
  }

  // Server initialization.
  socket_priv->port = 0;
  socket_priv->user_specified_port = 0;
  socket_priv->socket_descriptor = -1;

  // Federate initialization
  strncpy(socket_priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  socket_priv->server_ip_addr.s_addr = 0;
  socket_priv->server_port = -1;

  sst_priv->socket_priv = socket_priv;

  // SST initialization. Only set pointers to NULL.
  sst_priv->sst_ctx = NULL;
  sst_priv->session_ctx = NULL;

  return (netdrv_t)sst_priv;
}

void free_netdrv(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  free(priv->socket_priv);
  free(priv);
}

int create_server(netdrv_t drv, bool increment_port_on_retry) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  SST_ctx_t* ctx = init_SST(sst_config_path);
  priv->sst_ctx = ctx;
  return create_socket_server(priv->socket_priv->user_specified_port, &priv->socket_priv->socket_descriptor,
                              &priv->socket_priv->port, TCP, increment_port_on_retry);
}

netdrv_t accept_netdrv(netdrv_t server_drv, netdrv_t rti_drv) {
  sst_priv_t* serv_priv = get_sst_priv_t(server_drv);
  int rti_socket;
  if (rti_drv == NULL) {
    // Set to -1, to indicate that this accept_netdrv() call is not trying to check if the rti_drv is available, inside
    // the accept_socket() function.
    rti_socket = -1;
  } else {
    sst_priv_t* rti_priv = get_sst_priv_t(rti_drv);
    rti_socket = rti_priv->socket_priv->socket_descriptor;
  }
  netdrv_t fed_netdrv = initialize_netdrv();
  sst_priv_t* fed_priv = get_sst_priv_t(fed_netdrv);

  int sock = accept_socket(serv_priv->socket_priv->socket_descriptor, rti_socket);
  if (sock == -1) {
    free_netdrv(fed_netdrv);
    return NULL;
  }
  fed_priv->socket_priv->socket_descriptor = sock;
  // Get the peer address from the connected socket_id. Saving this for the address query.
  if (get_peer_address(fed_priv->socket_priv) != 0) {
    lf_print_error("RTI failed to get peer address.");
  };

  // TODO: Do we need to copy sst_ctx form server_drv to fed_drv?
  session_key_list_t* s_key_list = init_empty_session_key_list();
  SST_session_ctx_t* session_ctx =
      server_secure_comm_setup(serv_priv->sst_ctx, fed_priv->socket_priv->socket_descriptor, s_key_list);
  // Session key used is copied to the session_ctx.
  free_session_key_list_t(s_key_list);
  fed_priv->session_ctx = session_ctx;
  return fed_netdrv;
}

void create_client(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  priv->socket_priv->socket_descriptor = create_real_time_tcp_socket_errexit();
  SST_ctx_t* ctx = init_SST(sst_config_path);
  priv->sst_ctx = ctx;
}

int connect_to_netdrv(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  int ret = connect_to_socket(priv->socket_priv->socket_descriptor, priv->socket_priv->server_hostname,
                              priv->socket_priv->server_port);
  if (ret != 0) {
    return ret;
  }
  session_key_list_t* s_key_list = get_session_key(priv->sst_ctx, NULL);
  SST_session_ctx_t* session_ctx =
      secure_connect_to_server_with_socket(&s_key_list->s_key[0], priv->socket_priv->socket_descriptor);
  priv->session_ctx = session_ctx;
  return 0;
}

// TODO:
int read_from_netdrv(netdrv_t drv, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return read_from_socket(priv->socket_priv->socket_descriptor, num_bytes, buffer);
}

int read_from_netdrv_close_on_error(netdrv_t drv, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  int read_failed = read_from_netdrv(drv, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_priv->socket_descriptor, false);
    return -1;
  }
  return 0;
}

void read_from_netdrv_fail_on_error(netdrv_t drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
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

int write_to_netdrv(netdrv_t drv, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return write_to_socket(priv->socket_priv->socket_descriptor, num_bytes, buffer);
}

int write_to_netdrv_close_on_error(netdrv_t drv, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  int result = write_to_netdrv(drv, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_priv->socket_descriptor, false);
  }
  return result;
}

void write_to_netdrv_fail_on_error(netdrv_t drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
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

bool check_netdrv_closed(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return check_socket_closed(priv->socket_priv->socket_descriptor);
}

int shutdown_netdrv(netdrv_t drv, bool read_before_closing) {
  if (drv == NULL) {
    lf_print("Socket already closed.");
    return 0;
  }
  sst_priv_t* priv = get_sst_priv_t(drv);
  int ret = shutdown_socket(&priv->socket_priv->socket_descriptor, read_before_closing);
  if (ret != 0) {
    lf_print_error("Failed to shutdown socket.");
  }
  free_netdrv(drv);
  return ret;
}
// END of TODO:

// Get/set functions.
int32_t get_my_port(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return priv->socket_priv->port;
}

int32_t get_server_port(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return priv->socket_priv->server_port;
}

struct in_addr* get_ip_addr(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return &priv->socket_priv->server_ip_addr;
}

char* get_server_hostname(netdrv_t drv) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  return priv->socket_priv->server_hostname;
}

void set_my_port(netdrv_t drv, int32_t port) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  priv->socket_priv->port = port;
}

void set_server_port(netdrv_t drv, int32_t port) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  priv->socket_priv->server_port = port;
}

void set_server_hostname(netdrv_t drv, const char* hostname) {
  sst_priv_t* priv = get_sst_priv_t(drv);
  memcpy(priv->socket_priv->server_hostname, hostname, INET_ADDRSTRLEN);
}

// Helper function.
void lf_set_sst_config_path(const char* config_path) { sst_config_path = config_path; }
