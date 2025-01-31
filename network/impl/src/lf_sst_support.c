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
  if (get_peer_address(fed_netdrv) != 0) {
    lf_print_error("RTI failed to get peer address.");
  };

  session_key_list_t* s_key_list = init_empty_session_key_list();
  return fed_netdrv;
}

// Helper function.
void lf_set_sst_config_path(const char* config_path) { sst_config_path = config_path; }
