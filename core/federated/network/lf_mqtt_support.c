#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
#include "lf_mqtt_support.h"

// #include <MQTTClient.h>

#define ADDRESS "tcp://mqtt.eclipseprojects.io:1883"
#define QOS 2
#define TIMEOUT 10000L

static MQTT_priv_t* MQTT_priv_init() {
  MQTT_priv_t* MQTT_priv = malloc(sizeof(MQTT_priv_t));
  if (!MQTT_priv) {
    lf_print_error_and_exit("Falied to malloc MQTT_priv_t.");
  }
  memset(MQTT_priv, 0, sizeof(MQTT_priv_t));
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  memcpy(&MQTT_priv->conn_opts, &conn_opts, sizeof(MQTTClient_connectOptions));
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  memcpy(&MQTT_priv->pubmsg, &pubmsg, sizeof(MQTTClient_message));
  return MQTT_priv;
}

static const char* create_topic_federation_id_rti(const char* federation_id) {
  // Determine the maximum length of the resulting string
  int max_length = snprintf(NULL, 0, "%s_RTI", federation_id) + 1; // +1 for null terminator

  // Allocate memory for the resulting string
  char* result = (char*)malloc(max_length);
  if (result == NULL) {
    lf_print_error_and_exit("Falied to malloc.");
    return NULL;
  }

  // Format the string using snprintf
  snprintf(result, max_length, "%s_RTI", federation_id);

  return (const char*) result;
}

static void set_client_id(MQTT_priv_t* MQTT_priv, int id) {
  if (id == -1) {
    strcat(MQTT_priv->client_id, "RTI");
  }
  sprintf(MQTT_priv->client_id, "%d", id);
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
  // drv->open = MQTT_open;
  // drv->close = MQTT_close;
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
 * RTI subscribes “{Federation_ID}_RTI” topic. This should be done here, because the establish_communication_session
 * loops. Check socket_common.c for example. It is a common function because also lf_sst_support.c will also use it.
 * @param drv
 * @param server_type
 * @param port The port is NULL here.
 * @return int
 */
int create_server(netdrv_t* drv, int server_type, uint16_t port) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  set_client_id(MQTT_priv, drv->federate_id);
  int rc;
  if ((rc = MQTTClient_create(&MQTT_priv->client, ADDRESS, MQTT_priv->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to create client, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }

  MQTT_priv->conn_opts.keepAliveInterval = 20;
  MQTT_priv->conn_opts.cleansession = 1;
  if ((rc = MQTTClient_connect(MQTT_priv->client, &MQTT_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect, return code %d\n", rc);
    exit(EXIT_FAILURE);
  }
  const char* topic = create_topic_federation_id_rti(drv->federation_id);
  if ((rc = MQTTClient_subscribe(MQTT_priv->client, topic, QOS)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to subscribe, return code %d\n", rc);
    rc = EXIT_FAILURE;
  }
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

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  char* topicName = NULL;
  int topicLen;
  MQTTClient_message* message = NULL;
  int rc;
  if ((rc = MQTTClient_receive(MQTT_priv->client, &topicName, &topicLen, &message, 1000000)) != MQTTCLIENT_SUCCESS) {
    MQTTClient_free(topicName);
    MQTTClient_freeMessage(&message);
    lf_print_error_and_exit("Failed to receive message, return code %d\n", rc);
  }
  MQTTClient_free(topicName);
  MQTTClient_freeMessage(&message);
}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {}

// int create_clock_sync_server(uint16_t* clock_sync_port) {}
