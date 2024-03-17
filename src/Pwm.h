#pragma once

#include <Arduino.h>

#define NB_STEP 25

class Pwm
{
public:
    Pwm(const uint8_t pin, const uint8_t channel, const uint16_t frequency);
    virtual ~Pwm();

    void setup();
    void handle();

    void setFadingSpeed(uint16_t speedInMs); // Set to 0 if no fading

    uint32_t getValue();
    uint8_t  getValueInPercent();

    void setValue(const uint32_t value);
    void setValueInPercent(const uint8_t value);

private:
    void     setPwmValue(const uint32_t value);
    uint32_t getPwmValue();

private:
    uint8_t       _pin;
    uint8_t       _channel;
    uint16_t      _frequency;
    unsigned long _oldTick;
    uint16_t      _fadingSpeed;
    int8_t        _step;
    uint32_t      _newValue, _currentValue;
};
