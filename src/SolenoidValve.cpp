#include "SolenoidValve.h"
#include "Logger.h"
#include "Mqtt.h"


extern float waterQty1, waterQty2;   // in l
extern float waterLevel; // in cm

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
    _isOpen = false;
    _timeout = 0;
    _waterQuantity = 0;
    _waterQuantityMax = 0;
}

SolenoidValve::~SolenoidValve()
{
}

void SolenoidValve::handle()
{
    if (_isOpen == true && _waterQuantityMax != 0)
    {
        if ((waterQty1 - _waterQuantity) >= _waterQuantityMax)
        {
            close();
        }
    }
}

void SolenoidValve::setTimeout(uint16_t timeInSeconds)
{
    _timeout = timeInSeconds;
}

void SolenoidValve::setMaxWaterQuantity(uint16_t waterQuantity)
{
    _waterQuantityMax = waterQuantity;
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

    _tickTimeout.detach();
    _isOpen = true;
    _waterQuantity = waterQty1;

    if (_timeout != 0)
        _tickTimeout.once(_timeout, +[](SolenoidValve *inst) { inst->close(); }, this);
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

    _tickTimeout.detach();
    _isOpen = false;
}

/********************************************************/
/******************** Private Method ********************/
/********************************************************/