/*
 * Project: Si4463 Radio Library for AVR and Arduino (Callbacks example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Example showing all callbacks and turning IRQ off and on
 */

#include <Si446x.h>

#define CHANNEL 10

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
	Si446x_RX(CHANNEL);

	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.print(F("Got packet (Len: "));
	Serial.print(length);
	Serial.print(F(" | RSSI: "));
	Serial.print(rssi);
	Serial.println(F(")"));
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	Si446x_RX(CHANNEL);

	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.print(F("Packet CRC failed (RSSI: "));
	Serial.print(rssi);
	Serial.println(F(")"));
}

void SI446X_CB_RXBEGIN(int16_t rssi)
{
	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.print(F("Incoming packet (RSSI: "));
	Serial.print(rssi);
	Serial.println(F(")"));
}

void SI446X_CB_SENT(void)
{
	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.println(F("Packet sent"));
}

void SI446X_CB_WUT(void)
{
	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.println(F("Wakeup timer"));
}

void SI446X_CB_LOWBATT(void)
{
	// Printing to serial inside an interrupt is bad!
	// If the serial buffer fills up the program will lock up!
	// Don't do this in your program, this only works here because we're not printing too much data
	Serial.println(F("Low battery"));
}

void setup()
{
	Serial.begin(115200);

	// Start up
	Si446x_init();

	Si446x_setupCallback(SI446X_CBS_RXBEGIN | SI446X_CBS_SENT, 1); // Enable packet RX begin and packet sent callbacks
	Si446x_setLowBatt(3000); // Set low battery voltage to 3000mV
	Si446x_setupWUT(1, 8192, 0, SI446X_WUT_RUN | SI446X_WUT_BATT); // Run WUT and check battery every 2 seconds
	
	Si446x_RX(CHANNEL);
}

void loop()
{
	static uint8_t testData[] = {
		2,
		3,
		4,
		5,
		6,
		7
	};

	// Transmit some data every 500ms
	delay(500);

	Si446x_TX(testData, sizeof(testData), CHANNEL, SI446X_STATE_RX); // Transmit and go to receive mode once sent
	testData[0]++;

	// We're about to print some stuff to serial, however the callbacks also print to serial. The callbacks are ran from an interrupt which could run at any time (like in the middle of printing out the serial message below).
	// To make sure the callbacks don't run we temporarily turn the radio interrupt off.
	// When communicating with other SPI devices on the same bus as the radio then you should also wrap those sections in an SI446X_NO_INTERRUPT() block, this will stop the Si446x interrupt from running and trying to use the bus at the same time.

	SI446X_NO_INTERRUPT()
	{
		// Print the message
		Serial.println(F("Packet send begin"));
	}
	// Radio interrupt is now back on, any callbacks waiting will now run
}
