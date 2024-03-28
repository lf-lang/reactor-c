#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <MQTTClient.h>

#include "net_util.h"

typedef struct mqtt_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  MQTTClient_message pubmsg; //= MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  const char* topic_name;
} mqtt_priv_t;

#endif // LF_MQTT_SUPPORT_H
