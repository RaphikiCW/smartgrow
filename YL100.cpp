#include "YL100.h"
#include <Arduino.h>

YL100::YL100(unsigned int _pinRead)
{
  this->pinRead = _pinRead;
}

void YL100::begin()
{	
	pinMode(this->pinRead, INPUT);
}

float YL100::readSoilMoisture()
{
	float moisture = analogRead(this->pinRead) / (this->valueO2/100);
	
	return moisture;
}
