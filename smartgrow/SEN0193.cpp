#include "SEN0193.h"
#include <Arduino.h>

SEN0193::SEN0193(unsigned int _pinRead)
{
  this->pinRead = _pinRead;
}

void SEN0193::begin()
{	
	pinMode(this->pinRead, INPUT);
}

float SEN0193::readSoilMoisture()
{
	float moisture = (analogRead(this->pinRead) - valueO2) * 100 / (valueH2o - valueO2);
	
	return moisture;
}
