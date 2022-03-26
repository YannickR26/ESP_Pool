#pragma once

#include <Arduino.h>
#include <Ticker.h>

#define RELAY_ON    1
#define RELAY_OFF   0

class ExtendedRelay
{
public:
    ExtendedRelay(uint8_t pinRelay, const char* name);
    virtual ~ExtendedRelay();

    void handle();

    void setModeAuto(const bool enable);

    void setStartTime(const uint8_t hours, const uint8_t minutes);
    void setStopTime(const uint8_t hours, const uint8_t minutes);
    void setState(const uint8_t state);
    const int getState();

private:
    uint8_t _pinRelay;
    char _name[20];
    bool _modeAuto;
    uint8_t _startHours, _startMinutes;
    uint8_t _stopHours, _stopMinutes;
};