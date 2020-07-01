#pragma once

#include <ArduinoJson.h>
#include "settings.h"

class JsonConfiguration
{
public:
	JsonConfiguration();
	virtual ~JsonConfiguration();

	void setup();

	bool readConfig();
	bool saveConfig();

	void restoreDefault();

	void encodeToJson(JsonDocument &_json);
	uint8_t decodeJsonFromFile(const char *input);

	/* Members */
	String _hostname;
	String _mqttIpServer;
	uint16_t _mqttPortServer;
	uint16_t _timeSaveData;
	uint16_t _timeSendData;
	uint32_t _waterQtyA, _waterQtyB;
	uint16_t _rollerShutterTimeout;
	uint16_t _solenoidValveTimeout;
	uint16_t _solenoidValveMaxWaterQty;

private:
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
