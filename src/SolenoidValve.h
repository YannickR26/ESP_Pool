#pragma once

#include <Arduino.h>
#include <Ticker.h>

class SolenoidValve
{
public:
    SolenoidValve(uint8_t pinPositivePower, uint8_t pinNegativePower);
    virtual ~SolenoidValve();

    void handle();

    void setTimeout(uint16_t timeInSeconds);          // set to 0 for infinite
    void setMaxWaterQuantity(uint16_t waterQuantity); // set to 0 for no max quantity of water
    void setMaxWaterLevel(float waterLevel);          // set to 0 for no max level of water
    void open();
    void close();

protected:

private:
    bool     _isOpen;
    uint8_t  _pinPositivePower, _pinNegativePower;
    uint16_t _timeout;
    Ticker   _tickTimeout;
    uint16_t _waterQuantityMax, _waterQuantity;
    float    _waterLevelMax;
};
