/*
 * Project: Si4463 Radio Library for AVR and Arduino
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

#ifndef SI443X_H_
#define SI443X_H_

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include <stdint.h>

#include "Si446x_config.h"

// Address matching doesnt really work very well as the FIFO still needs to be
// manually cleared after receiving a packet, so the MCU still needs to wakeup and
// do stuff instead of the radio doing things automatically :/
#if !DOXYGEN
#define SI446X_ENABLE_ADDRMATCHING		0
#endif

#define SI446X_MAX_PACKET_LEN	128 ///< Maximum packet length

#define SI446X_MAX_TX_POWER		127 ///< Maximum TX power (+20dBm/100mW)

#define SI446X_WUT_RUN	1 ///< Wake the microcontroller when the WUT expires
#define SI446X_WUT_BATT	2 ///< Take a battery measurement when the WUT expires
#define SI446X_WUT_RX	4 ///< Go into RX mode for LDC time (not supported yet!)

#define SI446X_GPIO_PULL_EN		0x40 ///< Pullup enable for GPIO pins
#define SI446X_GPIO_PULL_DIS	0x00 ///< Pullup disable for GPIO pins
#define SI446X_NIRQ_PULL_EN		0x40 ///< Pullup enable for NIRQ pin
#define SI446X_NIRQ_PULL_DIS	0x00 ///< Pullup disable for NIRQ pin
#define SI446X_SDO_PULL_EN		0x40 ///< Pullup enable for SDO pin
#define SI446X_SDO_PULL_DIS		0x00 ///< Pullup disable for SDO pin
#define SI446X_PIN_PULL_EN		0x40 ///< Pullup enable for any pin
#define SI446X_PIN_PULL_DIS		0x00 ///< Pullup disable for any pin

#define SI446X_GPIO_DRV_HIGH		0x00 ///< GPIO drive strength high
#define SI446X_GPIO_DRV_MED_HIGH	0x20 ///< GPIO drive strength medium-high
#define SI446X_GPIO_DRV_MED_LOW		0x40 ///< GPIO drive strength medium-low
#define SI446X_GPIO_DRV_LOW			0x60 ///< GPIO drive strength low

#define SI446X_PROP_GROUP_GLOBAL		0x00 ///< Property group global
#define SI446X_PROP_GROUP_INT			0x01 ///< Property group interrupts
#define SI446X_PROP_GROUP_FRR			0x02 ///< Property group fast response registers
#define SI446X_PROP_GROUP_PREAMBLE		0x10 ///< Property group preamble
#define SI446X_PROP_GROUP_SYNC			0x11 ///< Property group sync
#define SI446X_PROP_GROUP_PKT			0x12 ///< Property group packet config
#define SI446X_PROP_GROUP_MODEM			0x20 ///< Property group modem
#define SI446X_PROP_GROUP_MODEM_CHFLT	0x21 ///< Property group RX coefficients
#define SI446X_PROP_GROUP_PA			0x22 ///< Property group power amp
#define SI446X_PROP_GROUP_SYNTH			0x23 ///< Property group synthesizer 
#define SI446X_PROP_GROUP_MATCH			0x30 ///< Property group address match
#define SI446X_PROP_GROUP_FREQ_CONTROL	0x40 ///< Property group frequency control
#define SI446X_PROP_GROUP_RX_HOP		0x50 ///< Property group RX hop
#define SI446X_PROP_GROUP_PTI			0xF0 ///< Property group packet trace interface

/**
* @brief GPIO pin modes (see the Si446x API docs for what they all mean)
*/
typedef enum
{
	SI446X_GPIO_MODE_DONOTHING	= 0x00,
	SI446X_GPIO_MODE_TRISTATE	= 0x01,
	SI446X_GPIO_MODE_DRIVE0		= 0x02,
	SI446X_GPIO_MODE_DRIVE1		= 0x03,
	SI446X_GPIO_MODE_INPUT		= 0x04,
	SI446X_GPIO_MODE_32K_CLK	= 0x05,
	SI446X_GPIO_MODE_BOOT_CLK	= 0x06,
	SI446X_GPIO_MODE_DIV_CLK	= 0x07,
	SI446X_GPIO_MODE_CTS		= 0x08,
	SI446X_GPIO_MODE_INV_CTS	= 0x09,
	SI446X_GPIO_MODE_CMD_OVERLAP	= 0x0A,
	SI446X_GPIO_MODE_SDO		= 0x0B,
	SI446X_GPIO_MODE_POR		= 0x0C,
	SI446X_GPIO_MODE_CAL_WUT	= 0x0D,
	SI446X_GPIO_MODE_WUT		= 0x0E,
	SI446X_GPIO_MODE_EN_PA		= 0x0F,
	SI446X_GPIO_MODE_TX_DATA_CLK	= 0x10,
	SI446X_GPIO_MODE_RX_DATA_CLK	= 0x11,
	SI446X_GPIO_MODE_EN_LNA			= 0x12,
	SI446X_GPIO_MODE_TX_DATA		= 0x13,
	SI446X_GPIO_MODE_RX_DATA		= 0x14,
	SI446X_GPIO_MODE_RX_RAW_DATA		= 0x15,
	SI446X_GPIO_MODE_ANTENNA_1_SW		= 0x16,
	SI446X_GPIO_MODE_ANTENNA_2_SW		= 0x17,
	SI446X_GPIO_MODE_VALID_PREAMBLE		= 0x18,
	SI446X_GPIO_MODE_INVALID_PREAMBLE	= 0x19,
	SI446X_GPIO_MODE_SYNC_WORD_DETECT	= 0x1A,
	SI446X_GPIO_MODE_CCA			= 0x1B,
	SI446X_GPIO_MODE_IN_SLEEP		= 0x1C,
	SI446X_GPIO_MODE_PKT_TRACE		= 0x1D,
// Nothing for 0x1E (30)
	SI446X_GPIO_MODE_TX_RX_DATA_CLK	= 0x1F,
	SI446X_GPIO_MODE_TX_STATE		= 0x20,
	SI446X_GPIO_MODE_RX_STATE		= 0x21,
	SI446X_GPIO_MODE_RX_FIFO_FULL	= 0x22,
	SI446X_GPIO_MODE_TX_FIFO_EMPTY	= 0x23,
	SI446X_GPIO_MODE_LOW_BATT		= 0x24,
	SI446X_GPIO_MODE_CCA_LATCH		= 0x25,
	SI446X_GPIO_MODE_HOPPED			= 0x26,
	SI446X_GPIO_MODE_HOP_TABLE_WRAP	= 0x27
} si446x_gpio_mode_t;

