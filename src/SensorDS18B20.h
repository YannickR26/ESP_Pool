#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

class SensorDS18B20
{
public:
    SensorDS18B20(uint8_t pin);
    virtual ~SensorDS18B20();

    float readTemp();

protected:
private:
    OneWire _oneWire;
    DallasTemperature _sensor;
};