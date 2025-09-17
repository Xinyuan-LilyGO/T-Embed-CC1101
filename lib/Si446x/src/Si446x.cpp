/*
 * Project: Si4463 Radio Library for AVR and Arduino
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

#ifdef ARDUINO
#include <Arduino.h>
#include <SPI.h>
#else
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <util/delay.h>
#include "Si446x_spi.h"
#endif

#include <string.h>
#include <stdint.h>
#include "Si446x.h"
#include "Si446x_config.h"
#include "Si446x_defs.h"

#include "radio_config.h"

#define IDLE_STATE SI446X_IDLE_MODE

// When FIFOs are combined it becomes a 129 byte FiFO
// The first byte is used for length, then the remaining 128 bytes for the packet data
#define MAX_PACKET_LEN			SI446X_MAX_PACKET_LEN

#define IRQ_PACKET				0
#define IRQ_MODEM				1
#define IRQ_CHIP				2

#define rssi_dBm(val)			((val / 2) - 134)

#if SI446X_INTERRUPTS != 0
	#if defined(ARDUINO) && SI446X_IRQ == -1
		#error "SI446X_INTERRUPTS is 1, but SI446X_IRQ is set to -1!"
	#elif !defined(ARDUINO) && !defined(IRQ_BIT)
		#error "SI446X_INTERRUPTS is 1, but SI446X_IRQ_PORT or SI446X_IRQ_BIT has not been set!"
	#endif
#endif

#ifdef ARDUINO
#define	delay_ms(ms)			delay(ms)
#define delay_us(us)			delayMicroseconds(us)
#define spiSelect()				(digitalWrite(SI446X_CSN, LOW))
#define spiDeselect()			(digitalWrite(SI446X_CSN, HIGH))
#define spi_transfer_nr(data)	(SPI.transfer(data))
#define spi_transfer(data)		(SPI.transfer(data))
#else
#define	delay_ms(ms)			_delay_ms(ms)
#define delay_us(us)			_delay_us(us)
#define spiSelect()				(CSN_PORT &= ~_BV(CSN_BIT))
#define spiDeselect()			(CSN_PORT |= _BV(CSN_BIT))
#endif

static const uint8_t config[] PROGMEM = RADIO_CONFIGURATION_DATA_ARRAY;

static volatile uint8_t enabledInterrupts[3];

// http://stackoverflow.com/questions/10802324/aliasing-a-function-on-a-c-interface-within-a-c-application-on-linux
#if defined(__cplusplus)
extern "C" {
#endif
	static void __empty_callback0(void){}
	static void __empty_callback1(int16_t param1){(void)(param1);}
#if defined(__cplusplus)
}
#endif

void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_CMDTIMEOUT(void);
void __attribute__((weak, alias ("__empty_callback1"))) SI446X_CB_RXBEGIN(int16_t rssi);
void __attribute__((weak)) SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi){(void)(length);(void)(rssi);}
void __attribute__((weak, alias ("__empty_callback1"))) SI446X_CB_RXINVALID(int16_t rssi);
void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_SENT(void);
void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_WUT(void);
void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_LOWBATT(void);
#if SI446X_ENABLE_ADDRMATCHING
void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_ADDRMATCH(void);
void __attribute__((weak, alias ("__empty_callback0"))) SI446X_CB_ADDRMISS(void);
#endif

// TODO
//void __attribute__((weak)) SI446X_CB_DEBUG(uint8_t* interrupts){(void)(interrupts);}

// http://www.nongnu.org/avr-libc/user-manual/atomic_8h_source.html

#ifdef ARDUINO

#if SI446X_INTERRUPTS != 0
// It's not possible to get the current interrupt enabled state in Arduino (SREG only works for AVR based Arduinos, and no way of getting attachInterrupt() status), so we use a counter thing instead
static volatile uint8_t isrState_local;
#endif

#if SI446X_INTERRUPTS == 1 || SI446X_INT_SPI_COMMS == 1
static volatile uint8_t isrState;
static volatile uint8_t isrBusy; // Don't mess with global interrupts if we're inside an ISR

static inline uint8_t interrupt_off(void)
{
	if(!isrBusy)
	{
		noInterrupts();
		isrState = isrState + 1;
	}
	return 1;
}

static inline uint8_t interrupt_on(void)
{
	if(!isrBusy)
	{
		if(isrState > 0)
			isrState = isrState - 1;
		if(isrState == 0)
			interrupts();
	}
	return 0;
}
#endif

#endif

static inline uint8_t cselect(void)
{
//	spi_enable();
	spiSelect();
	return 1;
}

static inline uint8_t cdeselect(void)
{
	spiDeselect();
//	spi_disable();
	return 0;
}

#define CHIPSELECT()	for(uint8_t _cs = cselect(); _cs; _cs = cdeselect())

// TODO
// 2 types of interrupt blocks
// Local (SI446X_NO_INTERRUPT()): Disables the pin interrupt so the ISR does not run while normal code is busy in the Si446x code, however another interrupt can enter the code which would be bad.
// Global (SI446X_ATOMIC()): Disable all interrupts, don't use waitForResponse() inside here as it can take a while to complete. These blocks are to make sure no other interrupts use the SPI bus.

// If an interrupt might do some SPI communications with another device then we
// need to turn global interrupts off while communicating with the radio.
// Otherwise, just turn off our own radio interrupt while doing SPI stuff.
#if SI446X_INTERRUPTS == 0 && SI446X_INT_SPI_COMMS == 0
#define SI446X_ATOMIC() ((void)(0));
#elif defined(ARDUINO)
#define SI446X_ATOMIC() for(uint8_t _cs2 = interrupt_off(); _cs2; _cs2 = interrupt_on())
#else
#define SI446X_ATOMIC()	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#endif

// When doing SPI comms with the radio or doing multiple commands we don't want the radio interrupt to mess it up.
uint8_t Si446x_irq_off()
{
#if SI446X_INTERRUPTS != 0

#ifdef ARDUINO
	detachInterrupt(digitalPinToInterrupt(SI446X_IRQ));
	isrState_local = isrState_local + 1;
	return 0;
#else
	uint8_t origVal = SI446X_REG_EXTERNAL_INT;
	SI446X_REG_EXTERNAL_INT &= ~_BV(SI446X_BIT_EXTERNAL_INT);
	origVal = !!(origVal & _BV(SI446X_BIT_EXTERNAL_INT));
	//origVal += 1; // We always want to return a non-zero value so the for() loop will loop TODO
	return origVal;
#endif

#else
	return 0;
#endif
}

void Si446x_irq_on(uint8_t origVal)
{
#if SI446X_INTERRUPTS != 0

#ifdef ARDUINO
	((void)(origVal));
	if(isrState_local > 0)
		isrState_local = isrState_local - 1;
	if(isrState_local == 0)
		attachInterrupt(digitalPinToInterrupt(SI446X_IRQ), Si446x_SERVICE, FALLING);
#else
	if(origVal)// == 2) TODO
		SI446X_REG_EXTERNAL_INT |= _BV(SI446X_BIT_EXTERNAL_INT);
#endif

#else
	((void)(origVal));
#endif
}

// Read CTS and if its ok then read the command buffer
static uint8_t getResponse(void* buff, uint8_t len)
{
	uint8_t cts = 0;

	SI446X_ATOMIC()
	{
		CHIPSELECT()
		{
			// Send command
			spi_transfer_nr(SI446X_CMD_READ_CMD_BUFF);

			// Get CTS value
			cts = (spi_transfer(0xFF) == 0xFF);

			if(cts)
			{
				// Get response data
				for(uint8_t i=0;i<len;i++)
					((uint8_t*)buff)[i] = spi_transfer(0xFF);
			}
		}
	}
	return cts;
}

// Keep trying to read the command buffer, with timeout of around 500ms
static uint8_t waitForResponse(void* out, uint8_t outLen, uint8_t useTimeout)
{
	// With F_CPU at 8MHz and SPI at 4MHz each check takes about 7us + 10us delay
	uint16_t timeout = 40000;
	while(!getResponse(out, outLen))
	{
		delay_us(10);
		if(useTimeout && !--timeout)
		{
			SI446X_CB_CMDTIMEOUT();
			return 0;
		}
	}
	return 1;
}

static void doAPI(void* data, uint8_t len, void* out, uint8_t outLen)
{
	SI446X_NO_INTERRUPT()
	{
		if(waitForResponse(NULL, 0, 1)) // Make sure it's ok to send a command
		{
			SI446X_ATOMIC()
			{
				CHIPSELECT()
				{
					for(uint8_t i=0;i<len;i++)
						spi_transfer_nr(((uint8_t*)data)[i]); // (pgm_read_byte(&((uint8_t*)data)[i]));
				}
			}

			if(((uint8_t*)data)[0] == SI446X_CMD_IRCAL) // If we're doing an IRCAL then wait for its completion without a timeout since it can sometimes take a few seconds
				waitForResponse(NULL, 0, 0);
			else if(out != NULL) // If we have an output buffer then read command response into it
				waitForResponse(out, outLen, 1);
		}
	}
}

// Configure a bunch of properties (up to 12 properties in one go)
static void setProperties(uint16_t prop, void* values, uint8_t len)
{
	// len must not be greater than 12

	uint8_t data[16] = {
		SI446X_CMD_SET_PROPERTY,
		(uint8_t)(prop>>8),
		len,
		(uint8_t)prop
	};

	// Copy values into data, starting at index 4
	memcpy(data + 4, values, len);

	doAPI(data, len + 4, NULL, 0);
}

// Set a single property
static inline void setProperty(uint16_t prop, uint8_t value)
{
	setProperties(prop, &value, 1);
}
/*
// Set a 16bit property
static void setProperty16(uint16_t prop, uint16_t value)
{
	uint8_t properties[] = {value>>8, value};
	setProperties(prop, properties, sizeof(properties));
}
*/
// Read a bunch of properties
static void getProperties(uint16_t prop, void* values, uint8_t len)
{
	uint8_t data[] = {
		SI446X_CMD_GET_PROPERTY,
		(uint8_t)(prop>>8),
		len,
		(uint8_t)prop
	};

	doAPI(data, sizeof(data), values, len);
}

