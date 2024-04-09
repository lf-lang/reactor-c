#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

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
  return MQTT_priv;
}

static char* create_topic_federation_id_rti(const char* federation_id) {
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

  return result;
}

static char* create_topic_federation_id_fed_id_to_rti(const char* federation_id, uint16_t fed_id) {
  // Determine the maximum length of the resulting string
  int max_length = snprintf(NULL, 0, "%s_fed_%d_to_RTI", federation_id, fed_id) + 1; // +1 for null terminator

  // Allocate memory for the resulting string
  char* result = (char*)malloc(max_length);
  if (result == NULL) {
    lf_print_error_and_exit("Falied to malloc.");
    return NULL;
  }

  // Format the string using snprintf
  snprintf(result, max_length, "%s_fed_%d_to_RTI", federation_id, fed_id);

  return result;
}

static char* create_topic_federation_id_rti_to_fed_id(const char* federation_id, uint16_t fed_id) {
  // Determine the maximum length of the resulting string
  int max_length = snprintf(NULL, 0, "%s_RTI_to_fed_%d", federation_id, fed_id) + 1; // +1 for null terminator

  // Allocate memory for the resulting string
  char* result = (char*)malloc(max_length);
  if (result == NULL) {
    lf_print_error_and_exit("Falied to malloc.");
    return NULL;
  }

  // Format the string using snprintf
  snprintf(result, max_length, "%s_RTI_to_fed_%d", federation_id, fed_id);

  return result;
}

// Function to encode data as Base64 using OpenSSL's EVP_EncodeBlock()
static char* base64_encode(const unsigned char* input, int input_len, int* output_len) {
    // Calculate the maximum possible length of the Base64 encoded data
    int max_encoded_len = (((input_len + 2) / 3) * 4) + 1; // +1 for null terminator

    // Allocate memory for the Base64 encoded data
    char* encoded_data = (char*)malloc(max_encoded_len);
    if (encoded_data == NULL) {
        *output_len = 0;
        return NULL; // Memory allocation failed
    }

    // Encode the input data as Base64
    *output_len = EVP_EncodeBlock((unsigned char*)encoded_data, input, input_len);
    return encoded_data;
}

// Function to encode data as Base64 using OpenSSL's EVP_DecodeBlock()
static unsigned char* base64_decode(const unsigned char* input, int input_len, int* output_len) {
    // Allocate memory for the output buffer
    unsigned char* output = (unsigned char*)malloc(input_len);
    if (output == NULL) {
        return NULL; // Memory allocation failed
    }

    // Decode the Base64 data
    // TODO: DONGHA This can have errors, because this may add 0bit paddings.
    *output_len = EVP_DecodeBlock(output, input, input_len);

    return output;
}

static void set_client_id(MQTT_priv_t* MQTT_priv, int id) {
  if (id == -1) {
    strcat(MQTT_priv->client_id, "RTI");
  }
  sprintf(MQTT_priv->client_id, "%d", id);
}

/**
 * @brief Federate connects to broker.
 *
 * @param drv
 */
static void MQTT_open(netdrv_t* drv) {
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
    lf_print_error_and_exit("Failed to connect, return code %d\n", rc);
  }
}
static void MQTT_close(netdrv_t* drv) {}

/**
 * @brief
 * Initializes structure of netdrv, and priv inside.
 * Allocate memory.
 * Check lf_socket_support.c for example.
 *
 * @return netdrv_t*
 */
