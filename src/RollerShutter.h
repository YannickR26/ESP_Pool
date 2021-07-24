#pragma once

#include <Arduino.h>
#include <Ticker.h>

class RollerShutter
{
public:
    RollerShutter(uint8_t pinOpen, uint8_t pinClose);
    virtual ~RollerShutter();

    void handle();

    void setDuration(const float timeInSeconds);
    void setCurrentPosition(const float position);
    void setPosition(const float positionInPercent);
    void open();
    void close();
    void stop(bool forced = false);

protected:
private:
    void move();
    void publishState(bool forced = false);

    uint8_t _pinOpen, _pinClose;
    float _duration;    // in Seconds
    float _position;    // in %
    float _target;      // in %
    int8_t _direction;
    unsigned long _oldTick;
    Ticker _tickerStop;
};