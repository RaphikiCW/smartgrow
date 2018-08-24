#include <SoftwareSerial.h>

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define TRAME_ACK 

class K30
{
  private:
	  SoftwareSerial* _Serial;
  public:
    K30(uint8_t rx, uint8_t tx);
	void begin();
    uint32_t getCO2();
};
