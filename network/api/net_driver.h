#ifndef NET_DRIVER_H
#define NET_DRIVER_H

#include "socket_common.h"

typedef struct netdrv_t {
  void* priv;
  //   unsigned int read_remaining_bytes;
  //   int my_federate_id; // The RTI is -1, and unitialized is -2. This must be int not uint16_t
  //   const char* federation_id;
} netdrv_t;

#endif /* NET_DRIVER_H */