/**
* @brief NIRQ pin modes (see the Si446x API docs for what they all mean)
*/
typedef enum
{
	SI446X_NIRQ_MODE_DONOTHING	= 0x00,
	SI446X_NIRQ_MODE_TRISTATE	= 0x01,
	SI446X_NIRQ_MODE_DRIVE0		= 0x02,
	SI446X_NIRQ_MODE_DRIVE1		= 0x03,
	SI446X_NIRQ_MODE_INPUT		= 0x04,
//	SI446X_NIRQ_MODE_32K_CLK	= 0x05,
//	SI446X_NIRQ_MODE_BOOT_CLK	= 0x06,
	SI446X_NIRQ_MODE_DIV_CLK	= 0x07,
	SI446X_NIRQ_MODE_CTS		= 0x08,
//	SI446X_NIRQ_MODE_INV_CTS	= 0x09,
//	SI446X_NIRQ_MODE_CMD_OVERLAP	= 0x0A,
	SI446X_NIRQ_MODE_SDO		= 0x0B,
	SI446X_NIRQ_MODE_POR		= 0x0C,
//	SI446X_NIRQ_MODE_CAL_WUT	= 0x0D,
//	SI446X_NIRQ_MODE_WUT		= 0x0E,
	SI446X_NIRQ_MODE_EN_PA		= 0x0F,
	SI446X_NIRQ_MODE_TX_DATA_CLK	= 0x10,
	SI446X_NIRQ_MODE_RX_DATA_CLK	= 0x11,
	SI446X_NIRQ_MODE_EN_LNA			= 0x12,
	SI446X_NIRQ_MODE_TX_DATA		= 0x13,
	SI446X_NIRQ_MODE_RX_DATA		= 0x14,
	SI446X_NIRQ_MODE_RX_RAW_DATA	= 0x15,
	SI446X_NIRQ_MODE_ANTENNA_1_SW	= 0x16,
	SI446X_NIRQ_MODE_ANTENNA_2_SW	= 0x17,
	SI446X_NIRQ_MODE_VALID_PREAMBLE	= 0x18,
	SI446X_NIRQ_MODE_INVALID_PREAMBLE	= 0x19,
	SI446X_NIRQ_MODE_SYNC_WORD_DETECT	= 0x1A,
	SI446X_NIRQ_MODE_CCA			= 0x1B,
//	SI446X_NIRQ_MODE_IN_SLEEP		= 0x1C,
	SI446X_NIRQ_MODE_PKT_TRACE		= 0x1D,
// Nothing for 0x1E (30)
	SI446X_NIRQ_MODE_TX_RX_DATA_CLK	= 0x1F,
//	SI446X_NIRQ_MODE_TX_STATE		= 0x20,
//	SI446X_NIRQ_MODE_RX_STATE		= 0x21,
//	SI446X_NIRQ_MODE_RX_FIFO_FULL	= 0x22,
//	SI446X_NIRQ_MODE_TX_FIFO_EMPTY	= 0x23,
//	SI446X_NIRQ_MODE_LOW_BATT		= 0x24,
//	SI446X_NIRQ_MODE_CCA_LATCH		= 0x25,
//	SI446X_NIRQ_MODE_HOPPED			= 0x26,
	SI446X_NIRQ_MODE_NIRQ			= 0x27
} si446x_nirq_mode_t;

