/*
 *  SensorAM2301.h
 *
 *  Created on: 09/05/2021
 *
 *  Author : Yannick RICHARDOT (yannick.richardot@carbonbee.fr)
 */

#pragma once

#include <DHTesp.h>

class SensorAM2301
{
public:
    explicit SensorAM2301(uint8_t pin)
    {
        _sensor.setup(pin, DHTesp::DHT22);
    }

    virtual ~SensorAM2301() {}

    float readTemp()
    {
        return _sensor.getTemperature();
    }

    float readHumidity()
    {
        return _sensor.getHumidity();
    }

    const char* getStatus()
    {
        return _sensor.getStatusString();
    }

private:
    DHTesp _sensor;
};
