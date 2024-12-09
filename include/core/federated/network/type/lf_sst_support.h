#ifndef LF_SST_SUPPORT_H
#define LF_SST_SUPPORT_H

#include "socket_common.h"
#include <sst-c-api/c_api.h>
typedef struct sst_priv_t {
  socket_priv_t* socket_priv; // Must be first variable.
  SST_ctx_t* sst_ctx;
  SST_session_ctx_t* session_ctx;

  // RTI:
  // RTI_netdrv needs a sst_ctx, but not session_ctx;
  // fed_netdrv needs only session_ctx not sst_ctx.

  // Centralized Federate:
  // netdrv_to_rti needs both sst_ctx and session_ctx
  // Decentralized Federate:
  // my_netdrv needs a sst_ctx, but not session_ctx;
  // netdrv_for_inbound_p2p_connections and outbound needs only session_ctx not sst_ctx.

} sst_priv_t;

void lf_set_sst_config_path(const char* config_path);
void lf_set_rti_sst_config_path(const char* config_path);


#endif // LF_SST_SUPPORT_H
