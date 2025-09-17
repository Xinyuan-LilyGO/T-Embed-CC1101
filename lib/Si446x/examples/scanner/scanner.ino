/*
 * Project: Si4463 Radio Library for AVR and Arduino (Channel scanner example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Channel scanner
 *
 * Listen to each channel, record the highest RSSI value and print a pretty graph
 */

#include <Si446x.h>

static volatile uint8_t channel;

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
	(void)(length); // Stop warnings about unused parameters
	(void)(rssi);

	Si446x_RX(channel);
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	(void)(rssi);

	Si446x_RX(channel);
}

void setup()
{
	Serial.begin(115200);

	// Start up
	Si446x_init();
}

void loop()
{
	Si446x_RX(channel);
	
	int16_t peakRssi = -999;
	for(uint16_t i=0;i<1800;i++)
	{
		delay(1);
		int16_t rssi = Si446x_getRSSI();
		if(rssi > peakRssi)
			peakRssi = rssi;
	}

	uint16_t bars = (130 - (peakRssi * -1)) / 4;

	char buff[6];
	sprintf_P(buff, PSTR("%03hhu: "), channel);
	Serial.print(buff);

	for(uint16_t i=0;i<bars;i++)
		Serial.print(F("|"));

	Serial.print(F(" ("));
	Serial.print(peakRssi);
	Serial.println(F(")"));

	channel++;
}
