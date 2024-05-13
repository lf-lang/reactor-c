#ifndef NETDRIVER_H
#define NETDRIVER_H

#include "low_level_platform.h"

#include "socket_common.h"

#if defined(COMM_TYPE_TCP)
#include "lf_socket_support.h"
#elif defined(COMM_TYPE_MQTT)
#include "lf_mqtt_support.h"
#elif defined(COMM_TYPE_SST)
#include "lf_sst_support.h"
#endif

typedef enum netdrv_type_t { NETDRV, UDP } netdrv_type_t;

// Just doing 0 for RTI, 1 for FED
typedef enum server_type_t { RTI, FED } server_type_t;

typedef struct netdrv_t {
  void* priv;
  unsigned int read_remaining_bytes;
  int federate_id;
  const char* federation_id;
} netdrv_t;

/**
 * @brief Allocate memory for the netdriver, save the federate_id, and federation_id used for the netdriver.
 * If the netdriver belongs to the RTI, the federtae_id is -1.
 * @param federate_id
 * @param federation_id
 * @return netdrv_t*
 */
netdrv_t* initialize_netdrv(int federate_id, const char* federation_id);

netdrv_t* initialize_common_netdrv(int federate_id, const char* federation_id);

/**
 * @brief Close connections, and free allocated memory.
 *
 * @param drv
 */
void close_netdrv(netdrv_t* drv);

// Port will be NULL on MQTT.
/**
 * @brief Create a netdriver server. This is such as a server socket which accepts connections. However this is only the
 * creation of the server netdriver.
 *
 * @param drv
 * @param server_type
 * @param port
 * @return int
 */
int create_server(netdrv_t* drv, server_type_t server_type, uint16_t port);

/**
 * @brief Creates a communications session.
 *
 * @param netdrv
 * @return netdrv_t*
 */
netdrv_t* establish_communication_session(netdrv_t* netdrv);

/**
 * @brief Create a netdriver client.
 *
 * @param drv
 */
void create_client(netdrv_t* drv);

/**
 * @brief Request connect to the target server.
 *
 * @param drv
 * @return int
 */
int connect_to_netdrv(netdrv_t* drv);

int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);

// Return 0 when connection lost. -1 on error. > 0 bytes read.
ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length);

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length);

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...);

// Temporary //////////////////
char* get_host_name(netdrv_t* drv);
int32_t get_my_port(netdrv_t* drv);
int32_t get_port(netdrv_t* drv);
struct in_addr* get_ip_addr(netdrv_t* drv);

void set_host_name(netdrv_t* drv, const char* hostname);
void set_port(netdrv_t* drv, int port);
void set_specified_port(netdrv_t* drv, int port);
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr);

// Returns socket number of clock_sync_server.
int create_clock_sync_server(uint16_t* clock_sync_port);

/**
 * Without blocking, peek at the specified socket and, if there is
 * anything on the queue, put its first byte at the specified address and return 1.
 * If there is nothing on the queue, return 0, and if an error occurs,
 * return -1.
 * @param socket The socket ID.
 * @param result Pointer to where to put the first byte available on the socket.
 */
ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result);

////////////////////////////

#endif /* NETDRIVER_H */
