#include <stdlib.h> // malloc()
#include <string.h> // strncpy()
#include <stdio.h>

#include "net_abstraction.h"
#include "lf_sst_support.h"
#include "util.h"

const char* sst_config_path; // The SST's configuration file path.

SST_ctx_t* ctx;

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
  sst_priv->buf_filled = 0;
  sst_priv->buf_off = 0;

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
  LF_ASSERT_NON_NULL(net_abs);
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  if (ctx == NULL) {
    ctx = init_SST(sst_config_path);
    if (ctx == NULL) {
      lf_print_error_and_exit("Failed to initialze SST settings.");
    }
  }
  priv->sst_ctx = ctx;
  return create_socket_server(priv->socket_priv->user_specified_port, &priv->socket_priv->socket_descriptor,
                              &priv->socket_priv->port, TCP);
}

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
  LF_ASSERT_NON_NULL(net_abs);
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  priv->socket_priv->socket_descriptor = create_real_time_tcp_socket_errexit();
  if (ctx == NULL) {
    ctx = init_SST(sst_config_path);
    if (ctx == NULL) {
      lf_print_error_and_exit("Failed to initialze SST settings.");
    }
  }
  priv->sst_ctx = ctx;
}

net_abstraction_t connect_to_net(net_params_t* params) {
  // Create a network abstraction.
  net_abstraction_t net = initialize_net();
  sst_priv_t* priv = (sst_priv_t*)net;
  sst_connection_params_t* sst_params = (sst_connection_params_t*)params;
  priv->socket_priv->server_port = sst_params->socket_params.port;
  memcpy(priv->socket_priv->server_hostname, sst_params->socket_params.server_hostname, INET_ADDRSTRLEN);
  // Create the client network abstraction.
  create_client(net);
  // Connect to the target server.
  if (connect_to_socket(priv->socket_priv->socket_descriptor, priv->socket_priv->server_hostname,
                        priv->socket_priv->server_port) != 0) {
    lf_print_error("Failed to connect to socket.");
    return NULL;
  }
  if (sst_params->target == 1) {
    // Override target group to federates.
    snprintf(priv->sst_ctx->config.purpose[ctx->config.purpose_index],
             sizeof(ctx->config.purpose[ctx->config.purpose_index]), "{\"group\":\"Federates\"}");
  }
  session_key_list_t* s_key_list = get_session_key(priv->sst_ctx, NULL);
  SST_session_ctx_t* session_ctx =
      secure_connect_to_server_with_socket(&s_key_list->s_key[0], priv->socket_priv->socket_descriptor);
  priv->session_ctx = session_ctx;
  return net;
}

int read_from_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  sst_priv_t* priv = (sst_priv_t*)net_abs;

  if (num_bytes > MAX_SECURE_COMM_MSG_LENGTH) {
    lf_print_error("Unable to handle message. Expected: %zu, Maximum: %d", num_bytes, MAX_SECURE_COMM_MSG_LENGTH);
    return -1;
  }
  size_t copied = 0;
  // 1) First use buffered data.
  if (priv->buf_off < priv->buf_filled) {
    size_t avail = priv->buf_filled - priv->buf_off;
    size_t to_copy = (avail < num_bytes) ? avail : num_bytes;
    memcpy(buffer, priv->buffer + priv->buf_off, to_copy);
    priv->buf_off += to_copy;
    copied += to_copy;

    // Reset buffer offset when the buffer is all used.
    if (priv->buf_off == priv->buf_filled) {
      priv->buf_off = priv->buf_filled = 0;
    }

    // Return when the buffered data is enough.
    if (copied == num_bytes) {
      return 0;
    }
  }

  // 2) Additionally try to read more bytes.
  while (copied < num_bytes) {
    int ret = read_secure_message(priv->buffer, priv->session_ctx);
    if (ret == 0) {
      // EOF received.
      return 1;
    } else if (ret < 0) {
      lf_print_error("read_secure_message failed: %d", ret);
      return -1;
    }

    // Mark the filled length and reset offset.
    priv->buf_filled = (size_t)ret;
    priv->buf_off = 0;

    size_t need = num_bytes - copied;
    size_t to_copy = (priv->buf_filled < need) ? priv->buf_filled : need;
    memcpy(buffer + copied, priv->buffer + priv->buf_off, to_copy);
    priv->buf_off += to_copy;
    copied += to_copy;

    // Reset buffer offset when meets the end of the filled buffer.
    if (priv->buf_off == priv->buf_filled) {
      priv->buf_off = priv->buf_filled = 0;
    }
  }

  return 0;
}

int read_from_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
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
  LF_ASSERT_NON_NULL(net_abs);
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
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return send_secure_message((char*)buffer, (unsigned int)num_bytes, priv->session_ctx);
}

int write_to_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
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
  LF_ASSERT_NON_NULL(net_abs);
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
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return is_socket_open(priv->socket_priv->socket_descriptor);
}

int shutdown_net(net_abstraction_t net_abs, bool read_before_closing) {
  if (net_abs == NULL) {
    LF_PRINT_LOG("Socket already closed.");
    return 0;
  }
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  int ret = shutdown_socket(&priv->socket_priv->socket_descriptor, read_before_closing);
  free_net(net_abs);
  return ret;
}

// Helper function.
void lf_set_sst_config_path(const char* config_path) { sst_config_path = config_path; }

// Get/set functions.
int32_t get_my_port(net_abstraction_t net_abs) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return priv->socket_priv->port;
}

int32_t get_server_port(net_abstraction_t net_abs) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return priv->socket_priv->server_port;
}

struct in_addr* get_ip_addr(net_abstraction_t net_abs) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  return &priv->socket_priv->server_ip_addr;
}

void set_my_port(net_abstraction_t net_abs, int32_t port) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  priv->socket_priv->user_specified_port = port;
}

void set_server_port(net_abstraction_t net_abs, int32_t port) {
  sst_priv_t* priv = (sst_priv_t*)net_abs;
  priv->socket_priv->server_port = port;
}
