#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"

static sst_priv_t* sst_priv_init() {
  sst_priv_t* priv = malloc(sizeof(sst_priv_t));
  if (!priv) {
    lf_print_error_and_exit("Falied to malloc sst_priv_t.");
  }
  memset(priv, 0, sizeof(sst_priv_t));
  priv->socket_priv = TCP_socket_priv_init();
  return priv;
}

static void sst_open(netdrv_t* drv) { sst_priv_t* priv = (sst_priv_t*)drv->priv; }
static void sst_close(netdrv_t* drv) {
  sst_priv_t* priv = (sst_priv_t*)drv->priv;
  if (priv->socket_priv != NULL) {
    TCP_socket_close(priv->socket_priv);
  } else {
    lf_print_error("Trying to close TCP socket not existing.");
  }
}

void netdrv_free(netdrv_t* drv) {
  sst_priv_t* priv = (sst_priv_t*)drv->priv;
  // free(priv); // Already freed on socket close()
  free_SST_ctx_t(priv->sst_ctx);
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
  sst_priv_t* priv = sst_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

// Port will be NULL on MQTT.
int create_server(netdrv_t* drv, server_type_t server_type, uint16_t port) {
  sst_priv_t* priv = (sst_priv_t*)drv->priv;
}

// Returns socket number of clock_sync_server.
int create_clock_sync_server(uint16_t* clock_sync_port) {}

netdrv_t* establish_communication_session(netdrv_t* netdrv) {}

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {}
