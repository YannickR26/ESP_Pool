#include "SensorAM2301.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
SensorAM2301::SensorAM2301(uint8_t pin)
{
    _sensor.setup(pin, DHTesp::DHT11);
}

SensorAM2301::~SensorAM2301()
{
}

float SensorAM2301::readTemp()
{
    // Send the command to get temperatures
    return _sensor.getTemperature();
}

float SensorAM2301::readHumidity()
{
    return _sensor.getHumidity();
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/