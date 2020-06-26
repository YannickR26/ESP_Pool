#include "SolenoidValve.h"
#include "Logger.h"
#include "Mqtt.h"

/********************************************************/
/******************** Public Method *********************/
/********************************************************/
SolenoidValve::SolenoidValve(uint8_t pinPositivePower, uint8_t pinNegativePower)
{
    _pinPositivePower = pinPositivePower;
    _pinNegativePower = pinNegativePower;
    pinMode(_pinPositivePower, OUTPUT);
    pinMode(_pinNegativePower, OUTPUT);
    digitalWrite(_pinPositivePower, 1);
    digitalWrite(_pinNegativePower, 1);
}

SolenoidValve::~SolenoidValve()
{
}

void SolenoidValve::open()
{
    digitalWrite(_pinPositivePower, 1);
    digitalWrite(_pinNegativePower, 0);
    delay(500);
    digitalWrite(_pinNegativePower, 1);
    digitalWrite(_pinPositivePower, 1);

    Log.println("Open solenoidValve !");
    MqttClient.publish(String("solenoidValve"), String("open"));
}

void SolenoidValve::close()
{
    digitalWrite(_pinNegativePower, 1);
    digitalWrite(_pinPositivePower, 0);
    delay(500);
    digitalWrite(_pinPositivePower, 1);
    digitalWrite(_pinNegativePower, 1);

    Log.println("Close solenoidValve !");
    MqttClient.publish(String("solenoidValve"), String("close"));
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/