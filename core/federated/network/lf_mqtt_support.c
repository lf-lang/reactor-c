#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "net_common.h"
#include "net_util.h"
#include "netdriver.h"
#include "lf_mqtt_support.h"
#include "MQTTClientPersistence.h" // For logging
// #include "reactor_common.h"

// #include <MQTTClient.h>

#define ADDRESS "tcp://127.0.0.1:1883"
#define QOS 2
#define TIMEOUT 10000L

#define MAX_RETRIES 5 // Number of retry attempts
#define RETRY_DELAY 2 // Delay between attempts in seconds

extern bool _lf_termination_executed;

static MQTT_priv_t* MQTT_priv_init();
static char* create_topic_federation_id_listener_id(const char* federation_id, int listener_id);
static char* create_topic_federation_id_A_to_B(const char* federation_id, int A, int B, int flag);
static void set_MQTTClient_id(MQTT_priv_t* MQTT_priv, int my_id, int target_id, int flag);
int MQTT_connect_with_retry(MQTTClient client, MQTTClient_connectOptions* conn_opts);
// int MQTT_subscribe_with_retry(MQTTClient client, const char* topic, int qos);
int MQTT_subscribe_with_retry(MQTTClient client, const char* topic, int qos, int flag);

void log_callback(enum MQTTCLIENT_TRACE_LEVELS level, char* message) {
  (void)level; // Explicitly mark the parameter as unused
  lf_print("MQTT_LOG: %s\n", message);
}

/**
 * @brief
 * Initializes structure of netdrv, and priv inside.
 * Allocate memory.
 * Check lf_socket_support.c for example.
 *
 * @return netdrv_t*
 */
netdrv_t* initialize_netdrv(int my_federate_id, const char* federation_id) {
  netdrv_t* drv = initialize_common_netdrv(my_federate_id, federation_id);

  // Initialize priv.
  MQTT_priv_t* priv = MQTT_priv_init();

  // Set drv->priv pointer to point the malloc'd priv.
  drv->priv = (void*)priv;
  return drv;
}

void close_netdrv(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  // If target_id is available (not -2, which means the listener netdriver), it sends a MQTT_RESIGNED messaged.
  if (MQTT_priv->target_id != -2) {
    unsigned char buffer[1];
    buffer[0] = MQTT_RESIGNED;
    write_to_netdrv_fail_on_error(drv, 1, buffer, NULL, "Failed to send MQTT_RESIGNED message.");
    LF_PRINT_DEBUG("Sending MQTT_RESIGNED message.");
  }
  int rc;
  if ((rc = MQTTClient_disconnect(MQTT_priv->client, 10000)) != MQTTCLIENT_SUCCESS) {
    lf_print("Failed to disconnect, return code %d.", rc);
  }
  MQTTClient_destroy(&MQTT_priv->client);
  if (MQTT_priv->topic_name_to_send) {
    free(MQTT_priv->topic_name_to_send);
  }
  free(MQTT_priv);
  drv->priv = NULL;
  free(drv);
  drv = NULL;
}

/**
 * @brief Create a server object
 * Initializes MQTT client, and connects to broker.
 * RTI subscribes “{Federation_ID}_RTI” topic. This should be done here, because the establish_communication_session
 * loops. Check socket_common.c for example. It is a common function because also lf_sst_support.c will also use it.
 * @param drv
 * @param server_type
 * @param port The port is NULL here.
 * @return int
 */
