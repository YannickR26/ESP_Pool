#include "SensorDS18B20.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
SensorDS18B20::SensorDS18B20(uint8_t pin)
{
    _oneWire = OneWire(pin);
    _sensor = DallasTemperature(&_oneWire);
    _sensor.begin();
}

SensorDS18B20::~SensorDS18B20()
{
}

float SensorDS18B20::readTemp()
{
    // Send the command to get temperatures
    _sensor.requestTemperatures();

    return _sensor.getTempCByIndex(0);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/