/**
* @brief SDO pin modes (see the Si446x API docs for what they all mean)
*/
typedef enum
{
	SI446X_SDO_MODE_DONOTHING	= 0x00,
	SI446X_SDO_MODE_TRISTATE	= 0x01,
	SI446X_SDO_MODE_DRIVE0		= 0x02,
	SI446X_SDO_MODE_DRIVE1		= 0x03,
	SI446X_SDO_MODE_INPUT		= 0x04,
	SI446X_SDO_MODE_32K_CLK	= 0x05,
//	SI446X_SDO_MODE_BOOT_CLK	= 0x06,
	SI446X_SDO_MODE_DIV_CLK	= 0x07,
	SI446X_SDO_MODE_CTS		= 0x08,
//	SI446X_SDO_MODE_INV_CTS	= 0x09,
//	SI446X_SDO_MODE_CMD_OVERLAP	= 0x0A,
	SI446X_SDO_MODE_SDO		= 0x0B,
	SI446X_SDO_MODE_POR		= 0x0C,
//	SI446X_SDO_MODE_CAL_WUT	= 0x0D,
	SI446X_SDO_MODE_WUT		= 0x0E,
	SI446X_SDO_MODE_EN_PA		= 0x0F,
	SI446X_SDO_MODE_TX_DATA_CLK	= 0x10,
	SI446X_SDO_MODE_RX_DATA_CLK	= 0x11,
	SI446X_SDO_MODE_EN_LNA			= 0x12,
	SI446X_SDO_MODE_TX_DATA			= 0x13,
	SI446X_SDO_MODE_RX_DATA			= 0x14,
	SI446X_SDO_MODE_RX_RAW_DATA		= 0x15,
	SI446X_SDO_MODE_ANTENNA_1_SW		= 0x16,
	SI446X_SDO_MODE_ANTENNA_2_SW		= 0x17,
	SI446X_SDO_MODE_VALID_PREAMBLE		= 0x18,
	SI446X_SDO_MODE_INVALID_PREAMBLE	= 0x19,
	SI446X_SDO_MODE_SYNC_WORD_DETECT	= 0x1A,
	SI446X_SDO_MODE_CCA			= 0x1B,
//	SI446X_SDO_MODE_IN_SLEEP		= 0x1C,
//	SI446X_SDO_MODE_PKT_TRACE		= 0x1D,
// Nothing for 0x1E (30)
//	SI446X_SDO_MODE_TX_RX_DATA_CLK	= 0x1F,
//	SI446X_SDO_MODE_TX_STATE		= 0x20,
//	SI446X_SDO_MODE_RX_STATE		= 0x21,
//	SI446X_SDO_MODE_RX_FIFO_FULL	= 0x22,
//	SI446X_SDO_MODE_TX_FIFO_EMPTY	= 0x23,
//	SI446X_SDO_MODE_LOW_BATT		= 0x24,
//	SI446X_SDO_MODE_CCA_LATCH		= 0x25,
//	SI446X_SDO_MODE_HOPPED			= 0x26,
//	SI446X_SDO_MODE_HOP_TABLE_WRAP	= 0x27
} si446x_sdo_mode_t;

