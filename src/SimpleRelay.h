#pragma once

#include <Arduino.h>
#include <Ticker.h>

class SimpleRelay
{
public:
    SimpleRelay(uint8_t pinRelay, const char* name);
    virtual ~SimpleRelay();

    void setTimeout(uint16_t timeInSeconds); // set to 0 for infinite
    void setState(uint8_t state);

protected:

private:
    uint8_t  _pinRelay;
    char     _name[20];
    uint16_t _timeout;
    Ticker   _tickTimeout;
};
