#pragma once

#include <Arduino.h>
#include <Ticker.h>

class RollerShutter
{
public:
    RollerShutter(uint8_t pinOpen, uint8_t pinClose);
    virtual ~RollerShutter();

    void setTimeout(uint16_t timeInSeconds);
    void open();
    void stop();
    void close();

protected:
private:
    uint8_t _pinOpen, _pinClose;
    uint16_t _timeout;
    Ticker _tickTimeout;
};