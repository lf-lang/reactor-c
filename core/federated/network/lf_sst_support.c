#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"

static sst_priv_t* sst_priv_init() {
  sst_priv_t* sst_priv = malloc(sizeof(sst_priv_t));
  if (!sst_priv) {
    lf_print_error_and_exit("Falied to malloc sst_priv_t.");
  }
  memset(sst_priv, 0, sizeof(sst_priv_t));
  sst_priv->socket_priv = TCP_socket_priv_init();
  return sst_priv;
}

static void sst_open(netdrv_t* drv) { sst_priv_t* sst_priv = (sst_priv_t*)drv->priv; }
static void sst_close(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  if (sst_priv->socket_priv != NULL) {
    TCP_socket_close(sst_priv->socket_priv);
  } else {
    lf_print_error("Trying to close TCP socket not existing.");
  }
}

void netdrv_free(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  // free(priv); // Already freed on socket close()
  free_SST_ctx_t(sst_priv->sst_ctx);
  free(drv);
}

netdrv_t* netdrv_init() {
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (!drv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(drv, 0, sizeof(netdrv_t));
  drv->open = sst_open;
  drv->close = sst_close;
  // drv->read = socket_read;
  // drv->write = socket_write;
  drv->read_remaining_bytes = 0;

  // Initialize priv.
  sst_priv_t* sst_priv = sst_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)sst_priv;
  return drv;
}

char* get_host_name(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return sst_priv->socket_priv->server_hostname;
}
int32_t get_my_port(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return sst_priv->socket_priv->port;
}
int32_t get_port(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return (sst_priv->socket_priv == NULL) ? -1 : sst_priv->socket_priv->server_port;
}
//
struct in_addr* get_ip_addr(netdrv_t* drv) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  return &sst_priv->socket_priv->server_ip_addr;
}
void set_host_name(netdrv_t* drv, const char* hostname) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  memcpy(sst_priv->socket_priv->server_hostname, hostname, INET_ADDRSTRLEN);
}
void set_port(netdrv_t* drv, int port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->server_port = port;
}

void set_specified_port(netdrv_t* drv, int port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->user_specified_port = port;
}

// Unused.
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->server_ip_addr = ip_addr;
}

// Port will be NULL on MQTT.
int create_server(netdrv_t* drv, int server_type, uint16_t port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  SST_ctx_t* ctx = init_SST(
      "/home/dongha/project/lingua-franca/core/src/main/resources/lib/c/reactor-c/core/federated/RTI/RTI.config");
  sst_priv->sst_ctx = ctx;
  return create_TCP_server(sst_priv->socket_priv, server_type, port);
}

netdrv_t* establish_communication_session(netdrv_t* my_netdrv) {
  netdrv_t* ret_netdrv = netdrv_init();
  sst_priv_t* my_priv = (sst_priv_t*)my_netdrv->priv;
  sst_priv_t* ret_priv = (sst_priv_t*)ret_netdrv->priv;

  // Wait for an incoming connection request.
  struct sockaddr client_fd;
  uint32_t client_length = sizeof(client_fd);
  // The following blocks until a client connects.
  while (1) {
    ret_priv->socket_priv->socket_descriptor =
        accept(my_priv->socket_priv->socket_descriptor, &client_fd, &client_length);
    if (ret_priv->socket_priv->socket_descriptor >= 0) {
      // Got a socket
      break;
    } else if (ret_priv->socket_priv->socket_descriptor < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
      lf_print_error_and_exit("Failed to accept the socket. %s. ret_priv->socket_priv->socket_descriptor = %d",
                              strerror(errno), ret_priv->socket_priv->socket_descriptor);
    } else {
      // Try again
      lf_print_warning("Failed to accept the socket. %s. Trying again.", strerror(errno));
      continue;
    }
  }

  session_key_list_t* s_key_list = init_empty_session_key_list();
  SST_session_ctx_t* session_ctx =
      server_secure_comm_setup(my_priv->sst_ctx, my_priv->socket_priv->socket_descriptor, s_key_list);
  free_session_key_list_t(s_key_list);
  ret_priv->session_ctx = session_ctx;

  // TODO: DONGHA
  // Get the IP address of the other accepting client. This is used in two cases.
  // 1) Decentralized coordination - handle_address_query() - Sends the port number and address of the federate.
  // 2) Clock synchronization - send_physical_clock - Send through UDP.
  struct sockaddr_in* pV4_addr = (struct sockaddr_in*)&client_fd;
  ret_priv->socket_priv->server_ip_addr = pV4_addr->sin_addr;
}

int netdrv_connect(netdrv_t* drv) {
  char cwd[256];
  getcwd(cwd, sizeof(cwd));
  printf("Current working dir: %s\n", cwd);
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  unsigned char* config_path;
  SST_ctx_t* ctx = init_SST(
      "/home/dongha/project/lingua-franca/core/src/main/resources/lib/c/reactor-c/core/federated/network/fed1.config");
  session_key_list_t* s_key_list = get_session_key(ctx, NULL);
  SST_session_ctx_t* session_ctx = secure_connect_to_server(&s_key_list->s_key[0], ctx);
  sst_priv->sst_ctx = ctx;
  sst_priv->session_ctx = session_ctx;
  return 1;
}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {}

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {}
