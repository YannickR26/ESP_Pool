#include "DS18B20.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
DS18B20::DS18B20(uint8_t pin)
{
    _oneWire = OneWire(pin);
    _sensors = DallasTemperature(&_oneWire);
    _sensors.begin();
}

DS18B20::~DS18B20()
{
}

float DS18B20::readTemp()
{
    // Send the command to get temperatures
    _sensors.requestTemperatures();

    return _sensors.getTempCByIndex(0);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/