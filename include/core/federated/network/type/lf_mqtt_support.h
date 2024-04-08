#ifndef LF_MQTT_SUPPORT_H
#define LF_MQTT_SUPPORT_H

#include <MQTTClient.h>

#define ADDRESS     "tcp://mqtt.eclipseprojects.io:1883"
#define QOS         2
#define TIMEOUT     10000L

typedef struct MQTT_priv_t {
  MQTTClient client;
  MQTTClient_connectOptions conn_opts; // = MQTTClient_connectOptions_initializer;
  MQTTClient_message pubmsg; //= MQTTClient_message_initializer;
  MQTTClient_deliveryToken token;
  const char* topic_name;
} MQTT_priv_t;

#endif // LF_MQTT_SUPPORT_H