/**
* @brief Data structure for storing chip info from ::Si446x_getInfo()
*/
typedef struct {
	uint8_t chipRev; ///< Chip revision
	uint16_t part; ///< Part ID
	uint8_t partBuild; ///< Part build
	uint16_t id; ///< ID
	uint8_t customer; ///< Customer
	uint8_t romId; ///< ROM ID (3 = revB1B, 6 = revC2A)
	
	uint8_t revExternal; ///< Revision external
	uint8_t revBranch; ///< Revision branch
	uint8_t revInternal; ///< Revision internal
	uint16_t patch; ///< Patch
	uint8_t func; ///< Function
} si446x_info_t;

/**
* @brief GPIOs for passing to ::Si446x_writeGPIO(), or for masking when reading from ::Si446x_readGPIO()
*/
typedef enum
{
	SI446X_GPIO0 = 0, ///< GPIO 1
	SI446X_GPIO1 = 1, ///< GPIO 2
	SI446X_GPIO2 = 2, ///< GPIO 3
	SI446X_GPIO3 = 3, ///< GPIO 4
	SI446X_NIRQ = 4, ///< NIRQ
	SI446X_SDO = 5 ///< SDO
} si446x_gpio_t;

/**
* @brief Radio states, returned from ::Si446x_getState()
*/
typedef enum
{
	SI446X_STATE_NOCHANGE	= 0x00,
	SI446X_STATE_SLEEP		= 0x01, ///< This will never be returned since SPI activity will wake the radio into ::SI446X_STATE_SPI_ACTIVE
	SI446X_STATE_SPI_ACTIVE	= 0x02,
	SI446X_STATE_READY		= 0x03,
	SI446X_STATE_READY2		= 0x04, ///< Will return as ::SI446X_STATE_READY
	SI446X_STATE_TX_TUNE	= 0x05, ///< Will return as ::SI446X_STATE_TX
	SI446X_STATE_RX_TUNE	= 0x06, ///< Will return as ::SI446X_STATE_RX
	SI446X_STATE_TX			= 0x07,
	SI446X_STATE_RX			= 0x08
} si446x_state_t;

#if SI446X_ENABLE_ADDRMATCHING
/*-*
* @brief Address modes (NOT SUPPORTED)
*/
typedef enum
{
	SI446X_ADDRMODE_DISABLE,		///< Disable address matching
	SI446X_ADDRMODE_ADDR,			///< Only match exact address (contained in the first byte of the received data)
	SI446X_ADDRMODE_ADDRBROADCAST	///< Match exact address or broadcast flag (broascast flag is the MSB bit of the second byte in the received data)
} si446x_addrMode_t;

#define SI446X_CBS_ADDRMATCH		_BV(7+8)
#define SI446X_CBS_ADDRMISS			_BV(6+8)
#endif

