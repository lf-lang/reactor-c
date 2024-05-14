#include <stdlib.h>
#include <string.h>
// #include <openssl/evp.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
#include "lf_mqtt_support.h"

// #include <MQTTClient.h>

#define ADDRESS "tcp://10.218.100.147:1883"
#define QOS 2
#define TIMEOUT 10000L

static MQTT_priv_t* MQTT_priv_init();
static char* create_topic_federation_id_rti(const char* federation_id);
static char* create_topic_federation_id_fed_id_to_rti(const char* federation_id, uint16_t fed_id);
static char* create_topic_federation_id_rti_to_fed_id(const char* federation_id, uint16_t fed_id);
// static char* base64_encode(const unsigned char* input, int input_len, int* output_len);
// static unsigned char* base64_decode(const unsigned char* input, int input_len, int* output_len);
static void set_MQTTServer_id(MQTT_priv_t* MQTT_priv, int my_id, int client_id);
static void set_MQTTClient_id(MQTT_priv_t* MQTT_priv, int client_id);

/**
 * @brief
 * Initializes structure of netdrv, and priv inside.
 * Allocate memory.
 * Check lf_socket_support.c for example.
 *
 * @return netdrv_t*
 */
netdrv_t* initialize_netdrv(int federate_id, const char* federation_id) {
  netdrv_t* drv = initialize_common_netdrv(federate_id, federation_id);

  // Initialize priv.
  MQTT_priv_t* priv = MQTT_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

void close_netdrv(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  unsigned char buffer[1];
  buffer[0] = MQTT_RTI_RESIGNED;
  write_to_netdrv_fail_on_error(drv, 1, buffer, NULL,
                                "Failed to send MQTT_RTI_RESIGNED to federate %d", drv->federate_id);
  int rc;
  if ((rc = MQTTClient_disconnect(MQTT_priv->client, 10000)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to disconnect, return code %d\n", rc);
  }
  MQTTClient_destroy(&MQTT_priv->client);
}

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
int create_server(netdrv_t* drv, server_type_t server_type, uint16_t port) {
  if (server_type == RTI) {
  } // JUST TO PASS COMPILER.
  if (port == 0) {
  } // JUST TO PASS COMPILER.
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  // If RTI calls this, it will be -1. If federate server calls, it will be it's federate ID.
  set_MQTTServer_id(MQTT_priv, drv->federate_id, drv->federate_id);
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
  MQTT_priv->topic_name = (const char*)create_topic_federation_id_rti(drv->federation_id);
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
  netdrv_t* ret_netdrv = initialize_netdrv(-2, my_netdrv->federation_id);
  // MQTT_priv_t* my_priv = (MQTT_priv_t*)my_netdrv->priv;
  MQTT_priv_t* ret_priv = (MQTT_priv_t*)ret_netdrv->priv;
  unsigned char buffer[1 + sizeof(uint16_t)];
  read_from_netdrv_fail_on_error(my_netdrv, buffer, 1 + sizeof(uint16_t), NULL, "MQTT receive failed.");
  if (buffer[0] != MSG_TYPE_MQTT_JOIN) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_JOIN.");
  }
  uint16_t fed_id = extract_uint16(buffer + 1);
  ret_netdrv->federate_id = (int)fed_id;

  set_MQTTServer_id(ret_priv, my_netdrv->federate_id, ret_netdrv->federate_id);
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

  char* topic_to_subscribe = create_topic_federation_id_fed_id_to_rti(ret_netdrv->federation_id, fed_id);
  if ((rc = MQTTClient_subscribe(ret_priv->client, (const char*)topic_to_subscribe, QOS)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to subscribe, return code %d\n", rc);
  }
  LF_PRINT_LOG("Subscribing on topic %s.", topic_to_subscribe);
  free(topic_to_subscribe);

  ret_priv->topic_name = (const char*)create_topic_federation_id_rti_to_fed_id(ret_netdrv->federation_id, fed_id);
  buffer[0] = MSG_TYPE_MQTT_ACCEPT;
  encode_uint16((uint16_t)ret_netdrv->federate_id, buffer + 1);
  write_to_netdrv_fail_on_error(ret_netdrv, 1 + sizeof(uint16_t), buffer, NULL,
                                "Failed to send MSG_TYPE_MQTT_ACCEPT to federate %d", ret_netdrv->federate_id);
  read_from_netdrv_fail_on_error(ret_netdrv, buffer, 1, NULL, "MQTT receive failed.");
  if (buffer[0] != MSG_TYPE_MQTT_ACCEPT_ACK) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_ACCEPT_ACK.");
  }
  return ret_netdrv;
}

/**
 * @brief Federate connects to broker.
 *
 * @param drv
 */
void create_client(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  set_MQTTClient_id(MQTT_priv, drv->federate_id);
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
/**
 * @brief Federate publishes it's federate ID to "{Federation_ID}_RTI", then subscribes to
 * "{Federation_ID}_RTI_to_fed{fed_id}"
 *
 * @param drv
 * @return int
 */
//TODO: Suppport Decentralized
int connect_to_netdrv(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  int rc;
  MQTT_priv->topic_name = (const char*)create_topic_federation_id_rti(drv->federation_id);
  unsigned char buffer[1 + sizeof(uint16_t)];
  buffer[0] = MSG_TYPE_MQTT_JOIN;
  lf_print_log("Federate_id: %d", drv->federate_id);
  encode_uint16((uint16_t)drv->federate_id, buffer + 1);
  write_to_netdrv_fail_on_error(drv, 1 + sizeof(uint16_t), buffer, NULL,
                                "Failed to write federate_id_to RTI for connection through MQTT.");
  free((char*)MQTT_priv->topic_name);
  char* topic_to_subscribe = create_topic_federation_id_rti_to_fed_id(drv->federation_id, drv->federate_id);
  if ((rc = MQTTClient_subscribe(MQTT_priv->client, (const char*)topic_to_subscribe, QOS)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to subscribe, return code %d\n", rc);
  }
  LF_PRINT_LOG("Subscribing on topic %s.", topic_to_subscribe);
  free(topic_to_subscribe);
  MQTT_priv->topic_name = (const char*)create_topic_federation_id_fed_id_to_rti(drv->federation_id, drv->federate_id);
  read_from_netdrv_fail_on_error(drv, buffer, 1 + sizeof(uint16_t), NULL, "Failed to read MSG_TYPE_MQTT_ACCEPT.");
  if (buffer[0] != MSG_TYPE_MQTT_ACCEPT) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_ACCEPT.");
  }
  uint16_t temp_fed_id = extract_uint16(buffer + 1);
  if (drv->federate_id != temp_fed_id) {
    lf_print_error_and_exit("Wrong federate id. Received %d\n", temp_fed_id);
  }
  buffer[0] = MSG_TYPE_MQTT_ACCEPT_ACK;
  write_to_netdrv_fail_on_error(drv, 1, buffer, NULL,
                                "Failed to write MSG_TYPE_MQTT_ACCEPT_ACK_to RTI for connection through MQTT.");
  return 0;
}

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
  // pubmsg.payload = (void*)base64_encode(buffer, num_bytes, &pubmsg.payloadlen);
  pubmsg.payload = (void*)buffer;
  pubmsg.payloadlen = num_bytes;
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  // TESTING
  lf_print_log("num_bytes: %ld", num_bytes);
  // int decoded_length;
  // unsigned char* decoded = base64_decode(pubmsg.payload, pubmsg.payloadlen, &decoded_length);
  // pubmsg.payloadlen does not include null terminator. Only characters.
  if ((rc = MQTTClient_publishMessage(MQTT_priv->client, MQTT_priv->topic_name, &pubmsg, &token)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to publish message, return code %d\n", rc);
  }
  // LF_PRINT_LOG("Message publishing on topic %s is %.*s", MQTT_priv->topic_name, pubmsg.payloadlen,
  //              (char*)(pubmsg.payload));
  rc = MQTTClient_waitForCompletion(MQTT_priv->client, token, TIMEOUT);
  int bytes_written = pubmsg.payloadlen;
  return bytes_written;
}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  if (buffer_length == 0) {
  } // JUST TO PASS COMPILER.
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  char* topicName = NULL;
  int topicLen;
  MQTTClient_message* message = NULL;
  int rc;
  if ((rc = MQTTClient_receive(MQTT_priv->client, &topicName, &topicLen, &message, 1000000)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to receive message, return code %d\n", rc);
  }

  if (buffer[0] == MQTT_RTI_RESIGNED) {
    return 0;
  }
  // int decoded_length;
  // unsigned char* decoded = base64_decode(message->payload, message->payloadlen, &decoded_length);
  // lf_print_log("decoded_length: %d", decoded_length);
  // if (decoded_length == buffer_length + 1) {
  //   decoded_length--;
  //   lf_print_log("decoded_length -- : %d", decoded_length);
  // }

  // if (buffer_length < decoded_length) {
  //   lf_print_error("Buffer to read is too short.");
  //   return -1;
  // }
  // // LF_PRINT_LOG("Message received on topic %s is %.*s", topicName, message->payloadlen, (char*)(message->payload));

  // memcpy(buffer, decoded, decoded_length);
  // free(decoded);

  memcpy(buffer, (unsigned char*)message->payload, message->payloadlen);
  int bytes_read = message->payloadlen;
  MQTTClient_free(topicName);
  MQTTClient_freeMessage(&message);
  return bytes_read;
}

char* get_host_name(netdrv_t* drv) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  return NULL;
}
int32_t get_my_port(netdrv_t* drv) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  return 0;
}
int32_t get_port(netdrv_t* drv) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  return 0;
}
struct in_addr* get_ip_addr(netdrv_t* drv) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  return NULL;
}
void set_host_name(netdrv_t* drv, const char* hostname) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  if (hostname == NULL) {
  } // JUST TO PASS COMPILER.
}
void set_port(netdrv_t* drv, int port) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER
  if (port == 0) {
  } // JUST TO PASS COMPILER.
}
void set_specified_port(netdrv_t* drv, int port) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  if (port == 0) {
  } // JUST TO PASS COMPILER.
}
void set_ip_addr(netdrv_t* drv, struct in_addr ip_addr) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  if (ip_addr.s_addr == 0) {
  } // JUST TO PASS COMPILER.
}
ssize_t peek_from_netdrv(netdrv_t* drv, unsigned char* result) {
  if (drv == NULL) {
  } // JUST TO PASS COMPILER.
  if (result == NULL) {
  } // JUST TO PASS COMPILER.
  return 0;
}