// Read a single property
static inline uint8_t getProperty(uint16_t prop)
{
	uint8_t val;
	getProperties(prop, &val, 1);
	return val;
}

// Do an ADC conversion
static uint16_t getADC(uint8_t adc_en, uint8_t adc_cfg, uint8_t part)
{
	uint8_t data[6] = {
		SI446X_CMD_GET_ADC_READING,
		adc_en,
		adc_cfg
	};
	doAPI(data, 3, data, 6);
	return (data[part]<<8 | data[part + 1]);
}

// Read a fast response register
static uint8_t getFRR(uint8_t reg)
{
	uint8_t frr = 0;
	SI446X_ATOMIC()
	{
		CHIPSELECT()
		{
			spi_transfer_nr(reg);
			frr = spi_transfer(0xFF);
		}
	}
	return frr;
}

// Ge the patched RSSI from the beginning of the packet
static int16_t getLatchedRSSI(void)
{
	uint8_t frr = getFRR(SI446X_CMD_READ_FRR_A);
	int16_t rssi = rssi_dBm(frr);
	return rssi;
}

// Get current radio state
static si446x_state_t getState(void)
{
	uint8_t state = getFRR(SI446X_CMD_READ_FRR_B);
	if(state == SI446X_STATE_TX_TUNE)
		state = SI446X_STATE_TX;
	else if(state == SI446X_STATE_RX_TUNE)
		state = SI446X_STATE_RX;
	else if(state == SI446X_STATE_READY2)
		state = SI446X_STATE_READY;
	return (si446x_state_t)state;
}

