#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "net_abstraction.h"
#include "lf_tls_support.h"
#include "util.h"
#include "logging.h"

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#error "OpenSSL 3.0 or higher is required."
#endif

// Global configuration for TLS (cert and key paths)
static const char* tls_cert_path = NULL;
static const char* tls_key_path = NULL;

static SSL_CTX* global_client_ctx = NULL;
static SSL_CTX* global_server_ctx = NULL;

void lf_set_tls_configuration(const char* cert_path, const char* key_path) {
  tls_cert_path = cert_path;
  tls_key_path = key_path;
}

// Helper to initialize OpenSSL context
static SSL_CTX* create_ssl_context(const SSL_METHOD* method) {
  SSL_CTX* ctx = SSL_CTX_new(method);
  if (!ctx) {
    lf_print_error("Unable to create SSL context");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
  return ctx;
}

static void configure_ssl_context(SSL_CTX* ctx) {
  if (tls_cert_path && tls_key_path) {
    if (SSL_CTX_use_certificate_file(ctx, tls_cert_path, SSL_FILETYPE_PEM) <= 0) {
      lf_print_error("Failed to load certificate file: %s", tls_cert_path);
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, tls_key_path, SSL_FILETYPE_PEM) <= 0) {
      lf_print_error("Failed to load private key file: %s", tls_key_path);
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
    }
  }
}

net_abstraction_t initialize_net() {
  tls_priv_t* priv = (tls_priv_t*)malloc(sizeof(tls_priv_t));
  if (priv == NULL) {
    lf_print_error_and_exit("Failed to allocate memory for tls_priv_t.");
  }

  // Initialize socket_priv
  priv->socket_priv = (socket_priv_t*)malloc(sizeof(socket_priv_t));
  if (priv->socket_priv == NULL) {
    free(priv);
    lf_print_error_and_exit("Failed to allocate memory for socket_priv_t.");
  }

  // Default initialization for socket_priv
  priv->socket_priv->port = 0;
  priv->socket_priv->user_specified_port = 0;
  priv->socket_priv->socket_descriptor = -1;
  strncpy(priv->socket_priv->server_hostname, "localhost", INET_ADDRSTRLEN);
  priv->socket_priv->server_ip_addr.s_addr = 0;
  priv->socket_priv->server_port = -1;

  priv->ctx = NULL;
  priv->ssl = NULL;

  return (net_abstraction_t)priv;
}

void free_net(net_abstraction_t net_abs) {
  if (net_abs == NULL)
    return;
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  if (priv->ssl) {
    SSL_free(priv->ssl);
  }
  // Note: We generally don't free the global context here if it's shared,
  // but if it's per-connection, we should.
  // Assuming global context is managed separately or lives until exit.

  if (priv->socket_priv) {
    free(priv->socket_priv);
  }
  free(priv);
}

int create_server(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  // Initialize Global Context if not already done
  if (global_server_ctx == NULL) {
    global_server_ctx = create_ssl_context(TLS_server_method());
    configure_ssl_context(global_server_ctx);
  }
  priv->ctx = global_server_ctx;

  // Create the underlying socket server
  return create_socket_server(priv->socket_priv->user_specified_port, &priv->socket_priv->socket_descriptor,
                              &priv->socket_priv->port, TCP);
}

net_abstraction_t accept_net(net_abstraction_t server_chan) {
  LF_ASSERT_NON_NULL(server_chan);
  tls_priv_t* server_priv = (tls_priv_t*)server_chan;

  // Accept TCP connection
  int client_sock = accept_socket(server_priv->socket_priv->socket_descriptor);
  if (client_sock < 0) {
    return NULL;
  }

  // Initialize new network abstraction for the client
  net_abstraction_t client_net = initialize_net();
  tls_priv_t* client_priv = (tls_priv_t*)client_net;
  client_priv->socket_priv->socket_descriptor = client_sock;

  // Share the context (or create new if needed, but sharing is standard for server)
  client_priv->ctx = server_priv->ctx;

  // Create SSL structure
  client_priv->ssl = SSL_new(client_priv->ctx);
  SSL_set_fd(client_priv->ssl, client_sock);

  SSL_set_accept_state(client_priv->ssl);
  // Perform TLS handshake (accept)
  if (SSL_accept(client_priv->ssl) <= 0) {
    lf_print_error("SSL_accept failed.");
    ERR_print_errors_fp(stderr);

    shutdown_net(client_net, false);
    return NULL;
  }

  // Create generic socket info (peer address)
  if (get_peer_address(client_priv->socket_priv) != 0) {
    lf_print_error("Failed to save peer address.");
  }

  return client_net;
}

void create_client(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  // Create the underlying TCP socket
  priv->socket_priv->socket_descriptor = create_real_time_tcp_socket_errexit();

  if (global_client_ctx == NULL) {
    global_client_ctx = create_ssl_context(TLS_client_method());
  }
  priv->ctx = global_client_ctx;
}

