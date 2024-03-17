#include "ExtendedRelay.h"
#include "Logger.h"
#include "Mqtt.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
ExtendedRelay::ExtendedRelay(uint8_t pinRelay, const char* name)
{
    _pinRelay = pinRelay;
    strncpy(_name, name, sizeof(_name));
    pinMode(_pinRelay, OUTPUT);
    digitalWrite(_pinRelay, 0);
    _modeAuto   = false;
    _startHours = _startMinutes = 0;
    _stopHours = _stopMinutes = 0;
}

ExtendedRelay::~ExtendedRelay()
{
}

void ExtendedRelay::handle()
{
    static unsigned long oldTick = 0;

    // If mode auto is disable, we quit now
    if (_modeAuto == false)
    {
        return;
    }

    // If no start and no stop time, we quit now
    if ((_startHours == 0) && (_startMinutes == 0) && (_stopHours == 0) && (_stopMinutes == 0))
    {
        return;
    }

    // Check state every 20 seconds
    if ((millis() - oldTick) < (20 * 1000))
    {
        return;
    }

    // Relay OFF
    if (getState() == RELAY_OFF)
    {
        if (checkTime())
        {
            setState(RELAY_ON);
        }
    }

    // Relay ON
    else
    {
        if (!checkTime())
        {
            setState(RELAY_OFF);
        }
    }

    oldTick = millis();
}

void ExtendedRelay::setModeAuto(const bool enable)
{
    _modeAuto = enable;
}

void ExtendedRelay::setStartTime(const uint8_t hours, const uint8_t minutes)
{
    _startHours   = hours;
    _startMinutes = minutes;
}

void ExtendedRelay::setStopTime(const uint8_t hours, const uint8_t minutes)
{
    _stopHours   = hours;
    _stopMinutes = minutes;
}

void ExtendedRelay::setState(const uint8_t state)
{
    digitalWrite(_pinRelay, state);

    Log.println("set " + String(_name) + " to " + String(state));
    MqttClient.publish(String(_name), String(state));
}

const int ExtendedRelay::getState()
{
    return digitalRead(_pinRelay);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

// Return true if currentTime is bewteen startTime and stopTime
bool ExtendedRelay::checkTime()
{
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo))
    {
        return false;
    }

    // Compute to time in minutes (0 -> 1439 min)
    uint16_t currentTime = timeInfo.tm_hour * 60;
    currentTime += timeInfo.tm_min;

    uint16_t startTime = _startHours * 60;
    startTime += _startMinutes;

    uint16_t stopTime = _stopHours * 60;
    stopTime += _stopMinutes;

    //                           _____________
    // 00:00 _____________ Start               Stop _____________ 23:59
    if (startTime < stopTime)
    {
        if ((startTime <= currentTime) && (currentTime < stopTime))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    //       _____                                          _____
    // 00:00       Stop _____________________________ Start       23:59
    else if (startTime > stopTime)
    {
        if ((stopTime <= currentTime) && (currentTime < startTime))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    return false;
}
