/*
 * Project: Si4463 Radio Library for AVR and Arduino (WUT, ADC and GPIO example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * WUT (Wakeup timer), ADC and GPIO operations
 *
 * Here the wakeup timer is used to initiate a battery and temperature reading.
 * Every 500ms the GPIO0 output state is toggled between HIGH and LOW and GPIO input values are read.
 * If GPIO1 is LOW then a message is printed.
 */

#include <Si446x.h>

static volatile uint8_t wut;
static volatile uint8_t lowbatt;

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
	(void)(length); // Stop warnings about unused parameters
	(void)(rssi);
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	(void)(rssi);
}

void SI446X_CB_WUT()
{
	wut = 1;
}

void SI446X_CB_LOWBATT()
{
	lowbatt = 1;
}

void setup()
{
	Serial.begin(115200);
	
	// Start up
	Si446x_init();

	Si446x_setLowBatt(3000); // Set low battery voltage to 3000mV
	Si446x_setupWUT(0, 16384, 0, SI446X_WUT_RUN | SI446X_WUT_BATT); // Run WUT and check battery every 2 seconds
	Si446x_writeGPIO(SI446X_GPIO1, SI446X_GPIO_MODE_INPUT | SI446X_PIN_PULL_EN); // Set GPIO 1 as INPUT with PULLUP
	Si446x_sleep();
}

void loop()
{
	static uint32_t lastRead;
	static uint8_t gpState;

	if(wut)
	{
		wut = 0;
		uint16_t batt = Si446x_adc_battery(); // Read supply voltage
		float temp = Si446x_adc_temperature(); // Read temperature
		Si446x_sleep(); // Go to sleep
		
		// Print values
		Serial.print(F("Battery: "));
		Serial.print(batt);
		Serial.print(F(" | Temp: "));
		Serial.println(temp);
	}

	// Print a message when the supply voltage is below the value set by Si446x_setLowBatt()
	if(lowbatt)
	{
		lowbatt = 0;
		Si446x_sleep();
		Serial.println(F("Low battery!"));
	}
	
	if(millis() - lastRead > 500)
	{
		lastRead = millis();
		
		// Toggle GPIO 0 output state
		if(gpState)
			Si446x_writeGPIO(SI446X_GPIO0, SI446X_GPIO_MODE_DRIVE1);
		else
			Si446x_writeGPIO(SI446X_GPIO0, SI446X_GPIO_MODE_DRIVE0);
		gpState = !gpState;

		// Read GPIO input values
		uint8_t states = Si446x_readGPIO();

		// If GPIO1 is LOW then print a message
		if(!(states & _BV(SI446X_GPIO1)))
			Serial.println(F("GPIO1 Active!"));
		
		// Go to sleep
		Si446x_sleep();
	}
}