// Set new state
static void setState(si446x_state_t newState)
{
	uint8_t data[] = {
		SI446X_CMD_CHANGE_STATE,
		newState
	};
	doAPI(data, sizeof(data), NULL, 0);
}

// Clear RX and TX FIFOs
static void clearFIFO(void)
{
	// 'static const' saves 20 bytes of flash here, but uses 2 bytes of RAM
	static const uint8_t clearFifo[] = {
		SI446X_CMD_FIFO_INFO,
		SI446X_FIFO_CLEAR_RX | SI446X_FIFO_CLEAR_TX
	};
	doAPI((uint8_t*)clearFifo, sizeof(clearFifo), NULL, 0);
}

/*
// Sometimes the Si446x gets all messed up if it receives a bad packet, so we have to enable the INVALID SYNC interrupt when
// a new packet starts coming in. If the INVALID SYNC interrupt is triggered then RX mode is restarted. The interrupt is turned off again
// if the packet is successfully received.
void __attribute__((weak)) SI446X_CB_RXINVALIDSYNC(void)
{
	setState(IDLE_STATE);
	clearFIFO();

	uint8_t data = SI446X_CMD_START_RX; // Restart RX with same params as before (channel etc)
	doAPI(&data, sizeof(data), NULL, 0);
	
	// TODO turn off interrupt?
}
*/

