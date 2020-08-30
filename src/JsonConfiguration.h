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
	float _waterQtyA, _waterQtyB;
	float _waterLevel, _waterTemp;
	float _intTemp, _extTemp;
	float _intHumidity, _extHumidity;
	uint16_t _rollerShutterTimeout;
	uint16_t _solenoidValveTimeout;
	uint16_t _solenoidValveMaxWaterQty;
	float _solenoidValveMaxWaterLevel;
	uint16_t _pumpTimeout;
	uint16_t _lampTimeout;
	uint16_t _lightFading;

private:
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