netdrv_t* netdrv_init(int federate_id, const char* federation_id) {
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

void netdrv_free(netdrv_t* drv) {
  MQTT_priv_t* priv = (MQTT_priv_t*)drv->priv;
  // free(priv); // Already freed on socket close()
  free(drv);
}

char* get_host_name(netdrv_t* drv) {}

int32_t get_my_port(netdrv_t* drv) {}

int32_t get_port(netdrv_t* drv) {}

struct in_addr* get_ip_addr(netdrv_t* drv) {}

void set_host_name(netdrv_t* drv, const char* hostname) {}

void set_port(netdrv_t* drv, int port) {}
void set_specified_port(netdrv_t* drv, int port) {}

void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {}

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
    lf_print_error_and_exit("Failed to connect, return code %d\n", rc);
  }
  MQTT_priv->topic_name = (const char*) create_topic_federation_id_rti(drv->federation_id);
  if ((rc = MQTTClient_subscribe(MQTT_priv->client, MQTT_priv->topic_name, QOS)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to subscribe, return code %d\n", rc);
  }
  LF_PRINT_LOG("Subscribing on topic %s.", MQTT_priv->topic_name);

  return 1;
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
netdrv_t* establish_communication_session(netdrv_t* my_netdrv) {
  netdrv_t* ret_netdrv = netdrv_init(-2, my_netdrv->federation_id);
  // MQTT_priv_t* my_priv = (MQTT_priv_t*)my_netdrv->priv;
  MQTT_priv_t* ret_priv = (MQTT_priv_t*)ret_netdrv->priv;
  unsigned char buffer[sizeof(uint16_t)];
  read_from_netdrv_fail_on_error(my_netdrv, buffer, sizeof(uint16_t), NULL, "MQTT receive failed.");
  uint16_t fed_id = extract_uint16(buffer + 1);
  ret_netdrv->federate_id = (int)fed_id;

  set_client_id(ret_priv, ret_netdrv->federate_id);
  int rc;
  if ((rc = MQTTClient_create(&ret_priv->client, ADDRESS, ret_priv->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to create client, return code %d\n", rc);
  }

  ret_priv->conn_opts.keepAliveInterval = 20;
  ret_priv->conn_opts.cleansession = 1;
  if ((rc = MQTTClient_connect(ret_priv->client, &ret_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to connect, return code %d\n", rc);
  }

  ret_priv->topic_name = (const char*) create_topic_federation_id_fed_id_to_rti(ret_netdrv->federation_id, fed_id);
  if ((rc = MQTTClient_subscribe(ret_priv->client, ret_priv->topic_name, QOS)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to subscribe, return code %d\n", rc);
  }
  LF_PRINT_LOG("Subscribing on topic %s.", ret_priv->topic_name);

  return ret_netdrv;
}
/**
 * @brief Federate publishes it's federate ID to "{Federation_ID}_RTI", then subscribes to
 * "{Federation_ID}_RTI_to_fed{fed_id}"
 *
 * @param drv
 * @return int
 */
int netdrv_connect(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  int rc;
  MQTT_priv->topic_name = (const char*) create_topic_federation_id_rti(drv->federation_id);
  unsigned char buffer[sizeof(uint16_t)];
  encode_uint16((uint16_t)drv->federate_id, buffer);
  uint16_t fed_id = extract_uint16(buffer + 1);
  write_to_netdrv_fail_on_error(drv, sizeof(uint16_t), buffer, NULL, "Failed to write federate_id_to RTI for connection through MQTT.");
  MQTT_priv->topic_name = (const char*) create_topic_federation_id_rti_to_fed_id(drv->federation_id, drv->federate_id);
  if ((rc = MQTTClient_subscribe(MQTT_priv->client, MQTT_priv->topic_name, QOS)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to subscribe, return code %d\n", rc);
  }
  LF_PRINT_LOG("Subscribing on topic %s.", MQTT_priv->topic_name);

}

ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {}

/**
 * @brief Publish message.
 *
 * @param drv
 * @param num_bytes
 * @param buffer
 * @return int
 */
int write_to_netdrv(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  int rc;
  pubmsg.payload = (void *) base64_encode(buffer, num_bytes, &pubmsg.payloadlen);
  // pubmsg.payload = buffer;
  // pubmsg.payloadlen = num_bytes;
  pubmsg.qos = QOS;
  pubmsg.retained = 0;
  // pubmsg.payloadlen does not include null terminator. Only characters.
  if ((rc = MQTTClient_publishMessage(MQTT_priv->client, MQTT_priv->topic_name, &pubmsg, &token)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to publish message, return code %d\n", rc);
  }
  LF_PRINT_LOG("Message publishing on topic %s is %.*s", MQTT_priv->topic_name, pubmsg.payloadlen, (char*)(pubmsg.payload));
  rc = MQTTClient_waitForCompletion(MQTT_priv->client, token, TIMEOUT);
  free(pubmsg.payload);
  return 1;
}

int write_to_netdrv_close_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer) {
  int bytes_written = write_to_netdrv(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    // Netdrv has probably been closed from the other side.
    // Shut down and close the netdrv from this side.
    drv->close(drv);
  }
  return bytes_written;
}

void write_to_netdrv_fail_on_error(netdrv_t* drv, size_t num_bytes, unsigned char* buffer, lf_mutex_t* mutex,
                                   char* format, ...) {
  va_list args;
  int bytes_written = write_to_netdrv_close_on_error(drv, num_bytes, buffer);
  if (bytes_written <= 0) {
    // Write failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error("Failed to write to socket. Closing it.");
    }
  }
}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  char* topicName = NULL;
  int topicLen;
  MQTTClient_message* message = NULL;
  int rc;
  if ((rc = MQTTClient_receive(MQTT_priv->client, &topicName, &topicLen, &message, 1000000)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to receive message, return code %d\n", rc);
  }
  int decoded_length;
  unsigned char * decoded = base64_decode(message->payload, message->payloadlen, &decoded_length);
  if (buffer_length < decoded_length) {
    lf_print_error("Buffer to read is too short.");
    return -1;
  }
  LF_PRINT_LOG("Message received on topic %s is %.*s", topicName, message->payloadlen, (char*)(message->payload));

  memcpy(buffer, decoded, decoded_length);
  free(decoded);
  MQTTClient_free(topicName);
  MQTTClient_freeMessage(&message);
  return 1;
}

ssize_t read_from_netdrv_close_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  ssize_t bytes_read = read_from_netdrv(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    drv->close(drv);
    return -1;
  }
  return bytes_read;
}

void read_from_netdrv_fail_on_error(netdrv_t* drv, unsigned char* buffer, size_t buffer_length, lf_mutex_t* mutex,
                                    char* format, ...) {
  va_list args;
  ssize_t bytes_read = read_from_netdrv_close_on_error(drv, buffer, buffer_length);
  if (bytes_read <= 0) {
    // Read failed.
    if (mutex != NULL) {
      lf_mutex_unlock(mutex);
    }
    if (format != NULL) {
      lf_print_error_system_failure(format, args);
    } else {
      lf_print_error_system_failure("Failed to read from netdrv.");
    }
  }
}