// TODO use Si446x_setupCallback instead?
/*static void fix_invalidSync_irq(uint8_t enable)
{
	uint8_t data = getProperty(SI446X_INT_CTL_MODEM_ENABLE);
	enable ? (data |= 32) : (data &= ~32); // TODO use def for 32
	enabledInterrupts[IRQ_MODEM] = data;
	setProperty(SI446X_INT_CTL_MODEM_ENABLE, data);
}
*/
// Read pending interrupts
// Reading interrupts will also clear them
// Buff should either be NULL (just clear interrupts) or a buffer of atleast 8 bytes for storing statuses
static void interrupt(void* buff)
{
	uint8_t data = SI446X_CMD_GET_INT_STATUS;
	doAPI(&data, sizeof(data), buff, 8);
}

// Similar to interrupt() but with the option of not clearing certain interrupt flags
static void interrupt2(void* buff, uint8_t clearPH, uint8_t clearMODEM, uint8_t clearCHIP)
{
	uint8_t data[] = {
		SI446X_CMD_GET_INT_STATUS,
		clearPH,
		clearMODEM,
		clearCHIP
	};
	doAPI(data, sizeof(data), buff, 8);
}

// Reset the RF chip
static void resetDevice(void)
{
#ifdef ARDUINO
	digitalWrite(SI446X_SDN, HIGH);
	delay_ms(50);
	digitalWrite(SI446X_SDN, LOW);
	delay_ms(50);
#else
	SDN_PORT |= _BV(SDN_BIT);
	delay_ms(50);
	SDN_PORT &= ~_BV(SDN_BIT);
	delay_ms(50);
#endif
}

/*
// TODO
// External version of interrupt()
void Si446x_interrupt(uint8_t* buff)
{
	uint8_t data[4] = {SI446X_CMD_GET_INT_STATUS, 0xff, 0xff, 0xff};
	doAPI(&data, sizeof(data), buff, 8);
}
*/

// Apply the radio configuration
static void applyStartupConfig(void)
{
	uint8_t buff[17];
	for(uint16_t i=0;i<sizeof(config);i++)
	{
		memcpy_P(buff, &config[i], sizeof(buff));
		doAPI(&buff[1], buff[0], NULL, 0);
		i += buff[0];
	}
}

void Si446x_init()
{
	spiDeselect();
#ifdef ARDUINO
	pinMode(SI446X_CSN, OUTPUT);
	pinMode(SI446X_SDN, OUTPUT);
#if SI446X_IRQ != -1
	pinMode(SI446X_IRQ, INPUT_PULLUP);
#endif
	
	SPI.begin();
#else
	CSN_DDR |= _BV(CSN_BIT);
	SDN_DDR |= _BV(SDN_BIT);

#ifdef IRQ_BIT
	// Interrupt pin (input with pullup)
#if defined(PUEA) || defined(PUEB) || defined(PUEC) || defined(PUED) || defined(PUEE)
	IRQ_PUE |= _BV(IRQ_BIT);
#else
	IRQ_PORT |= _BV(IRQ_BIT);
#endif
#endif

	spi_init();
#endif

	resetDevice();
	applyStartupConfig();
	interrupt(NULL);
	Si446x_sleep();

	enabledInterrupts[IRQ_PACKET] = (1<<SI446X_PACKET_RX_PEND) | (1<<SI446X_CRC_ERROR_PEND);
	//enabledInterrupts[IRQ_MODEM] = (1<<SI446X_SYNC_DETECT_PEND);

#ifndef ARDUINO
	// TODO Interrupt should trigger on low level, not falling edge?
#endif

	Si446x_irq_on(1);
}

void Si446x_getInfo(si446x_info_t* info)
{
	uint8_t data[8] = {
		SI446X_CMD_PART_INFO
	};
	doAPI(data, 1, data, 8);

	info->chipRev	= data[0];
	info->part		= (data[1]<<8) | data[2];
	info->partBuild	= data[3];
	info->id		= (data[4]<<8) | data[5];
	info->customer	= data[6];
	info->romId		= data[7];

	data[0] = SI446X_CMD_FUNC_INFO;
	doAPI(data, 1, data, 6);

	info->revExternal	= data[0];
	info->revBranch		= data[1];
	info->revInternal	= data[2];
	info->patch			= (data[3]<<8) | data[4];
	info->func			= data[5];
}

int16_t Si446x_getRSSI()
{
	uint8_t data[3] = {
		SI446X_CMD_GET_MODEM_STATUS,
		0xFF
	};
	doAPI(data, 2, data, 3);
	int16_t rssi = rssi_dBm(data[2]);
	return rssi;
}

si446x_state_t Si446x_getState()
{
	// TODO what about the state change delay with transmitting?
	return getState();
}

