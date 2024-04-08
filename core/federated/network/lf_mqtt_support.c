#include "netdriver.h"

static MQTT_priv_t* MQTT_priv_init() {
  MQTT_priv_t* MQTT_priv = malloc(sizeof(MQTT_priv_t));
  if (!MQTT_priv) {
    lf_print_error_and_exit("Falied to malloc MQTT_priv_t.");
  }
  memset(MQTT_priv, 0, sizeof(MQTT_priv_t));
  return MQTT_priv;
}

/**
 * @brief
 * Initializes structure of netdrv, and priv inside.
 * Allocate memory.
 * Check lf_socket_support.c for example.
 *
 * @return netdrv_t*
 */
netdrv_t* netdrv_init(int federate_id, const char* federation_id) {
  // FIXME: Delete below.
  printf("\n\t[MQTT PROTOCOL]\n\n");
  netdrv_t* drv = malloc(sizeof(netdrv_t));
  if (!drv) {
    lf_print_error_and_exit("Falied to malloc netdrv_t.");
  }
  memset(drv, 0, sizeof(netdrv_t));
  drv->open = MQTT_open;
  drv->close = MQTT_close;
  // drv->read = socket_read;
  // drv->write = socket_write;
  drv->read_remaining_bytes = 0;
  drv->federate_id = federate_id;
  drv->federation_id = federation_id;

  // Initialize priv.
  MQTT_priv_t* priv = MQTT_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

char* get_host_name(netdrv_t* drv) {}

int32_t get_my_port(netdrv_t* drv) {}

int32_t get_port(netdrv_t* drv) {}

struct in_addr* get_ip_addr(netdrv_t* drv) {}

void set_host_name(netdrv_t* drv, const char* hostname) {}

void set_port(netdrv_t* drv, int port) {}
void set_specified_port(netdrv_t* drv, int port) {}

void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {}

void netdrv_free(netdrv_t* drv) {}

/**
 * @brief Create a server object
 * Initializes MQTT client, and connects to broker.
 * MQTTClient_connect()
 * RTI subscribes “{Federation_ID}_RTI” topic.
 * Check socket_common.c for example. It is a common function because also lf_sst_support.c will also use it.
 * @param drv
 * @param server_type
 * @param port The port is NULL here.
 * @return int
 */
int create_server(netdrv_t* drv, int server_type, uint16_t port) {

}

/**
 * @brief
 * 1. Each federate publishes fed_id to {Federation_ID}_RTI
 * 2. fed_{n} subscribes to “{Federation_Id}_RTI_to_fed_{n}”.
 * 3. RTI subscribes to “{Federation_Id}_fed_{n}_to_RTI”.
 * Check lf_socket_support.c for example.

 * @param netdrv
 * @return netdrv_t*
 */
netdrv_t* establish_communication_session(netdrv_t* netdrv) {}

int netdrv_connect(netdrv_t* drv) {}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {}

/**
 * @brief Publish message.
 *
 * @param drv
 * @param num_bytes
 * @param buffer
 * @return int
 */
int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {}

// int create_clock_sync_server(uint16_t* clock_sync_port) {}
