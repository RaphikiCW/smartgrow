#pragma once

class YL100
{
private:
	unsigned int pinRead;
	float valueO2 = 700;
public:
  YL100(unsigned int _pinRead);
	void begin();
	float readSoilMoisture();
};
