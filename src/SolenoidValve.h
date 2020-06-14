#pragma once

#include <Arduino.h>

class SolenoidValve
{
public:
    SolenoidValve(uint8_t pinPositivePower, uint8_t pinNegativePower);
    virtual ~SolenoidValve();

    void open();
    void close();

protected:
private:
    uint8_t _pinPositivePower, _pinNegativePower;
};