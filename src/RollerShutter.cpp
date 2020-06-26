#include "RollerShutter.h"
#include "Mqtt.h"
#include "Logger.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
RollerShutter::RollerShutter(uint8_t pinOpen, uint8_t pinClose)
{
    _pinOpen = pinOpen;
    _pinClose = pinClose;
    pinMode(_pinOpen, OUTPUT);
    pinMode(_pinClose, OUTPUT);
    digitalWrite(_pinOpen, 1);
    digitalWrite(_pinClose, 1);
    _timeout = 0;
}

RollerShutter::~RollerShutter()
{
}

void RollerShutter::setTimeout(uint16_t timeInSeconds)
{
    _timeout = timeInSeconds;
}

void RollerShutter::open()
{
    digitalWrite(_pinClose, 1);
    delay(100);
    digitalWrite(_pinOpen, 0);

    Log.println("Open rollerShutter !");
    MqttClient.publish(String("rollerShutter"), String("open"));

    _tickTimeout.detach();

    if (_timeout > 0)
        _tickTimeout.once(_timeout, +[](RollerShutter *inst) { inst->stop(); }, this);
}

void RollerShutter::stop()
{
    digitalWrite(_pinClose, 1);
    digitalWrite(_pinOpen, 1);

    Log.println("Stop rollerShutter !");
    MqttClient.publish(String("rollerShutter"), String("stop"));

    _tickTimeout.detach();
}

void RollerShutter::close()
{
    digitalWrite(_pinOpen, 1);
    delay(100);
    digitalWrite(_pinClose, 0);

    Log.println("Close rollerShutter !");
    MqttClient.publish(String("rollerShutter"), String("close"));

    _tickTimeout.detach();

    if (_timeout > 0)
        _tickTimeout.once(_timeout, +[](RollerShutter *inst) { inst->stop(); }, this);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/