#include <ArduinoJson.h>

#include "RollerShutter.h"
#include "Mqtt.h"
#include "Logger.h"

#include "JsonConfiguration.h"
#include "ExtendedRelay.h"
extern ExtendedRelay pump;

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
RollerShutter::RollerShutter(uint8_t pinOpen, uint8_t pinClose)
{
    _pinOpen  = pinOpen;
    _pinClose = pinClose;
    pinMode(_pinOpen, OUTPUT);
    pinMode(_pinClose, OUTPUT);
    digitalWrite(_pinOpen, 0);
    digitalWrite(_pinClose, 0);
    _duration  = 0.f;
    _position  = 0; // 0 => Close, 100 => Open
    _target    = 0;
    _direction = 0; // -1 => to Close, 0 => Stop, 1 => to Open
    _oldTick   = 0;
}

RollerShutter::~RollerShutter()
{
}

void RollerShutter::handle()
{
    static unsigned long oldTickPublish;
    const float          deltaT       = abs((long)(millis() - _oldTick)) / 1000;
    const float          movePosition = (deltaT / _duration) * 100;
    _oldTick                          = millis();

    // Open
    if (_direction == 1)
    {
        _position += movePosition;
        if (_position > 100.f)
        {
            _position = 100.f;
        }
        if (_position >= _target)
        {
            stop();
        }
    }
    // Close
    else if (_direction == -1)
    {
        _position -= movePosition;
        if (_position < 0.f)
        {
            _position = 0.f;
        }
        if (_position <= _target)
        {
            stop();
        }
    }

    // Publish state every 1 seconds
    if ((millis() - oldTickPublish) >= (1 * 1000))
    {
        oldTickPublish = millis();
        publishState();
    }
}

void RollerShutter::setDuration(const float timeInSeconds)
{
    _duration = timeInSeconds;
}

void RollerShutter::setCurrentPosition(const float position)
{
    _position = position;
    _target   = position;
}

float RollerShutter::getTarget()
{
    return _target;
}

// 0 %   => Close
// 100 % => Open
void RollerShutter::setPosition(const float positionInPercent)
{
    if ((int)_position == (int)positionInPercent)
    {
        return;
    }

    _target = positionInPercent;

    if (_position < _target)
    {
        _direction = 1;
    }
    else
    {
        _direction = -1;
    }

    // Compute the time to go to the target
    const float timeToStop = abs(_position - _target) * _duration / 100.f;
    _tickerStop.detach();
    _tickerStop.once(
        timeToStop, +[](RollerShutter* inst) { inst->stop(true); }, this
    );

    move();
}

void RollerShutter::open()
{
    setPosition(100);
}

void RollerShutter::close()
{
    setPosition(0);
}

void RollerShutter::stop(bool forced)
{
    _direction = 0;

    if (forced)
    {
        _position = _target;
    }

    move();
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/

void RollerShutter::move()
{
    if (_direction == 1)
    {
        digitalWrite(_pinClose, 0);
        delay(100);
        digitalWrite(_pinOpen, 1);
        _oldTick = millis();
        Log.println("Open rollerShutter...");
        // Stop the pump during Open rollerShutter
        pump.setModeAuto(false);
        pump.setState(RELAY_OFF);
    }
    else if (_direction == -1)
    {
        digitalWrite(_pinOpen, 0);
        delay(100);
        digitalWrite(_pinClose, 1);
        _oldTick = millis();
        Log.println("Close rollerShutter...");
        // Stop the pump during Close rollerShutter
        pump.setModeAuto(false);
        pump.setState(RELAY_OFF);
    }
    else
    {
        digitalWrite(_pinClose, 0);
        digitalWrite(_pinOpen, 0);
        Log.println("Stop rollerShutter !");
        // Restore modeAuto to pump
        pump.setModeAuto(Configuration._pumpModeAuto);
    }

    publishState(true);
}

void RollerShutter::publishState(bool forced)
{
    // Force publish state every 5 minutes
    static unsigned long oldTick;
    if ((millis() - oldTick) >= (5 * 60 * 1000))
    {
        oldTick = millis();
        forced  = true;
    }

    if (_direction == 0 && !forced)
    {
        return;
    }

    DynamicJsonDocument json(256);

    json["position"]  = (int)_position;
    json["direction"] = _direction;
    json["target"]    = (int)_target;

    String jsonStr;
    serializeJson(json, jsonStr);
    Log.println(jsonStr);

    MqttClient.publish(String("rollerShutter"), jsonStr);
}
