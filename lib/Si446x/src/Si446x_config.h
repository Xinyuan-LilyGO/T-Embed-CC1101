/*
 * Project: Si4463 Radio Library for AVR and Arduino
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

#ifndef SI443X_CONFIG_H_
#define SI443X_CONFIG_H_

// If other libraries communicate with SPI devices while inside an interrupt then set this to 1, otherwise you can set this to 0
// If you're not sure then leave this at 1
// If this is 1 then global interrupts will be turned off when this library uses the SPI bus
#define SI446X_INT_SPI_COMMS 1

// ADC Conversion time
// 1 - 15
// RATE  = SYS_CLK / 12 / 2^(SI446X_ADC_SPEED + 1)
// SYS_CLK is usually 30MHz
// 10, 11 or 12 is recommended
// 10 = 0.82ms (1.22KHz)
// 11 = 1.64ms (610Hz)
// 12 = 3.27ms (305Hz)
// A slower conversion gives higher resolution
#define SI446X_ADC_SPEED 10

// Mode to enter when radio is idle
// The radio is put into idle mode when new data is being loaded for transmission, just before starting to receive and once a packet has been received
// This option effects response time to TX/RX mode and current consumption
// NOTE: After receiving an invalid packet the radio will be put into SLEEP mode instead of the option chosen here, this is to fix an issue with INVALID_SYNC causing the radio to get stuck
//
// SI446X_STATE_SPI_ACTIVE
//	Response time: 340us
//	Current consumption: 1.35mA
//
// SI446X_STATE_READY
//	Response time: 100us
//	Current consumption: 1.8mA
#define SI446X_IDLE_MODE SI446X_STATE_READY

// To use variable length packets set this to 0
// Otherwise for fixed length packets this should be set to the length. The len parameter in Si446x_TX() will then be ignored.
// Using fixed length packets will stop the length field from being transmitted, reducing the transmission by 3 bytes.
#define SI446X_FIXED_LENGTH 0


///////////////////
// Pin stuff
///////////////////

// Arduino pin assignments
#define SI446X_CSN			10
#define SI446X_SDN			5
#define SI446X_IRQ			2 // This needs to be an interrupt pin





// --------------------------------------
// Everything below here is for non-Arduino stuff
// --------------------------------------

// SPI slave select pin
#define SI446X_CSN_PORT		B
#define SI446X_CSN_BIT		2

// Shutdown pin
#define SI446X_SDN_PORT		D
#define SI446X_SDN_BIT		5

// Interrupt pin
#define SI446X_IRQ_PORT		D
#define SI446X_IRQ_BIT		2



///////////////////
// **************************** NOT for Arduino ****************************
// Interrupt register stuff
///////////////////

// Interrupt number
// This must match the INT that the NIRQ pin is connected to
#define SI446X_INTERRUPT_NUM	0



// Leave these commented out to let the library figure out what registers to use

// Which interrupt to use for IRQ
//#define SI446X_REG_EXTERNAL_INT	EIMSK
//#define SI446X_BIT_EXTERNAL_INT	INT1
//#define SI446X_INT_VECTOR			INT1_vect

// Set interrupt to trigger on LOW
// NOT USED
//#define SI446X_REG_EXTERNAL_INT_CTL	EICRA
//#define SI446X_BIT_EXTERNAL_INT_CTL	(0)



#define SI446X_CONCAT(a, b) a ## b
#define SI446X_INTCONCAT(num) SI446X_CONCAT(INT, num)

#ifndef SI446X_REG_EXTERNAL_INT
	#ifdef EIMSK
		#define SI446X_REG_EXTERNAL_INT EIMSK
	#elif defined GICR
		#define SI446X_REG_EXTERNAL_INT GICR
	#else
		#define SI446X_REG_EXTERNAL_INT GIMSK
	#endif
#endif

#ifndef SI446X_BIT_EXTERNAL_INT
	#define SI446X_BIT_EXTERNAL_INT SI446X_INTCONCAT(SI446X_INTERRUPT_NUM)
#endif



// NOT PROPERLY TESTED, KEEP 1
// what happens if:
// 1. Si446x_SERVICE()
// 2. *new packet RX after service()*
// 3. Si446x_TX(blah)
// RX packet is lost, what are the interrupt pending statuses at?
//
// Use pin interrupt
// If this is 1 and you have other devices that use the SPI bus then you will need to wrap areas of code that communicate with those devices with SI446X_NO_INTERRUPT()
// If this is 0 make sure to call Si446x_SERVICE() as often as possible so that the library can process radio events
// 0 = Off, run callbacks from Si446x_SERVICE()
// 1 = On, run callbacks from interrupt
#define SI446X_INTERRUPTS 1 // DO NOT CHANGE


#endif /* SI443X_CONFIG_H_ */
