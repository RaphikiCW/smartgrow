#pragma once

class SEN0193
{
private:
	unsigned int pinRead;
	float valueO2 = 820;
  float valueH2o = 350;
public:
  SEN0193(unsigned int _pinRead);
	void begin();
	float readSoilMoisture();
};
