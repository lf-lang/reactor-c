#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <stdint.h>
#include <MQTTClient.h>

#define MQTTkeepAliveInterval 20
#define MQTTcleansession 1
#define MQTTconnectTimeout 2

#define MQTT_RESIGNED 88

typedef struct MQTT_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  char* topic_name_to_send;
  char client_id[32];
  int target_id; // Must be int. Not uint_16_t. -1 stands for RTI, -2 means uninitialized.
} MQTT_priv_t;

#endif // LF_MQTT_SUPPORT_H
