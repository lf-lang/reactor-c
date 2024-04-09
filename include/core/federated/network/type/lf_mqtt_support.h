#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <MQTTClient.h>

typedef struct MQTT_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  const char* topic_name;
  char client_id[20];
} MQTT_priv_t;

#endif // LF_MQTT_SUPPORT_H
