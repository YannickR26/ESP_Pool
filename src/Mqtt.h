#pragma once

#include <PubSubClient.h>

#include "settings.h"

class Mqtt
{
public:
	Mqtt();
	virtual ~Mqtt();

	void setup();
	void handle();
	void publish(String topic, String body);
	void log(String level, String str);
	bool isConnected() { return clientMqtt.connected(); }

private:
	String getResetReason(int cpu);
	void reconnect();
	void callback(char *topic, uint8_t *payload, unsigned int length);

	PubSubClient clientMqtt;
	String startedAt;
};

#if !defined(NO_GLOBAL_INSTANCES)
extern Mqtt MqttClient;
#endif
