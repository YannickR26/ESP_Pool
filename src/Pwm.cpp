#include "Pwm.h"

#include "Logger.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
Pwm::Pwm(const uint8_t pin, const uint8_t channel, const uint16_t frequency) :
    _pin(pin),
    _channel(channel),
    _frequency(frequency),
    _oldTick(0),
    _fadingSpeed(0),
    _step(0)
{
    _currentValue = _newValue = 0;

    // Disable fading
    _fadingSpeed = 0;
}

Pwm::~Pwm()
{
}

void Pwm::setup()
{
    if (ledcSetup(_channel, _frequency, 8) == 0)
    {
        Log.println("Pwm::Pwm - Error => Unable to create PWM");
    }

    ledcAttachPin(_pin, _channel);

    // Log.println("Pwm::Pwm - Info => Create PWM successfully: " + String(_pin) + ", " + String(_channel) + ", " + String(_frequency));

    setPwmValue(0);
}

void Pwm::handle()
{
    static uint8_t      numStep     = 0;
    const unsigned long currentTick = millis();

    // If new value is different, we apply this
    if (_newValue != _currentValue)
    {
        // Fading ON
        if (_fadingSpeed != 0)
        {
            if (abs((long)(currentTick - _oldTick)) >= _fadingSpeed)
            {
                if (numStep == NB_STEP)
                {
                    _currentValue = _newValue;
                    numStep       = 0;
                }
                else
                {
                    _currentValue += _step;
                    numStep++;
                }

                setPwmValue(_currentValue);
                _oldTick = currentTick;
            }
        }

        // Fading OFF, we apply directly new value
        else
        {
            setPwmValue(_newValue);
        }
    }
    else
    {
        numStep = 0;
    }
}

void Pwm::setFadingSpeed(uint16_t speedInMs)
{
    _fadingSpeed = speedInMs;
}

uint32_t Pwm::getValue()
{
    return getPwmValue();
}

uint8_t Pwm::getValueInPercent()
{
    return (uint8_t)map(getValue(), 0, 256, 0, 100);
}

void Pwm::setValue(const uint32_t value)
{
    float step = (int32_t)(value - _currentValue);
    step /= NB_STEP;

    if ((0.f < step) && (step < 1.f))
    {
        step = 1;
    }
    else if ((-1.f < step) && (step < 0.f))
    {
        step = -1;
    }

    _step = (int8_t)step;

    _newValue = value;
    _oldTick  = millis();
}

void Pwm::setValueInPercent(const uint8_t valueinPercent)
{
    uint32_t value = map(valueinPercent, 0, 100, 0, 256);
    setValue(value);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void Pwm::setPwmValue(const uint32_t value)
{
    // Set new value
    Log.println("Pwm::setValue - Info => set value " + String(value) + " on pin " + String(_pin) + " on channel " + String(_channel));
    ledcWrite(_channel, value);

    // Read current value
    getPwmValue();
}

uint32_t Pwm::getPwmValue()
{
    _currentValue = ledcRead(_channel);

    return _currentValue;
}
