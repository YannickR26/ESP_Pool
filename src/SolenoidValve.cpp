#include "SolenoidValve.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
SolenoidValve::SolenoidValve(uint8_t pinPositivePower, uint8_t pinNegativePower)
{
    _pinPositivePower = pinPositivePower;
    _pinNegativePower = pinNegativePower;
    pinMode(_pinPositivePower, OUTPUT);
    pinMode(_pinNegativePower, OUTPUT);
    digitalWrite(_pinPositivePower, 0);
    digitalWrite(_pinNegativePower, 0);
}

SolenoidValve::~SolenoidValve()
{
}

void SolenoidValve::open()
{
    digitalWrite(_pinNegativePower, 0);
    digitalWrite(_pinPositivePower, 1);
    delay(200);
    digitalWrite(_pinNegativePower, 0);
    digitalWrite(_pinPositivePower, 0);
}

void SolenoidValve::close()
{
    digitalWrite(_pinPositivePower, 0);
    digitalWrite(_pinNegativePower, 1);
    delay(200);
    digitalWrite(_pinPositivePower, 0);
    digitalWrite(_pinNegativePower, 0);
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/