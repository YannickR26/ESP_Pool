#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20
{
public:
    DS18B20(uint8_t pin);
    virtual ~DS18B20();

    float readTemp();

protected:
private:
    OneWire _oneWire;
    DallasTemperature _sensors;
};