net_abstraction_t connect_to_net(net_params_t* params) {
  tls_connection_params_t* tls_params = (tls_connection_params_t*)params;

  net_abstraction_t net = initialize_net();
  tls_priv_t* priv = (tls_priv_t*)net;

  // Set socket params
  priv->socket_priv->server_port = tls_params->socket_params.port;
  memcpy(priv->socket_priv->server_hostname, tls_params->socket_params.server_hostname, INET_ADDRSTRLEN);

  create_client(net);

  // TCP Connect
  if (connect_to_socket(priv->socket_priv->socket_descriptor, priv->socket_priv->server_hostname,
                        priv->socket_priv->server_port) != 0) {
    lf_print_error("Failed to connect to socket.");
    free_net(net);
    return NULL;
  }

  // SSL Connect
  priv->ssl = SSL_new(priv->ctx);
  SSL_set_fd(priv->ssl, priv->socket_priv->socket_descriptor);

  if (SSL_connect(priv->ssl) <= 0) {
    lf_print_error("SSL_connect failed.");
    ERR_print_errors_fp(stderr);
    shutdown_net(net, false);
    return NULL;
  }

  return net;
}

static int is_disconnect_syscall(int err, int ret) {
  if (err != SSL_ERROR_SYSCALL)
    return 0;

  if (ret == 0) {
    // Often: "unexpected EOF while reading" / peer closed without close_notify
    return 1;
  }
  if (ret == -1) {
    // RST/timeout/pipe, treat as disconnect if you want EOF semantics
    if (errno == ECONNRESET || errno == EPIPE || errno == ETIMEDOUT || errno == ENOTCONN) {
      return 1;
    }
  }
  return 0;
}

int read_from_net(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  size_t bytes_read = 0;
  while (bytes_read < num_bytes) {
    int ret = SSL_read(priv->ssl, buffer + bytes_read, num_bytes - bytes_read);
    if (ret > 0) {
      bytes_read += (size_t)ret;
      continue;
    }

    int err = SSL_get_error(priv->ssl, ret);

    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      lf_sleep(DELAY_BETWEEN_SOCKET_RETRIES);
      continue;
    }

    if (err == SSL_ERROR_ZERO_RETURN) {
      // close_notify received
      return 1; // EOF
    }

    if (is_disconnect_syscall(err, ret)) {
      // peer disconnected without close_notify (or reset)
      return 1; // treat as EOF
    }

    // Real TLS/protocol error
    lf_print_error("SSL_read failed (ret=%d, err=%d, errno=%d)", ret, err, errno);
    ERR_print_errors_fp(stderr);
    return -1;
  }
  return 0;
}
int read_from_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  int ret = read_from_net(net_abs, num_bytes, buffer);
  if (ret < 0) {
    shutdown_net(net_abs, false);
    return -1;
  }
  return ret; // 0 for success, 1 for EOF
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
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  size_t bytes_written = 0;
  while (bytes_written < num_bytes) {
    int ret = SSL_write(priv->ssl, buffer + bytes_written, num_bytes - bytes_written);
    if (ret > 0) {
      bytes_written += ret;
    } else {
      int err = SSL_get_error(priv->ssl, ret);
      if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        continue;
      }
      lf_print_error("SSL_write failed with error %d", err);
      ERR_print_errors_fp(stderr);
      return -1;
    }
  }
  return 0;
}

int write_to_net_close_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer) {
  int ret = write_to_net(net_abs, num_bytes, buffer);
  if (ret < 0) {
    shutdown_net(net_abs, false);
    return -1;
  }
  return 0;
}

void write_to_net_fail_on_error(net_abstraction_t net_abs, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                char* format, ...) {
  va_list args;
  int ret = write_to_net_close_on_error(net_abs, num_bytes, buffer);
  if (ret < 0) {
    if (mutex)
      LF_MUTEX_UNLOCK(mutex);

    if (format != NULL) {
      va_start(args, format);
      lf_print_error_system_failure(format, args);
      va_end(args);
    } else {
      lf_print_error_system_failure("Failed to write to TLS connection.");
    }
  }
}

bool is_net_open(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  return is_socket_open(priv->socket_priv->socket_descriptor);
}

int shutdown_net(net_abstraction_t net_abs, bool read_before_closing) {
  if (net_abs == NULL)
    return 0;
  tls_priv_t* priv = (tls_priv_t*)net_abs;

  if (priv->ssl) {
    SSL_shutdown(priv->ssl);
    // We might want to read pending data here if read_before_closing is true,
    // but SSL_shutdown usually sends notify_close.
  }

  // Shutdown underlying socket
  if (priv->socket_priv) {
    shutdown_socket(&priv->socket_priv->socket_descriptor, read_before_closing);
  }

  free_net(net_abs);
  return 0;
}

int32_t get_my_port(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  return priv->socket_priv->port;
}

int32_t get_server_port(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  return priv->socket_priv->server_port;
}

struct in_addr* get_ip_addr(net_abstraction_t net_abs) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  return &priv->socket_priv->server_ip_addr;
}

void set_my_port(net_abstraction_t net_abs, int32_t port) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  priv->socket_priv->user_specified_port = port;
}

void set_server_port(net_abstraction_t net_abs, int32_t port) {
  LF_ASSERT_NON_NULL(net_abs);
  tls_priv_t* priv = (tls_priv_t*)net_abs;
  priv->socket_priv->server_port = port;
}
