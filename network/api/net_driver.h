#ifndef NET_DRIVER_H
#define NET_DRIVER_H

#include "socket_common.h"

typedef enum server_type_t { RTI, FED } server_type_t;
typedef struct netdrv_t {
  void* priv;
  //   unsigned int read_remaining_bytes;
  //   int my_federate_id; // The RTI is -1, and unitialized is -2. This must be int not uint16_t
  //   const char* federation_id;
} netdrv_t;

/**
 * Allocate memory for the netdriver.
 * @return netdrv_t*
 */
netdrv_t* initialize_netdrv();

/**
 * Create a netdriver server. This is such as a server socket which accepts connections. However this is only the creation of the server netdriver.
 * 
 * @param drv Server's network driver.
 * @param serv_type Type of server, RTI or FED.
 * @return int 0 for success, -1 for failure.
 */
int create_server_(netdrv_t* drv, server_type_t serv_type);

#endif /* NET_DRIVER_H */
