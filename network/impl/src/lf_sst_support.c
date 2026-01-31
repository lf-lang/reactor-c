#include <stdlib.h> // malloc()
#include <string.h> // strncpy()

#include "net_abstraction.h"
#include "lf_sst_support.h"
#include "util.h"

const char* sst_config_path; // The SST's configuration file path.

net_abstraction_t initialize_net() {
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

  return (net_abstraction_t)sst_priv;
}

void free_net(net_abstraction_t net_abs) {
  if (net_abs == NULL) {
    LF_PRINT_LOG("Socket already closed.");
    return;
  }
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  free(priv->socket_priv);
  free(priv);
}

int create_server(net_abstraction_t net_abs) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  SST_ctx_t* ctx = init_SST(sst_config_path);
  priv->sst_ctx = ctx;
  return create_socket_server(priv->socket_priv->user_specified_port, &priv->socket_priv->socket_descriptor,
                              &priv->socket_priv->port, TCP);
}

// TODO: check new implementation.
net_abstraction_t accept_net(net_abstraction_t server_chan) {

  LF_ASSERT_NON_NULL(server_chan);
  sst_priv_t* serv_priv = (sst_priv_t*)server_chan;

  int sock = accept_socket(serv_priv->socket_priv->socket_descriptor);
  if (sock != -1) {
    net_abstraction_t client_net = initialize_net();
    sst_priv_t* client_priv = (sst_priv_t*)client_net;
    client_priv->socket_priv->socket_descriptor = sock;
    // Get the peer address from the connected socket_id. Saving this for the address query.
    if (get_peer_address(client_priv->socket_priv) != 0) {
      lf_print_error("Failed to save peer address.");
    }

    // TODO: Do we need to copy sst_ctx form server_chan to fed_chan?
    session_key_list_t* s_key_list = init_empty_session_key_list();
    SST_session_ctx_t* session_ctx =
        server_secure_comm_setup(serv_priv->sst_ctx, client_priv->socket_priv->socket_descriptor, s_key_list);
    // Session key used is copied to the session_ctx.
    free_session_key_list_t(s_key_list);
    client_priv->session_ctx = session_ctx;

    return client_net;
  } else {
    return NULL;
  }
}

void create_client(net_abstraction_t net_abs) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  priv->socket_priv->socket_descriptor = create_real_time_tcp_socket_errexit();
  SST_ctx_t* ctx = init_SST(sst_config_path);
  priv->sst_ctx = ctx;
}

net_abstraction_t connect_to_net(net_params_t* params) {
  // Create a network abstraction.
  net_abstraction_t net = initialize_net();
  sst_priv_t* priv = (sst_priv_t*)net;
  socket_connection_parameters_t* sock_params = (socket_connection_parameters_t*)params;
  priv->socket_priv->server_port = sock_params->port;
  memcpy(priv->socket_priv->server_hostname, sock_params->server_hostname, INET_ADDRSTRLEN);
  // Create the client network abstraction.
  create_client(net);
  // Connect to the target server.
  if (connect_to_socket(priv->socket_priv->socket_descriptor, priv->socket_priv->server_hostname, priv->socket_priv->server_port) != 0) {
    lf_print_error("Failed to connect to socket.");
    return NULL;
  }
  session_key_list_t* s_key_list = get_session_key(priv->sst_ctx, NULL);
  SST_session_ctx_t* session_ctx =
      secure_connect_to_server_with_socket(&s_key_list->s_key[0], priv->socket_priv->socket_descriptor);
  priv->session_ctx = session_ctx;
  return net;
}

// TODO: Still need to fix...
int read_from_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return read_from_socket(priv->socket_priv->socket_descriptor, num_bytes, buffer);
}

int read_from_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  int read_failed = read_from_net(net_abs, num_bytes, buffer);
  if (read_failed) {
    // Read failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_priv->socket_descriptor, false);
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
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return write_to_socket(priv->socket_priv->socket_descriptor, num_bytes, buffer);
}

int write_to_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  int result = write_to_net(net_abs, num_bytes, buffer);
  if (result) {
    // Write failed.
    // Socket has probably been closed from the other side.
    // Shut down and close the socket from this side.
    shutdown_socket(&priv->socket_priv->socket_descriptor, false);
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
      lf_print_error("Failed to write to socket. Closing it.");
    }
  }
}

bool is_net_open(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  socket_priv_t* priv = (socket_priv_t*)net_abs;
  return is_socket_open(priv->socket_descriptor);
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
// END of TODO:

// Helper function.
void lf_set_sst_config_path(const char* config_path) { sst_config_path = config_path; }