void Si446x_setTxPower(uint8_t pwr)
{
	setProperty(SI446X_PA_PWR_LVL, pwr);
}

#if SI446X_ENABLE_ADDRMATCHING
// API docs say that you can match on the same byte, but programming guide says you can't!
// Truth is that you can't match on the same byte (that means broadcast flag needs to be on a separate byte than the address :/)
void Si446x_setAddress(si446x_addrMode_t mode, uint8_t address)
{
	uint8_t data[] = {
		address,
		0xFF,
		SI446X_MATCH_EN | 1,

		0x80,
		0x80,
		2,

		0x00,
		0x00,
		3,

		0x00,
		0x00,
		4
	};

	if(mode == SI446X_ADDRMODE_DISABLE) // Set everything to 0 to disable address matching
		memset(data, 0, sizeof(data));
	else if(mode == SI446X_ADDRMODE_ADDR) // Disable matching for the 2nd byte
	{
		data[3] = 0x00;
		data[4] = 0x00;
	}

	setProperties(SI446X_MATCH_VALUE_1, data, sizeof(data));
}
#endif

void Si446x_setLowBatt(uint16_t voltage)
{
	// voltage should be between 1500 and 3050
	uint8_t batt = (voltage / 50) - 30;//((voltage * 2) - 3000) / 100;
	setProperty(SI446X_GLOBAL_LOW_BATT_THRESH, batt);
}

void Si446x_setupWUT(uint8_t r, uint16_t m, uint8_t ldc, uint8_t config)
{
	// Maximum value of r is 20
	
	// The API docs say that if r or m are 0, then they will have the same effect as if they were 1, but this doesn't seem to be the case?

	// Check valid config
	// TODO needed?
	if(!(config & (SI446X_WUT_RUN | SI446X_WUT_BATT | SI446X_WUT_RX)))
		return;

	SI446X_NO_INTERRUPT()
	{
		// Disable WUT
		setProperty(SI446X_GLOBAL_WUT_CONFIG, 0);

		uint8_t doRun = !!(config & SI446X_WUT_RUN);
		uint8_t doBatt = !!(config & SI446X_WUT_BATT);
		uint8_t doRx = (config & SI446X_WUT_RX);

		// Setup WUT interrupts
		uint8_t intChip = 0;//getProperty(SI446X_INT_CTL_CHIP_ENABLE); // No other CHIP interrupts are enabled so dont bother reading the current state
		//intChip &= ~((1<<SI446X_INT_CTL_CHIP_LOW_BATT_EN)|(1<<SI446X_INT_CTL_CHIP_WUT_EN));
		intChip |= doBatt<<SI446X_INT_CTL_CHIP_LOW_BATT_EN;
		intChip |= doRun<<SI446X_INT_CTL_CHIP_WUT_EN;
		enabledInterrupts[IRQ_CHIP] = intChip;
		setProperty(SI446X_INT_CTL_CHIP_ENABLE, intChip);

		// Set WUT clock source to internal 32KHz RC
		if(getProperty(SI446X_GLOBAL_CLK_CFG) != SI446X_DIVIDED_CLK_32K_SEL_RC)
		{
			setProperty(SI446X_GLOBAL_CLK_CFG, SI446X_DIVIDED_CLK_32K_SEL_RC);
			delay_us(300); // Need to wait 300us for clock source to stabilize, see GLOBAL_WUT_CONFIG:WUT_EN info
		}

		// Setup WUT
		uint8_t properties[5];
		properties[0] = doRx ? SI446X_GLOBAL_WUT_CONFIG_WUT_LDC_EN_RX : 0;
		properties[0] |= doBatt<<SI446X_GLOBAL_WUT_CONFIG_WUT_LBD_EN;
		properties[0] |= (1<<SI446X_GLOBAL_WUT_CONFIG_WUT_EN);
		properties[1] = m>>8;
		properties[2] = m;
		properties[3] = r | SI446X_LDC_MAX_PERIODS_TWO | (1<<SI446X_WUT_SLEEP);
		properties[4] = ldc;
		setProperties(SI446X_GLOBAL_WUT_CONFIG, properties, sizeof(properties));
	}
}

void Si446x_disableWUT()
{
	SI446X_NO_INTERRUPT()
	{
		setProperty(SI446X_GLOBAL_WUT_CONFIG, 0);
		setProperty(SI446X_GLOBAL_CLK_CFG, 0);
	}
}

