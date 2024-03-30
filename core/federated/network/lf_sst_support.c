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

// Unused.
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  sst_priv->socket_priv->server_ip_addr = ip_addr;
}

// Port will be NULL on MQTT.
int create_server(netdrv_t* drv, int server_type, uint16_t port) {
  sst_priv_t* sst_priv = (sst_priv_t*)drv->priv;
  SST_ctx_t* ctx = init_SST(
      "/home/dongha/project/lingua-franca/core/src/main/resources/lib/c/reactor-c/core/federated/RTI/c_client.config");
  sst_priv->sst_ctx = ctx;
  return create_TCP_server(sst_priv->socket_priv, server_type, port);
}

netdrv_t* establish_communication_session(netdrv_t* netdrv) {
  netdrv_t* ret_netdrv = netdrv_init();
  sst_priv_t* my_priv = (sst_priv_t*)my_netdrv->priv;
  sst_priv_t* ret_priv = (sst_priv_t*)ret_netdrv->priv;
  session_key_list_t *s_key_list = init_empty_session_key_list();
  SST_session_ctx_t* session_ctx = server_secure_comm_setup(my_priv->sst_ctx, my_priv->socket_priv.socket_descriptor, s_key_list);
  free_session_key_list_t(s_key_list);
  ret_priv->session_ctx = session_ctx;
}

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {}
