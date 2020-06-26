#pragma once

#include <DHTesp.h>

class SensorAM2301
{
public:
    SensorAM2301(uint8_t pin);
    virtual ~SensorAM2301();

    float readTemp();
    float readHumidity();
    const char *getStatus();

protected:
private:
    DHTesp _sensor;
};