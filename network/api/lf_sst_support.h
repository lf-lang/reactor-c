#ifndef LF_SST_SUPPORT_H
#define LF_SST_SUPPORT_H

#include "socket_common.h"
#include <sst-c-api/c_api.h>

typedef struct sst_priv_t {
  socket_priv_t* socket_priv;
  SST_ctx_t* sst_ctx;
  SST_session_ctx_t* session_ctx;
} sst_priv_t;

void lf_set_sst_config_path(const char* config_path);

#endif /* LF_SST_SUPPORT_H */