int create_listener(netdrv_t* drv, server_type_t server_type, uint16_t port) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  if (server_type == RTI) {
  } // JUST TO PASS COMPILER.
  if (port == 0) {
  } // JUST TO PASS COMPILER.

  // Target is not available for listeners. We set it to -2 if it is uninitialized or unavailable. This is used when
  // close_netdrv() is called.
  MQTT_priv->target_id = -2;
  // If RTI calls this, it will be -1. If federate server calls, it will be it's federate ID.
  set_MQTTClient_id(MQTT_priv, drv->my_federate_id, drv->my_federate_id, -1);

  int rc;

  if ((rc = MQTTClient_create(&MQTT_priv->client, ADDRESS, MQTT_priv->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to create client during create_listener(), return code %d.", rc);
  }
  LF_PRINT_DEBUG("Connecting MQTTClient %s to broker.", MQTT_priv->client_id);
  LF_MUTEX_LOCK(&netdrv_mutex);
  if ((rc = MQTT_connect_with_retry(MQTT_priv->client, &MQTT_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
    MQTTClient_destroy(&MQTT_priv->client);
    lf_print_error_and_exit(
        "Failed to connect during create_listener(), return code %d. Check if MQTT broker is available.", rc);
  }
  LF_PRINT_DEBUG("Connected MQTTClient %s to broker, return code %d.", MQTT_priv->client_id, rc);
  MQTT_priv->topic_name_to_send = create_topic_federation_id_listener_id(drv->federation_id, drv->my_federate_id);
  LF_PRINT_DEBUG("create_listener(): MQTTClient %s Subscribing on topic %s.", MQTT_priv->client_id,
                 MQTT_priv->topic_name_to_send);
  if ((rc = MQTT_subscribe_with_retry(MQTT_priv->client, MQTT_priv->topic_name_to_send, QOS, 1)) !=
      MQTTCLIENT_SUCCESS) {
    LF_PRINT_DEBUG("create_listener(): Disconnecting MQTTClient %s.", MQTT_priv->client_id);
    MQTTClient_disconnect(MQTT_priv->client, TIMEOUT);
    MQTTClient_destroy(&MQTT_priv->client);
    lf_print_error_and_exit("Failed to subscribe during create_listener(), return code %d.", rc);
  }
  LF_PRINT_DEBUG("Subscribed on topic %s, return code %d.", MQTT_priv->topic_name_to_send, rc);
  LF_MUTEX_UNLOCK(&netdrv_mutex);
  return 1;
}

/**
 * @brief
 * This function returns a connector netdriver using the listener netdriver.
 * 1. Each federate publishes fed_id to {Federation_ID}_{listenerID}
 * 2. fed_{n} subscribes to “{Federation_Id}_{listenerID}_to_fed_{n}”.
 * 3. Listener subscribes to “{Federation_Id}_fed_{n}_to_{listenerID}”.
 * Check lf_socket_support.c for example.

 * @param netdrv
 * @return netdrv_t*
 */
netdrv_t* establish_communication_session(netdrv_t* listener_netdrv) {
  // Create connector netdriver which will communicate with the connector.
  netdrv_t* connector_nedrv = initialize_netdrv(-2, listener_netdrv->federation_id);
  MQTT_priv_t* connector_priv = (MQTT_priv_t*)connector_nedrv->priv;
  connector_nedrv->my_federate_id = listener_netdrv->my_federate_id;

  // // Set the trace level to maximum verbosity
  // MQTTClient_setTraceLevel(MQTTCLIENT_TRACE_MAXIMUM);
  // // Set the trace callback function
  // MQTTClient_setTraceCallback(log_callback);

  int rc;
  unsigned char buffer[1 + sizeof(uint16_t)];
  char* topic_to_subscribe = NULL;
  // Step1: The listener first waits for a MSG_TYPE_MQTT_JOIN message, through the topic federationID_listenerID.

  read_from_netdrv_fail_on_error(listener_netdrv, buffer, 1 + sizeof(uint16_t), NULL, "MQTT receive failed.");
  if (buffer[0] != MSG_TYPE_MQTT_JOIN) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_JOIN.");
  }
  uint16_t target_fed_id = extract_uint16(buffer + 1);
  LF_PRINT_LOG("Received MSG_TYPE_MQTT_JOIN message from federate %d.", target_fed_id);

  // The connector netdriver connects to the broker.
  connector_priv->target_id = (int)target_fed_id;
  LF_PRINT_DEBUG("Setting up MQTTClient_id to target federate %d.", connector_priv->target_id);
  // If RTI calls this, it will be RTI_targetfedID. If federate calls this, it will be myfedID_targetfedID
  set_MQTTClient_id(connector_priv, connector_nedrv->my_federate_id, connector_priv->target_id, -1);
  LF_PRINT_DEBUG("Setup MQTTClient_id to target federate %d as %s.", connector_priv->target_id,
                 connector_priv->client_id);

  LF_PRINT_DEBUG("Creating topic to target federate %d.", connector_priv->target_id);
  // Subscribe to topic: federationID_fedID_to_listenerID
  // When centralized, this will be federationID_fedID_to_RTI
  // When decentralized, this will be federationID_CONN_{targetfedID}_to_LIST_{myfedID}
  // This is the channel where the federate sends messages to the listener.
  topic_to_subscribe = create_topic_federation_id_A_to_B(connector_nedrv->federation_id, connector_priv->target_id,
                                                         connector_nedrv->my_federate_id, 1);

  LF_PRINT_DEBUG("Creating MQTTClient to target federate %d.", connector_priv->target_id);
  if ((rc = MQTTClient_create(&connector_priv->client, ADDRESS, connector_priv->client_id, MQTTCLIENT_PERSISTENCE_NONE,
                              NULL)) != MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to create client during establish_communication_session(), return code %d.", rc);
  }

  LF_MUTEX_LOCK(&netdrv_mutex);

  instant_t start_connect = lf_time_physical();
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_error_and_exit("Failed to connect and subscribe to topic %s with timeout: " PRINTF_TIME ". Giving up.",
                              topic_to_subscribe, CONNECT_TIMEOUT);
      break;
    }
    LF_PRINT_DEBUG("Connecting MQTTClient %s to broker.", connector_priv->client_id);
    if ((rc = MQTTClient_connect(connector_priv->client, &connector_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
      // MQTTClient_destroy(&connector_priv->client);
      lf_print_warning("Failed to connect during establish_communication_session(), return code %d.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    LF_PRINT_DEBUG("Connected MQTTClient %s to broker, return code %d.", connector_priv->client_id, rc);
    LF_PRINT_DEBUG("establish_communication_session(): MQTTClient %s Subscribing on topic %s.",
                   connector_priv->client_id, topic_to_subscribe);

    // The MQTTClient_subscribe() internally disconnects the MQTTClient to the broker. So, it should retry after
    // re-connecting the MQTTClient first.
    if ((rc = MQTTClient_subscribe(connector_priv->client, (const char*)topic_to_subscribe, QOS)) !=
        MQTTCLIENT_SUCCESS) {
      // LF_PRINT_DEBUG("establish_communication_session(): Disconnecting MQTTClient %s.", connector_priv->client_id);
      // MQTTClient_disconnect(connector_priv->client, TIMEOUT);
      // MQTTClient_destroy(&connector_priv->client);
      lf_print_warning("Failed to subscribe during establish_communication_session(), return code %d.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    LF_PRINT_DEBUG("Subscribed on topic %s, return code %d.", topic_to_subscribe, rc);
    break;
  }

  LF_MUTEX_UNLOCK(&netdrv_mutex);
  // Step2: The listener sends a MSG_TYPE_MQTT_ACCEPT message to the federate.
  // Publish to topic: federationID_listenerID_to_fedID
  // When centralized, this will be federationID_RTI_to_fedID
  // When decentralized, this will be federateionID_LIST_{myfedID}_to_CONN_{targetfedID}
  connector_priv->topic_name_to_send = create_topic_federation_id_A_to_B(
      connector_nedrv->federation_id, connector_nedrv->my_federate_id, connector_priv->target_id, -1);
  buffer[0] = MSG_TYPE_MQTT_ACCEPT;
  encode_uint16((uint16_t)connector_priv->target_id, buffer + 1);
  LF_PRINT_LOG("Publishing MSG_TYPE_MQTT_ACCEPT message on topic %s.", connector_priv->topic_name_to_send);
  write_to_netdrv_fail_on_error(connector_nedrv, 1 + sizeof(uint16_t), buffer, NULL,
                                "Failed to send MSG_TYPE_MQTT_ACCEPT to federate %d", connector_priv->target_id);

  // Step3: The listner receives the MSG_TYPE_MQTT_ACCEPT_ACK message from the federate.
  read_from_netdrv_fail_on_error(connector_nedrv, buffer, 1, NULL, "MQTT receive failed.");
  if (buffer[0] != MSG_TYPE_MQTT_ACCEPT_ACK) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_ACCEPT_ACK.");
  }
  LF_PRINT_LOG("Receiving MSG_TYPE_MQTT_ACCEPT_ACK message on topic %s.", topic_to_subscribe);
  free(topic_to_subscribe);
  return connector_nedrv;
}

/**
 * @brief Federate connects to broker.
 *
 * @param drv
 */
void create_connector(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  // Only federates call this. It will be myfedID_RTI or myfedID_targetfedID.
  set_MQTTClient_id(MQTT_priv, drv->my_federate_id, MQTT_priv->target_id, 1);
  int rc;
  if ((rc = MQTTClient_create(&MQTT_priv->client, ADDRESS, MQTT_priv->client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error_and_exit("Failed to create client, return code %d.", rc);
  }
  // LF_MUTEX_LOCK(&netdrv_mutex);
  // LF_PRINT_DEBUG("Connecting MQTTClient %s to broker.", MQTT_priv->client_id);
  // if ((rc = MQTT_connect_with_retry(MQTT_priv->client, &MQTT_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
  //   MQTTClient_destroy(&MQTT_priv->client);
  //   lf_print_error_and_exit(
  //       "Failed to connect during create_connector(), return code %d. Check if MQTT broker is available.", rc);
  // }
  // LF_PRINT_DEBUG("Connected MQTTClient %s to broker, return code %d.", MQTT_priv->client_id, rc);
  // LF_MUTEX_UNLOCK(&netdrv_mutex);
}
/**
 * @brief Federate publishes it's federate ID to "{Federation_ID}_RTI", then subscribes to
 * "{Federation_ID}_RTI_to_fed{fed_id}"
 *
 * @param drv
 * @return int
 */
int connect_to_netdrv(netdrv_t* drv) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  int rc;

  // Subscribe to topic: federationID_listenerID_to_fedID
  // This should be done before sending the MSG_TYPE_MQTT_JOIN message, because the listener publishing the
  // MSG_TYPE_MQTT_ACCEPT message at topic federationID_listenerID_to_fedID can be faster than the connector subscribing
  // to the topic.
  // When centralized, this will be federationID_RTI_to_fedID
  // When decentralized, this will be federateionID_LIST_{targetfedID}_to_CONN_{myfedID}
  char* topic_to_subscribe =
      create_topic_federation_id_A_to_B(drv->federation_id, MQTT_priv->target_id, drv->my_federate_id, -1);

  LF_MUTEX_LOCK(&netdrv_mutex);
  instant_t start_connect = lf_time_physical();
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_error_and_exit("Failed to connect and subscribe to topic %s with timeout: " PRINTF_TIME ". Giving up.",
                              topic_to_subscribe, CONNECT_TIMEOUT);
      break;
    }
    LF_PRINT_DEBUG("Connecting MQTTClient %s to broker.", MQTT_priv->client_id);
    if ((rc = MQTTClient_connect(MQTT_priv->client, &MQTT_priv->conn_opts)) != MQTTCLIENT_SUCCESS) {
      // MQTTClient_destroy(&MQTT_priv->client);
      lf_print_warning(
          "Failed to connect during create_connector(), return code %d. Check if MQTT broker is available.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    LF_PRINT_DEBUG("Connected MQTTClient %s to broker, return code %d.", MQTT_priv->client_id, rc);
    LF_PRINT_DEBUG("connect_to_netdrv(): MQTTClient %s Subscribing on topic %s.", MQTT_priv->client_id,
                   topic_to_subscribe);
    if ((rc = MQTTClient_subscribe(MQTT_priv->client, (const char*)topic_to_subscribe, QOS)) != MQTTCLIENT_SUCCESS) {
      // LF_PRINT_DEBUG("connect_to_netdrv(): Disconnecting MQTTClient %s.", MQTT_priv->client_id);
      // MQTTClient_disconnect(MQTT_priv->client, TIMEOUT);
      // MQTTClient_destroy(&MQTT_priv->client);
      // free(topic_to_subscribe);
      lf_print_warning("Failed to subscribe during connect_to_netdrv(), return code %d.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    LF_PRINT_DEBUG("Subscribed on topic %s, return code %d.", topic_to_subscribe, rc);
    break;
  }
  LF_MUTEX_UNLOCK(&netdrv_mutex);
  // Step1: The federate sends a MSG_TYPE_MQTT_JOIN message including it's federateID to the listener.
  // Publish to topic: federationID_listenerID
  MQTT_priv->topic_name_to_send = create_topic_federation_id_listener_id(drv->federation_id, MQTT_priv->target_id);
  unsigned char buffer[1 + sizeof(uint16_t)];
  buffer[0] = MSG_TYPE_MQTT_JOIN;
  encode_uint16((uint16_t)drv->my_federate_id, buffer + 1);

  // The connect_to_netdrv() can be called in the federates before the establish_communication_session() is called in
  // the RTI. The MQTT QOS2 ensures the message to arrive to the subscribed client, but this can be called even before
  // the netdriver was initialized in the RTI side. Thus, the federate must retry sending messages to the RTI until it
  // replies the MSG_TYPE_MQTT_ACCEPT message. The connect retry interval (500 msecs) should be shorter than the read
  // timeout time (which is 10 secs), thus it does not use read_from_netdrv().
  start_connect = lf_time_physical();
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_error("Failed to handshake with target with timeout: " PRINTF_TIME ". Giving up.", CONNECT_TIMEOUT);
      return -1;
    }
    LF_PRINT_LOG("Publishing MSG_TYPE_MQTT_JOIN message with federateID %d on topic %s.", drv->my_federate_id,
                 MQTT_priv->topic_name_to_send);
    write_to_netdrv_fail_on_error(drv, 1 + sizeof(uint16_t), buffer, NULL,
                                  "Failed to write my_federate_id to listener for connection through MQTT.");

    // Step2: Receive MSG_TYPE_MQTT_ACCEPT from listener, which sends the connector's federateID.
    // Receive from topic: federationID_listenerID_to_fedID
    char* topicName = NULL;
    int topicLen;
    int temp_rc;
    MQTTClient_message* message = NULL;
    temp_rc = MQTTClient_receive(MQTT_priv->client, &topicName, &topicLen, &message, 500);
    if (temp_rc != MQTTCLIENT_SUCCESS) {
      lf_print_error("Failed to receive MSG_TYPE_MQTT_ACCEPT message, return code %d.", temp_rc);
      if (topicName) {
        MQTTClient_free(topicName);
      }
      if (message) {
        MQTTClient_freeMessage(&message);
      }
      lf_sleep(MSEC(2000));
      continue;
    } else if (message == NULL) {
      // This means the call succeeded but no message was received within the timeout
      lf_print_log("No message received within the timeout period.");
      if (topicName) {
        MQTTClient_free(topicName);
      }
      lf_sleep(MSEC(2000));
      continue;
    } else {
      // Successfully received a message
      lf_print_log("Successfully received MSG_TYPE_MQTT_ACCEPT message, return code %d.", temp_rc);
      memcpy(buffer, (unsigned char*)message->payload, message->payloadlen);
      if (topicName) {
        MQTTClient_free(topicName);
      }
      if (message) {
        MQTTClient_freeMessage(&message);
      }
      break;
    }
  }
  if (buffer[0] != MSG_TYPE_MQTT_ACCEPT) {
    lf_print_error_and_exit("Wrong message type... Expected MSG_TYPE_MQTT_ACCEPT.");
  }
  LF_PRINT_LOG("Receiving MSG_TYPE_MQTT_ACCEPT message on topic %s.", topic_to_subscribe);
  free((char*)MQTT_priv->topic_name_to_send);
  free(topic_to_subscribe);

  // Compare the received federateID with my federateID.
  uint16_t temp_fed_id = extract_uint16(buffer + 1);
  if (drv->my_federate_id != temp_fed_id) {
    lf_print_error_and_exit("Wrong federate ID. Received %d", temp_fed_id);
  }

  // Step3: Send MSG_TYPE_MQTT_ACCEPT_ACK message to the listener.
  // Publish to topic: federationID_fedID_to_listenorID
  // When centralized, this will be federationID_fedID_to_RTI
  // When decentralized, this will be federationID_CONN_{myfedID}_to_LIST_{targetfedID}
  MQTT_priv->topic_name_to_send =
      create_topic_federation_id_A_to_B(drv->federation_id, drv->my_federate_id, MQTT_priv->target_id, 1);
  buffer[0] = MSG_TYPE_MQTT_ACCEPT_ACK;
  write_to_netdrv_fail_on_error(drv, 1, buffer, NULL,
                                "Failed to write MSG_TYPE_MQTT_ACCEPT_ACK_to RTI for connection through MQTT.");
  LF_PRINT_LOG("Publishing MSG_TYPE_MQTT_ACCEPT_ACK_to message on topic %s.", MQTT_priv->topic_name_to_send);
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
  pubmsg.payload = (void*)buffer;
  pubmsg.payloadlen = num_bytes;
  pubmsg.qos = QOS;
  pubmsg.retained = 0;

  if ((rc = MQTTClient_publishMessage(MQTT_priv->client, MQTT_priv->topic_name_to_send, &pubmsg, &token)) !=
      MQTTCLIENT_SUCCESS) {
    lf_print_error("Failed to publish message, return code %d.", rc);
    return rc;
  }
  // LF_PRINT_DEBUG("Message publishing on topic %s is %.*s", MQTT_priv->topic_name_to_send, pubmsg.payloadlen,
  //              (char*)(pubmsg.payload));
  if ((rc = MQTTClient_waitForCompletion(MQTT_priv->client, token, TIMEOUT)) != MQTTCLIENT_SUCCESS) {
    lf_print_error("Failed to complete publish message, return code %d.", rc);
    return rc;
  }
  int bytes_written = pubmsg.payloadlen;
  return bytes_written;
}

ssize_t read_from_netdrv(netdrv_t* drv, unsigned char* buffer, size_t buffer_length) {
  if (drv == NULL) {
    lf_print_warning("Netdriver is closed, returning -1.");
    return -1;
  }
  if (buffer_length == 0) {
  } // JUST TO PASS COMPILER.

  char* topicName = NULL;
  int topicLen = 0;
  MQTTClient_message* message = NULL;
  int rc;
  int bytes_read = 0;
  // LF_PRINT_LOG("RECEIVING message from federateID %d", MQTT_priv->target_id);
  instant_t start_receive = lf_time_physical();
  // If the netdrv was closed from the outside during termination, segmentation faults happen. When federate executes
  // and calls lf_terminate_execution(), closing inbound and outbound netdrivers are first before waiting for the
  // threads to join. In this case, the federates may not receive the MQTT_resign message and just get closed. The
  // close_outbound_netdrv() and close_inbound_netdrv() functions hold a mutex lock, so this just needs to check the
  // _lf_termination_executed status befor restarting the loop.
  while (!_lf_termination_executed) {
    if (CHECK_TIMEOUT(start_receive, CONNECT_TIMEOUT)) {
      lf_print_error("Failed to receive with timeout: " PRINTF_TIME ". Giving up.", CONNECT_TIMEOUT);
      // bytes_read = -1;
      break;
    }
    MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
    rc = MQTTClient_receive(MQTT_priv->client, &topicName, &topicLen, &message, 5000);
    if (rc != MQTTCLIENT_SUCCESS) {
      lf_print_warning("Failed to receive message, return code %d.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    } else if (message == NULL) {
      // This means the call succeeded but no message was received within the timeout
      lf_print_log("No message received within the MQTTClient_receive() timeout period.");
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    } else {
      // Successfully received a message
      lf_print_log("Successfully received message, return code %d.", rc);
      // TODO: NEED to add compare with buffer_length. Also actions.
      memcpy(buffer, (unsigned char*)message->payload, message->payloadlen);
      bytes_read = message->payloadlen;
      // LF_PRINT_LOG("RECEIVED message from federateID %d", MQTT_priv->target_id);
      if (buffer[0] == MQTT_RESIGNED) {
        LF_PRINT_LOG("Received MQTT_RESIGNED message from federateID %d", MQTT_priv->target_id);
        bytes_read = 0;
      }
      break;
    }
  }
  if (topicName) {
    MQTTClient_free(topicName);
  }
  if (message) {
    MQTTClient_freeMessage(&message);
  }
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

void set_target_id(netdrv_t* drv, int federate_id) {
  MQTT_priv_t* MQTT_priv = (MQTT_priv_t*)drv->priv;
  MQTT_priv->target_id = federate_id;
}

// ------------------Helper Functions------------------ //

static MQTT_priv_t* MQTT_priv_init() {
  MQTT_priv_t* MQTT_priv = malloc(sizeof(MQTT_priv_t));
  if (!MQTT_priv) {
    lf_print_error_and_exit("Falied to malloc MQTT_priv_t.");
  }
  memset(MQTT_priv, 0, sizeof(MQTT_priv_t));
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  memcpy(&MQTT_priv->conn_opts, &conn_opts, sizeof(MQTTClient_connectOptions));
  MQTT_priv->conn_opts.keepAliveInterval = MQTTkeepAliveInterval;
  MQTT_priv->conn_opts.cleansession = MQTTcleansession;
  MQTT_priv->conn_opts.connectTimeout = MQTTconnectTimeout;
  return MQTT_priv;
}

static char* create_topic_federation_id_listener_id(const char* federation_id, int listener_id) {
  int max_length;
  // Determine the maximum length of the resulting string
  if (listener_id == -2) {
    lf_print_error_and_exit("The ID used for the MQTT topic is not initalized.");
    return NULL;
  }
  if (listener_id == -1) {
    max_length = snprintf(NULL, 0, "%s_RTI", federation_id) + 1; // +1 for null terminator
  } else {
    max_length = snprintf(NULL, 0, "%s_fed_%d", federation_id, listener_id) + 1; // +1 for null terminator
  }
  // Allocate memory for the resulting string
  char* result = (char*)malloc(max_length);
  if (result == NULL) {
    lf_print_error_and_exit("Falied to malloc.");
    return NULL;
  }

  // Format the string using snprintf
  if (listener_id == -1) {
    snprintf(result, max_length, "%s_RTI", federation_id);
  } else {
    snprintf(result, max_length, "%s_fed_%d", federation_id, listener_id);
  }
  return result;
}

static char* create_topic_federation_id_A_to_B(const char* federation_id, int A, int B, int flag) {
  int max_length = 0;
  char* result;
  if (A == -2 || B == -2) {
    lf_print_error_and_exit("The ID used for the MQTT topic is not initalized.");
    return NULL;
  }
  // Determine the maximum length of the resulting string
  // If either the ID includes -1, then the flag does not matter.
  if (A == -1) {
    max_length = snprintf(NULL, 0, "%s_RTI_to_fed_%d", federation_id, B) + 1; // +1 for null terminator
  } else if (B == -1) {
    max_length = snprintf(NULL, 0, "%s_fed_%d_to_RTI", federation_id, A) + 1; // +1 for null terminator
  }

  else if (flag == 1) {
    max_length = snprintf(NULL, 0, "%s_CONN_fed_%d_to_LIST_fed_%d", federation_id, A, B) + 1; // +1 for null terminator
  } else if (flag == -1) {
    max_length = snprintf(NULL, 0, "%s_LIST_fed_%d_to_CONN_fed_%d", federation_id, A, B) + 1; // +1 for null terminator
  }

  // Allocate memory for the resulting string
  result = (char*)malloc(max_length);
  if (result == NULL) {
    lf_print_error_and_exit("Failed to malloc.");
    return NULL;
  }

  // Format the string using snprintf
  if (A == -1) {
    snprintf(result, max_length, "%s_RTI_to_fed_%d", federation_id, B);
  } else if (B == -1) {
    snprintf(result, max_length, "%s_fed_%d_to_RTI", federation_id, A);
  } else if (flag == 1) {
    snprintf(result, max_length, "%s_CONN_fed_%d_to_LIST_fed_%d", federation_id, A, B);
  } else if (flag == -1) {
    snprintf(result, max_length, "%s_LIST_fed_%d_to_CONN_fed_%d", federation_id, A, B);
  }
  return result;
}

static void set_MQTTClient_id(MQTT_priv_t* MQTT_priv, int my_id, int target_id, int flag) {
  // When the flag is 0, it is for centralized communication.

  if (my_id == -1 && target_id == -1) {
    strcat(MQTT_priv->client_id, "RTI_RTI");
  } else if (my_id == -1) {
    sprintf(MQTT_priv->client_id, "RTI_fed_%d", target_id);
  } else if (target_id == -1) {
    sprintf(MQTT_priv->client_id, "fed_%d_RTI", my_id);
  }

  // When the flag is 1, it means
  else if (flag == 1) {
    sprintf(MQTT_priv->client_id, "CONN_fed_%d_LIST_fed_%d", my_id, target_id);
  } else if (flag == -1) {
    sprintf(MQTT_priv->client_id, "LIST_fed_%d_CONN_fed_%d", my_id, target_id);
  }
}

int MQTT_connect_with_retry(MQTTClient client, MQTTClient_connectOptions* conn_opts) {
  int rc = -1;
  instant_t start_connect = lf_time_physical();
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_warning("Failed to connect with timeout: " PRINTF_TIME ". Giving up.", CONNECT_TIMEOUT);
      break;
    }
    if ((rc = MQTTClient_connect(client, conn_opts)) != MQTTCLIENT_SUCCESS) {
      lf_print_warning("Failed to connect, return code %d. Retrying to connect.", rc);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    break;
  }
  return rc;
}

// int MQTT_subscribe_with_retry(MQTTClient client, const char* topic, int qos) {
int MQTT_subscribe_with_retry(MQTTClient client, const char* topic, int qos, int flag) {
  int rc = -1;
  instant_t start_connect = lf_time_physical();
  while (1) {
    if (CHECK_TIMEOUT(start_connect, CONNECT_TIMEOUT)) {
      lf_print_warning("Failed to subscribe with timeout: " PRINTF_TIME ". Giving up.", CONNECT_TIMEOUT);
      break;
    }
    if ((rc = MQTTClient_subscribe(client, topic, qos)) != MQTTCLIENT_SUCCESS) {
      // lf_print_warning("Failed to subscribe, return code %d. Retrying to subscribe.", rc);
      lf_print_warning("Failed to subscribe, return code %d. Retrying to subscribe. Flag: %d", rc, flag);
      lf_sleep(CONNECT_RETRY_INTERVAL);
      continue;
    }
    break;
  }
  return rc;
}