void send_address_advertisement_to_RTI(netdrv_t* fed_drv, netdrv_t* rti_drv) {}

// ------------------Helper Functions------------------ //

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
// static char* base64_encode(const unsigned char* input, int input_len, int* output_len) {
//   // Calculate the maximum possible length of the Base64 encoded data
//   int max_encoded_len = (((input_len + 2) / 3) * 4) + 1; // +1 for null terminator

//   // Allocate memory for the Base64 encoded data
//   char* encoded_data = (char*)malloc(max_encoded_len);
//   if (encoded_data == NULL) {
//     *output_len = 0;
//     return NULL; // Memory allocation failed
//   }

//   // Encode the input data as Base64
//   *output_len = EVP_EncodeBlock((unsigned char*)encoded_data, input, input_len);
//   return encoded_data;
// }

// // Function to encode data as Base64 using OpenSSL's EVP_DecodeBlock()
// static unsigned char* base64_decode(const unsigned char* input, int input_len, int* output_len) {
//   // Allocate memory for the output buffer
//   unsigned char* output = (unsigned char*)malloc(input_len);
//   if (output == NULL) {
//     return NULL; // Memory allocation failed
//   }

//   // Decode the Base64 data
//   // TODO: DONGHA This can have errors, because this may add 0bit paddings.
//   *output_len = EVP_DecodeBlock(output, input, input_len);

//   return output;
// }

static void set_MQTTServer_id(MQTT_priv_t* MQTT_priv, int my_id, int client_id) {
  if (my_id == -1 && client_id == -1) {
    strcat(MQTT_priv->client_id, "RTI_RTI");
  } else if (my_id == -1) {
    sprintf(MQTT_priv->client_id, "RTI_%d", client_id);
  } else {
    sprintf(MQTT_priv->client_id, "fed%d_fed%d", my_id, client_id);
  }
}

static void set_MQTTClient_id(MQTT_priv_t* MQTT_priv, int client_id) { sprintf(MQTT_priv->client_id, "%d", client_id); }