// TODO
// ADDRESS MATCH (only useful with address mode on)
// ADDRESS MISS (only useful with address mode on)
// PACKET SENT
// PACKET RX (must never be turned off, otherwise RX mode would be pointless)
// PACKET RX INVALID (must never be turned off, otherwise RX mode would be pointless)
// PACKET BEGIN (SYNC, modem)
// WUT and LOWBATT (cant turn off/on from here, use wutSetup instead)
// INVALID SYNC (the fix thing)
void Si446x_setupCallback(uint16_t callbacks, uint8_t state)
{
	SI446X_NO_INTERRUPT()
	{
		uint8_t data[2];
		getProperties(SI446X_INT_CTL_PH_ENABLE, data, sizeof(data));

		if(state)
		{
			data[0] |= callbacks>>8;
			data[1] |= callbacks;
		}
		else
		{
			data[0] &= ~(callbacks>>8);
			data[1] &= ~callbacks;
		}
		
		// TODO
		// make sure RXCOMPELTE, RXINVALID and RXBEGIN? are always enabled

		enabledInterrupts[IRQ_PACKET] = data[0];
		enabledInterrupts[IRQ_MODEM] = data[1];
		setProperties(SI446X_INT_CTL_PH_ENABLE, data, sizeof(data));
	}
/*
	// TODO remove
	uint8_t data[4];
	data[0] = 0xFF;
	data[1] = 0b11111100;
	data[2] = 0b11111011;
	data[3] = 0xff;
	setProperties(SI446X_INT_CTL_ENABLE, data, sizeof(data));
*/
}

uint8_t Si446x_sleep()
{
	if(getState() == SI446X_STATE_TX)
		return 0;
	setState(SI446X_STATE_SLEEP);
	return 1;
}

void Si446x_read(void* buff, uint8_t len)
{
	SI446X_ATOMIC()
	{
		CHIPSELECT()
		{
			spi_transfer_nr(SI446X_CMD_READ_RX_FIFO);
			for(uint8_t i=0;i<len;i++)
				((uint8_t*)buff)[i] = spi_transfer(0xFF);
		}
	}
}
/*
// TODO maybe
void Si446x_write(void* buff, uint8_t len)
{
	// if we are going to do this then we need dual FIFO so we can stay in RX mode while filling TX FIFO
	// also need a public clearFIFO for TX and RX
	// this will also allow multiple transmissions without writing FIFO again
	// however, we wont know if the packet is corrupt until the whole thing has been transmitted/received - might run out of memory if its a large packet, unless its written to some external SPI RAM as its being received

	SI446X_ATOMIC()
	{
		// Load data to FIFO
		CHIPSELECT()
		{
			spi_transfer_nr(SI446X_CMD_WRITE_TX_FIFO);
			for(uint8_t i=0;i<len;i++)
				spi_transfer_nr(((uint8_t*)buff)[i]);
		}
	}
}
*/

#include <stdio.h>

uint8_t Si446x_TX(void* packet, uint8_t len, uint8_t channel, si446x_state_t onTxFinish)
{
	// TODO what happens if len is 0?

#if SI446X_FIXED_LENGTH
	// Stop the unused parameter warning
	((void)(len));
#endif

	SI446X_NO_INTERRUPT()
	{
		if(getState() == SI446X_STATE_TX) // Already transmitting
			return 0;

		// TODO collision avoid or maybe just do collision detect (RSSI jump)

		setState(IDLE_STATE);
		clearFIFO();
		interrupt2(NULL, 0, 0, 0xFF);

		SI446X_ATOMIC()
		{
			// Load data to FIFO
			CHIPSELECT()
			{
				spi_transfer_nr(SI446X_CMD_WRITE_TX_FIFO);
#if !SI446X_FIXED_LENGTH
				spi_transfer_nr(len);
				for(uint8_t i=0;i<len;i++)
					spi_transfer_nr(((uint8_t*)packet)[i]);
#else
				for(uint8_t i=0;i<SI446X_FIXED_LENGTH;i++)
					spi_transfer_nr(((uint8_t*)packet)[i]);
#endif
			}
		}

#if !SI446X_FIXED_LENGTH
		// Set packet length
		setProperty(SI446X_PKT_FIELD_2_LENGTH_LOW, len);
#endif

		// Begin transmit
		uint8_t data[] = {
			SI446X_CMD_START_TX,
			channel,
			(uint8_t)(onTxFinish<<4),
			0,
			SI446X_FIXED_LENGTH,
			0,
			0
		};
		doAPI(data, sizeof(data), NULL, 0);

#if !SI446X_FIXED_LENGTH
		// Reset packet length back to max for receive mode
		setProperty(SI446X_PKT_FIELD_2_LENGTH_LOW, MAX_PACKET_LEN);
#endif
	}
	return 1;
}

