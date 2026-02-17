#ifndef LF_SST_SUPPORT_H
#define LF_SST_SUPPORT_H

#include "socket_common.h"
#include <sst-c-api/c_api.h>

typedef struct sst_priv_t {
  socket_priv_t* socket_priv;
  SST_ctx_t* sst_ctx;
  SST_session_ctx_t* session_ctx;
  unsigned char buffer[MAX_SECURE_COMM_MSG_LENGTH];
  size_t buf_filled;
  size_t buf_off; 
  session_key_t* pending_key;
} sst_priv_t;

typedef struct sst_connection_params_t {
  socket_connection_params_t socket_params;

  // 0 for RTI, 1 for federates.
  int target;
} sst_connection_params_t;

void lf_set_sst_config_path(const char* config_path);

#endif /* LF_SST_SUPPORT_H */
