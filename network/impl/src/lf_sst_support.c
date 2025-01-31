#include "net_driver.h"
#include "lf_sst_support.h"
#include "util.h"

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

