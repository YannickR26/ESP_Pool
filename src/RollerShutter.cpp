#include "RollerShutter.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
RollerShutter::RollerShutter(uint8_t pinOpen, uint8_t pinClose)
{
    _pinOpen = pinOpen;
    _pinClose = pinClose;
    pinMode(_pinOpen, OUTPUT);
    pinMode(_pinClose, OUTPUT);
    digitalWrite(_pinOpen, 0);
    digitalWrite(_pinClose, 0);
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
    digitalWrite(_pinClose, 0);
    digitalWrite(_pinOpen, 1);

    if (_timeout > 0)
        _tickTimeout.once(_timeout, +[](RollerShutter *inst) { inst->stop(); }, this);
}

void RollerShutter::stop()
{
    digitalWrite(_pinClose, 0);
    digitalWrite(_pinOpen, 0);
}

void RollerShutter::close()
{
    digitalWrite(_pinOpen, 0);
    digitalWrite(_pinClose, 1);

    if (_timeout > 0)
        _tickTimeout.once(_timeout, +[](RollerShutter *inst) { inst->stop(); }, this);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/