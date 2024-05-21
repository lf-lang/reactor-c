#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <stdint.h>
#include <MQTTClient.h>

#define MQTTkeepAliveInterval 20
#define MQTTcleansession 1

#define MQTT_RESIGNED 88

typedef struct MQTT_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  const char* topic_name;
  char client_id[20];
  int target_id; // Must be int. Not uint_16_t. -1 stands for RTI, -2 means uninitialized.
} MQTT_priv_t;

#endif // LF_MQTT_SUPPORT_H
