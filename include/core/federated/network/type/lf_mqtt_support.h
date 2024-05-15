#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <MQTTClient.h>

#define MQTT_RTI_RESIGNED 88

typedef struct MQTT_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  const char* topic_name;
  char client_id[20];
  uint16_t target_id;
} MQTT_priv_t;

#endif // LF_MQTT_SUPPORT_H
