#include "lf_mqtt_support.h"

/**
 * @brief 
 * Initializes structure of netdrv, and priv inside.
 * Allocate memory.
 * Check lf_socket_support.c for example.
 * 
 * @return netdrv_t* 
 */
netdrv_t* netdrv_init() {}


/**
 * @brief Create a server object
 * Initializes MQTT client, and connects to broker.
 * MQTTClient_connect()
 * Check socket_common.c for example. It is a common function because also lf_sst_support.c will also use it.
 * @param drv 
 * @param server_type 
 * @param port The port is NULL here.
 * @return int 
 */
int create_server(netdrv_t* drv, server_type_t server_type, uint16_t port) { }



/**
 * @brief 
 * 1. RTI subscribes “{Federation_ID}_RTI” topic.
 * 2. Each federate publishes fed_id to {Federation_ID}_RTI
 * 3. fed_{n} subscribes to “{Federation_Id}_RTI_to_fed_{n}”.
 * 4. RTI subscribes to “{Federation_Id}_fed_{n}_to_RTI”.
 * Check lf_socket_support.c for example.

 * @param netdrv 
 * @return netdrv_t* 
 */
netdrv_t* establish_communication_session(netdrv_t* netdrv) {}


/**
 * @brief Publish message.
 * 
 * @param drv 
 * @param num_bytes 
 * @param buffer 
 * @return int 
 */
int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);


int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer);

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...);


ssize_t read_from_netdrv(netdrv_t* netdrv, unsigned char* buffer, size_t buffer_length);


ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length);


void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...);