void Si446x_RX(uint8_t channel)
{
	SI446X_NO_INTERRUPT()
	{
		setState(IDLE_STATE);
		clearFIFO();
		//fix_invalidSync_irq(0);
		//Si446x_setupCallback(SI446X_CBS_INVALIDSYNC, 0);
		//setProperty(SI446X_PKT_FIELD_2_LENGTH_LOW, MAX_PACKET_LEN); // TODO ?
		interrupt2(NULL, 0, 0, 0xFF); // TODO needed?

		// TODO RX timeout to sleep if WUT LDC enabled

		uint8_t data[] = {
			SI446X_CMD_START_RX,
			channel,
			0,
			0,
			SI446X_FIXED_LENGTH,
			SI446X_STATE_NOCHANGE, // RX Timeout
			IDLE_STATE, // RX Valid
			SI446X_STATE_SLEEP // IDLE_STATE // RX Invalid (using SI446X_STATE_SLEEP for the INVALID_SYNC fix)
		};
		doAPI(data, sizeof(data), NULL, 0);
	}
}

uint16_t Si446x_adc_gpio(uint8_t pin)
{
	uint16_t result = getADC(SI446X_ADC_CONV_GPIO | pin, (SI446X_ADC_SPEED<<4) | SI446X_ADC_RANGE_3P6, 0);
	return result;
}

uint16_t Si446x_adc_battery()
{
	uint16_t result = getADC(SI446X_ADC_CONV_BATT, (SI446X_ADC_SPEED<<4), 2);
	result = ((uint32_t)result * 75) / 32; // result * 2.34375;
	return result;
}

float Si446x_adc_temperature()
{
	float result = getADC(SI446X_ADC_CONV_TEMP, (SI446X_ADC_SPEED<<4), 4);
	result = (899/4096.0) * result - 293;
	return result;
}

void Si446x_writeGPIO(si446x_gpio_t pin, uint8_t value)
{
	uint8_t data[] = {
		SI446X_CMD_GPIO_PIN_CFG,
		SI446X_GPIO_MODE_DONOTHING,
		SI446X_GPIO_MODE_DONOTHING,
		SI446X_GPIO_MODE_DONOTHING,
		SI446X_GPIO_MODE_DONOTHING,
		SI446X_NIRQ_MODE_DONOTHING,
		SI446X_SDO_MODE_DONOTHING,
		SI446X_GPIO_DRV_HIGH
	};
	data[pin + 1] = value;
	doAPI(data, sizeof(data), NULL, 0);
}

uint8_t Si446x_readGPIO()
{
	uint8_t data[4] = {
		SI446X_CMD_GPIO_PIN_CFG
	};
	doAPI(data, 1, data, sizeof(data));
	uint8_t states = data[0]>>7 | (data[1] & 0x80)>>6 | (data[2] & 0x80)>>5 | (data[3] & 0x80)>>4;
	return states;
}

uint8_t Si446x_dump(void* buff, uint8_t group)
{
	static const uint8_t groupSizes[] PROGMEM = {
		SI446X_PROP_GROUP_GLOBAL,	0x0A,
		SI446X_PROP_GROUP_INT,		0x04,
		SI446X_PROP_GROUP_FRR,		0x04,
		SI446X_PROP_GROUP_PREAMBLE,	0x0E,
		SI446X_PROP_GROUP_SYNC,		0x06,
		SI446X_PROP_GROUP_PKT,		0x40,
		SI446X_PROP_GROUP_MODEM,	0x60,
		SI446X_PROP_GROUP_MODEM_CHFLT,	0x24,
		SI446X_PROP_GROUP_PA,		0x07,
		SI446X_PROP_GROUP_SYNTH,	0x08,
		SI446X_PROP_GROUP_MATCH,	0x0C,
		SI446X_PROP_GROUP_FREQ_CONTROL,	0x08,
		SI446X_PROP_GROUP_RX_HOP,	0x42,
		SI446X_PROP_GROUP_PTI,		0x04
	};
	
	uint8_t length = 0;
	for(uint8_t i=0;i<sizeof(groupSizes);i+=2)
	{
		uint8_t buff[2];
		memcpy_P(buff, &groupSizes[i], sizeof(buff));

		if(buff[0] == group)
		{
			length = buff[1];
			break;
		}
	}

	if(buff == NULL)
		return length;

	for(uint8_t i=0;i<length;i+=16)
	{
		uint8_t count = length - i;
		if(count > 16)
			count = 16;
		getProperties((group<<8) | i, ((uint8_t*)buff) + i, count);
	}
	
	return length;
}

