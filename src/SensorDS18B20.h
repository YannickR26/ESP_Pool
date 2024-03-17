/*
 *  SensorDS18B20.h
 *
 *  Created on: 10/04/2021
 *
 *  Author : Yannick RICHARDOT (yannick.richardot@carbonbee.fr)
 */

#pragma once

#include <OneWire.h>
#include <DallasTemperature.h>

class SensorDS18B20
{
public:
    explicit SensorDS18B20(uint8_t pin) :
        _oneWire(pin), _sensor(&_oneWire)
    {
        _sensor.begin();
        _sensor.setResolution(12);
    }

    virtual ~SensorDS18B20() {}

    float readTemp()
    {
        float         temp;
        unsigned long timeout = millis() + 2000;

        do
        {
            // Send the command to get temperatures
            _sensor.requestTemperatures();
            delay(200);
            temp = _sensor.getTempCByIndex(0);
            delay(50);
        } while ((temp == DEVICE_DISCONNECTED_C) && (millis() < timeout));

        if (temp == DEVICE_DISCONNECTED_C)
        {
            return DEVICE_DISCONNECTED_C;
        }

        return temp + 0.5f;
    }

private:
    OneWire           _oneWire;
    DallasTemperature _sensor;
};