#define SI446X_CBS_SENT				_BV(5+8) ///< Enable/disable packet sent callback
//#define SI446X_CBS_RXCOMPLETE		_BV(4+8)
//#define SI446X_CBS_RXINVALID		_BV(3+8)
#define SI446X_CBS_RXBEGIN			_BV(0) ///< Enable/disable packet receive begin callback
//#define SI446X_CBS_INVALIDSYNC		_BV(5) ///< Don't use this, it's used internally by the library

#if defined(__cplusplus)
extern "C" {
#endif

/**
* @brief Initialise, must be called before anything else!
*
* @return (none)
*/
void Si446x_init(void);

/**
* @brief Get chip info, see ::si446x_info_t
*
* @see ::si446x_info_t
* @param [info] Pointer to allocated ::si446x_info_t struct to place data into
* @return (none)
*/
void Si446x_getInfo(si446x_info_t* info);

/**
* @brief Get the current RSSI, the chip needs to be in receive mode for this to work
*
* @return The current RSSI in dBm (usually between -130 and 0)
*/
int16_t Si446x_getRSSI(void);

/**
* @brief Set the transmit power. The output power does not follow the \p pwr value, see the Si446x datasheet for a pretty graph
*
* 0 = -32dBm (<1uW)\n
* 7 = 0dBm (1mW)\n
* 12 = 5dBm (3.2mW)\n
* 22 = 10dBm (10mW)\n
* 40 = 15dBm (32mW)\n
* 100 = 20dBm (100mW)
*
* @param [pwr] A value from 0 to 127
* @return (none)
*/
void Si446x_setTxPower(uint8_t pwr);

/**
* @brief Enable or disable callbacks. This is mainly to configure what events should wake the microcontroller up.
*
* @param [callbacks] The callbacks to configure (multiple callbacks should be bitewise OR'd together)
* @param [state] Enable or disable the callbacks passed in \p callbacks parameter (1 = Enable, 0 = Disable)
* @return (none)
*/
void Si446x_setupCallback(uint16_t callbacks, uint8_t state);

/**
* @brief Read received data from FIFO
*
* @param [buff] Pointer to buffer to place data
* @param [len] Number of bytes to read, make sure not to read more bytes than what the FIFO has stored. The number of bytes that can be read is passed in the ::SI446X_CB_RXCOMPLETE() callback.
* @return (none)
*/
void Si446x_read(void* buff, uint8_t len);

/**
* @brief Transmit a packet
*
* @param [packet] Pointer to packet data
* @param [len] Number of bytes to transmit, maximum of ::SI446X_MAX_PACKET_LEN If configured for fixed length packets then this parameter is ignored and the length is set by ::SI446X_FIXED_LENGTH in Si446x_config.h
* @param [channel] Channel to transmit data on (0 - 255)
* @param [onTxFinish] What state to enter when the packet has finished transmitting. Usually ::SI446X_STATE_SLEEP or ::SI446X_STATE_RX
* @return 0 on failure (already transmitting), 1 on success (has begun transmitting)
*/
uint8_t Si446x_TX(void* packet, uint8_t len, uint8_t channel, si446x_state_t onTxFinish);

/**
* @brief Enter receive mode
*
* Entering RX mode will abort any transmissions happening at the time
*
* @param [channel] Channel to listen to (0 - 255)
* @return (none)
*/
void Si446x_RX(uint8_t channel);

/*-*
* @brief Changes will be applied next time the radio enters RX mode (NOT SUPPORTED)
*
* @param [mode] TODO
* @param [address] TODO
* @return (none)
*/
//void Si446x_setAddress(si446x_addrMode_t mode, uint8_t address);

/**
* @brief Set the low battery voltage alarm
*
* The ::SI446X_CB_LOWBATT() callback will be ran when the supply voltage drops below this value. The WUT must be configured with ::Si446x_setupWUT() to enable periodically checking the battery level.
*
* @param [voltage] The low battery threshold in millivolts (1050 - 3050).
* @return (none)
*/
void Si446x_setLowBatt(uint16_t voltage);

/**
* @brief Configure the wake up timer
* 
* This function will also reset the timer.\n
*\n
* The Wake Up Timer (WUT) can be used to periodically run a number of features:\n
* ::SI446X_WUT_RUN Simply wake up the microcontroller when the WUT expires and run the ::SI446X_CB_WUT() callback.\n
* ::SI446X_WUT_BATT Check battery voltage - If the battery voltage is below the threshold set by ::Si446x_setLowBatt() then wake up the microcontroller and run the ::SI446X_CB_LOWBATT() callback.\n
* ::SI446X_WUT_RX Enter receive mode for a length of time determinded by the ldc and r parameters (NOT SUPPORTED YET! dont use this option)\n
*\n
* For more info see the GLOBAL_WUT_M, GLOBAL_WUT_R and GLOBAL_WUT_LDC properties in the Si446x API docs.\n
*
* @note When first turning on the WUT this function will take around 300us to complete
* @param [r] Exponent value for WUT and LDC (Maximum valus is 20)
* @param [m] Mantissia value for WUT
* @param [ldc] Mantissia value for LDC (NOT SUPPORTED YET, just pass 0 for now)
* @param [config] Which WUT features to enable ::SI446X_WUT_RUN ::SI446X_WUT_BATT ::SI446X_WUT_RX These can be bitwise OR'ed together to enable multiple features.
* @return (none)
*/
void Si446x_setupWUT(uint8_t r, uint16_t m, uint8_t ldc, uint8_t config);

/**
* @brief Disable the wake up timer
*
* @return (none)
*/
void Si446x_disableWUT(void);

/**
* @brief Enter sleep mode
*
* If WUT is enabled then the radio will keep the internal 32KHz RC enabled with a current consumption of 740nA, otherwise the current consumption will be 40nA without WUT.
* Sleep will fail if the radio is currently transmitting.
*
* @note Any SPI communications with the radio will wake the radio into ::SI446X_STATE_SPI_ACTIVE mode. ::Si446x_sleep() will need to called again to put it back into sleep mode.
*
* @return 0 on failure (busy transmitting something), 1 on success
*/
uint8_t Si446x_sleep(void);

/**
* @brief Get the radio status
*
* @see ::si446x_state_t
* @return The current radio status
*/
si446x_state_t Si446x_getState(void);

/**
* @brief Read pin ADC value
*
* @param [pin] The GPIO pin number (0 - 3)
* @return ADC value (0 - 2048, where 2048 is 3.6V)
*/
uint16_t Si446x_adc_gpio(uint8_t pin);

/**
* @brief Read supply voltage
*
* @return Supply voltage in millivolts
*/
uint16_t Si446x_adc_battery(void);

/**
* @brief Read temperature
*
* @return Temperature in C
*/
float Si446x_adc_temperature(void);

/**
* @brief Configure GPIO/NIRQ/SDO pin
*
* @note NIRQ and SDO pins should not be changed, unless you really know what you're doing. 2 of the GPIO pins (usually 0 and 1) are also usually used for the RX/TX RF switch and should also be left alone.
*
* @param [pin] The pin, this can only take a single pin (don't use bitwise OR), see ::si446x_gpio_t
* @param [value] The new pin mode, this can be bitwise OR'd with the ::SI446X_PIN_PULL_EN option, see ::si446x_gpio_mode_t ::si446x_nirq_mode_t ::si446x_sdo_mode_t
* @return (none)
*/
void Si446x_writeGPIO(si446x_gpio_t pin, uint8_t value);

/**
* @brief Read GPIO pin states
*
* @return The pin states. Use ::si446x_gpio_t to mask the value to get the state for the desired pin.
*/
uint8_t Si446x_readGPIO(void);

/**
* @brief Get all values of a property group
*
* @param [buff] Pointer to memory to place group values, if this is NULL then nothing will be dumped, just the group size is returned
* @param [group] The group to dump
* @return Size of the property group
*/
uint8_t Si446x_dump(void* buff, uint8_t group);

/**
* @brief If interrupts are disabled (::SI446X_INTERRUPTS in Si446x_config.h) then this function should be called as often as possible to process any events
*
* @return (none)
*/
#if DOXYGEN || defined(ARDUINO) || SI446X_INTERRUPTS == 0
void Si446x_SERVICE(void);
#else
#define Si446x_SERVICE() ((void)(0))
#endif

#if SI446X_ENABLE_ADDRMATCHING
/*-*
* @brief Set the address in the first byte of the payload TODO
*
* @return (none)
*/
//inline void Si446x_setPacketAddress(uint8_t* data, uint8_t addr)
//{
//	data[0] = addr;
//}

/*-*
* @brief Set the MSB in the 2nd byte of the payload to signify that this is a broadcast transmission TODO
*
* @return (none)
*/
//inline void Si446x_setBroadcast(uint8_t* data)
//{
//	data[1] |= 0x80;
//}

/*-*
* @brief TODO (NOT SUPPORTED)
*
* @return (none)
*/
//inline void Si446x_clearBroadcast(uint8_t* data)
//{
//	data[1] &= ~(0x80);
//}

/*-*
* @brief TODO (NOT SUPPORTED)
*
* @return TODO
*/
//inline uint8_t Si446x_isBroadcast(uint8_t* data)
//{
//	return !!(data[1] & 0x80);
//}

/*-*
* @brief TODO (NOT SUPPORTED)
*
* @return The destination address of the packet
*/
//inline uint8_t Si446x_getPacketAddress(uint8_t* data)
//{
//	return data[0];
//}
#endif

/**
* @brief When using interrupts use this to disable them for the Si446x
*
* Ideally you should wrap sensitive sections with ::SI446X_NO_INTERRUPT() instead, as it automatically deals with this function and ::Si446x_irq_on()
*
* @see ::Si446x_irq_on() and ::SI446X_NO_INTERRUPT()
* @return The previous interrupt status; 1 if interrupt was enabled, 0 if it was already disabled
*/
uint8_t Si446x_irq_off(void);

/**
* @brief When using interrupts use this to re-enable them for the Si446x
*
* Ideally you should wrap sensitive sections with ::SI446X_NO_INTERRUPT() instead, as it automatically deals with this function and ::Si446x_irq_off()
*
* @see ::Si446x_irq_off() and ::SI446X_NO_INTERRUPT()
* @param [origVal] The original interrupt status returned from ::Si446x_irq_off()
* @return (none)
*/
void Si446x_irq_on(uint8_t origVal);

#if DOXYGEN || SI446X_INTERRUPTS != 0

#if !defined(DOXYGEN)
static inline void _Si446x_iRestore(const  uint8_t *__s)
{
	Si446x_irq_on(*__s);
	__asm__ volatile ("" ::: "memory");
}
#endif

/**
* @brief Disable Si446x interrupts for code inside this block
*
* When communicating with other SPI devices on the same bus as the radio then you should wrap those sections in a ::SI446X_NO_INTERRUPT() block, this will stop the Si446x interrupt from running and trying to use the bus at the same time.
* This macro is based on the code from avr/atomic.h, and wraps the ::Si446x_irq_off() and ::Si446x_irq_on() functions instead of messing with global interrupts. It is safe to return, break or continue inside an ::SI446X_NO_INTERRUPT() block.
*
* Example:
*
* Si446x_RX(63);\n
* SI446X_NO_INTERRUPT()\n
* {\n
* 	OLED.write("blah", 2, 10); // Communicate with SPI OLED display\n
* }\n
*/
#define SI446X_NO_INTERRUPT() \
	for(uint8_t si446x_irq __attribute__((__cleanup__(_Si446x_iRestore))) = Si446x_irq_off(), \
	si446x_tmp = 1; si446x_tmp ; si446x_tmp = 0)

#else
#define SI446X_NO_INTERRUPT() ((void)(0));
#endif

#if defined(__cplusplus)
}
#endif

#endif /* SI443X_H_ */
