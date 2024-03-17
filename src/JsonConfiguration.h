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

    void    encodeToJson(JsonDocument& _json);
    uint8_t decodeJsonFromFile(const char* input);

    /* Members */
    String   _hostname;
    String   _mqttIpServer;
    uint16_t _mqttPortServer;
    uint16_t _timeSaveData;
    uint16_t _timeSendData;
    float    _waterQtyA, _waterQtyB;
    float    _waterLevel, _waterTemp;
    float    _intTemp, _extTemp;
    float    _intHumidity, _extHumidity;
    float    _rollerShutterDuration;
    float    _rollerShutterPosition;
    uint16_t _solenoidValveTimeout;
    uint16_t _solenoidValveMaxWaterQty;
    float    _solenoidValveMaxWaterLevel;
    bool     _pumpModeAuto;
    uint8_t  _pumpStartHours, _pumpStartMinutes;
    uint8_t  _pumpStopHours, _pumpStopMinutes;
    uint16_t _lampTimeout;
    uint16_t _lightFading;

private:
};

#if !defined(NO_GLOBAL_INSTANCES)
extern JsonConfiguration Configuration;
#endif
