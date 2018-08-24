#include "K30.h"

byte trameAck[] = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};

K30::K30(uint8_t rx, uint8_t tx)
{
	this->_Serial = new SoftwareSerial(rx, tx);
}

void K30::begin()
{
	this->_Serial->begin(9600);
}

uint32_t K30::getCO2()
{
	uint32_t timer = millis();
	  
	while(this->_Serial->available() < 7 && millis() - timer < 1000)
	{
		this->_Serial->flush();
		this->_Serial->write(trameAck, 7);
		delay(100);
	}

	if (this->_Serial->available() == 7)
	{
		for (uint8_t i = 0; i < 3; i++)
		  this->_Serial->read();

		uint32_t co2 = this->_Serial->read() * 256;
		co2 += this->_Serial->read();

		return co2;
	}
	else
		return -1;

	delay(500);
}
