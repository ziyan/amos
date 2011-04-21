// CMPS03 Interface Code
// Simply spit out compass data in the following format:
// 	[STX:0x02] [HI-byte] [LO-byte]
// Compile it using Arduino

#include <Wire.h>

#define POLL_TIME 10
#define REPORT_INTERVAL 5
#define BAUD_RATE 115200
#define LED_PIN 13
#define RUNNING_AVG_WEIGHT 0.2f

void setup()
{
	//Initialize I2C and serial
	Wire.begin();
	Serial.begin(BAUD_RATE);
	pinMode(LED_PIN, OUTPUT);
}

uint16_t heading = 0;
uint16_t i = 0;
uint16_t error = 0;

void loop()
{
	// request transfer from register 1
	Wire.beginTransmission(96);
	Wire.send(2);
	Wire.endTransmission();

	// read register
	Wire.requestFrom(96, 2);
	byte hi = 0, low = 0;
	if (Wire.available())
		hi = Wire.receive();
	if (Wire.available())
		low = Wire.receive();

	uint16_t value = (uint16_t)(hi << 8 | low);
	// running average
	heading = RUNNING_AVG_WEIGHT * value + (1.0f - RUNNING_AVG_WEIGHT) * heading;

	// blink LED
	digitalWrite(LED_PIN, HIGH);
	delay(POLL_TIME/2);
	digitalWrite(LED_PIN, LOW);
	delay(POLL_TIME/2);

	// report
	if (!(++i % REPORT_INTERVAL))
	{
		i = 0;
		Serial.print(0x02, BYTE); //STX
		Serial.print((heading >> 8) & 0x00ffu, BYTE);
		Serial.print(heading & 0x00ffu, BYTE);
	}
}