#if defined(ARDUINO) || SI446X_INTERRUPTS == 0
void Si446x_SERVICE()
#else
ISR(INT_VECTOR)
#endif
{
#if defined(ARDUINO) && (SI446X_INTERRUPTS == 1 || SI446X_INT_SPI_COMMS == 1)
	isrBusy = 1;
#endif

	uint8_t interrupts[8];
	interrupt(interrupts);

	// TODO remove
	//SI446X_CB_DEBUG(interrupts);

	//printf_P(PSTR("INT %hhu/%hhu %hhu/%hhu %hhu/%hhu\n"), interrupts[2], interrupts[3], interrupts[4], interrupts[5], interrupts[6], interrupts[7]);

	// We could read the enabled interrupts properties instead of keep their states in RAM, but that would be much slower
	interrupts[2] &= enabledInterrupts[IRQ_PACKET];
	interrupts[4] &= enabledInterrupts[IRQ_MODEM];
	interrupts[6] &= enabledInterrupts[IRQ_CHIP];

	// Valid PREAMBLE and SYNC, packet data now begins
	if(interrupts[4] & (1<<SI446X_SYNC_DETECT_PEND))
	{
		//fix_invalidSync_irq(1);
//		Si446x_setupCallback(SI446X_CBS_INVALIDSYNC, 1); // Enable INVALID_SYNC when a new packet starts, sometimes a corrupted packet will mess the radio up
		SI446X_CB_RXBEGIN(getLatchedRSSI());
	}
/*
	// Disable INVALID_SYNC
	if((interrupts[4] & (1<<SI446X_INVALID_SYNC_PEND)) || (interrupts[2] & ((1<<SI446X_PACKET_SENT_PEND)|(1<<SI446X_CRC_ERROR_PEND))))
	{
		//fix_invalidSync_irq(0);
		Si446x_setupCallback(SI446X_CBS_INVALIDSYNC, 0);
	}
*/

	// INVALID_SYNC detected, sometimes the radio gets messed up in this state and requires a RX restart
//	if(interrupts[4] & (1<<SI446X_INVALID_SYNC_PEND))
//		SI446X_CB_RXINVALIDSYNC();

#if SI446X_ENABLE_ADDRMATCHING
	// Address match success
	// NOTE: This will still be called even if the packet failed the CRC
	if(interrupts[2] & (1<<SI446X_FILTER_MATCH_PEND))
		SI446X_CB_ADDRMATCH();

	// Address match missed
	// NOTE: This will still be called even if the packet failed the CRC
	if(interrupts[2] & (1<<SI446X_FILTER_MISS_PEND))
		SI446X_CB_ADDRMISS();
#endif

	// Valid packet
	if(interrupts[2] & (1<<SI446X_PACKET_RX_PEND))
	{
#if !SI446X_FIXED_LENGTH
		uint8_t len = 0;
		Si446x_read(&len, 1);
#else
		uint8_t len = SI446X_FIXED_LENGTH;
#endif
		SI446X_CB_RXCOMPLETE(len, getLatchedRSSI());
	}

	// Corrupted packet
	// NOTE: This will still be called even if the address did not match, but the packet failed the CRC
	// This will not be called if the address missed, but the packet passed CRC
	if(interrupts[2] & (1<<SI446X_CRC_ERROR_PEND))
	{
#if IDLE_STATE == SI446X_STATE_READY
		if(getState() == SI446X_STATE_SPI_ACTIVE)
			setState(IDLE_STATE); // We're in sleep mode (acually, we're now in SPI active mode) after an invalid packet to fix the INVALID_SYNC issue
#endif
		SI446X_CB_RXINVALID(getLatchedRSSI()); // TODO remove RSSI stuff for invalid packets, entering SLEEP mode looses the latched value?
	}

	// Packet sent
	if(interrupts[2] & (1<<SI446X_PACKET_SENT_PEND))
		SI446X_CB_SENT();

	if(interrupts[6] & (1<<SI446X_LOW_BATT_PEND))
		SI446X_CB_LOWBATT();

	if(interrupts[6] & (1<<SI446X_WUT_PEND))
		SI446X_CB_WUT();

#if defined(ARDUINO) && (SI446X_INTERRUPTS == 1 || SI446X_INT_SPI_COMMS == 1)
	isrBusy = 0;
#endif
}
