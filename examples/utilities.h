#pragma once

// Shared board definitions for the T-Embed PN532 examples.

#define T_EMBED_CC1101_HD_VER "v1.0-240729"

#define BOARD_USER_KEY 6
#define BOARD_PWR_EN   15

// WS2812
#define WS2812_NUM_LEDS 8
#define WS2812_DATA_PIN 14

// IR
#define BOARD_IR_EN 2
#define BOARD_IR_TX BOARD_IR_EN
#define BOARD_IR_RX 1

// MIC
#define BOARD_MIC_DATA 42
#define BOARD_MIC_CLK  39

// VOICE
#define BOARD_VOICE_BCLK  46
#define BOARD_VOICE_LRCLK 40
#define BOARD_VOICE_DIN   7

// --------- DISPLAY ---------
// About LCD definition in the file:
// lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h
#define DISPLAY_WIDTH  170
#define DISPLAY_HEIGHT 320

#define DISPLAY_BL   21
#define DISPLAY_CS   41
#define DISPLAY_MISO 10
#define DISPLAY_MOSI 9
#define DISPLAY_SCLK 11
#define DISPLAY_DC   16
#define DISPLAY_RST  -1

// --------- ENCODER ---------
#define ENCODER_INA 4
#define ENCODER_INB 5
#define ENCODER_KEY 0

// --------- I2C ---------
#define BOARD_I2C_SDA 8
#define BOARD_I2C_SCL 18

// Legacy names used by the CC1101 / Si4463 examples.
#define BOARD_SDA_PIN 18
#define BOARD_SCL_PIN 8

// I2C addr
#define BOARD_I2C_ADDR_1 0x24  // PN532
#define BOARD_I2C_ADDR_2 0x55  // PMU
#define BOARD_I2C_ADDR_3 0x6b  // BQ25896

// PN532
#define BOARD_PN532_SCL     BOARD_I2C_SCL
#define BOARD_PN532_SDA     BOARD_I2C_SDA
#define BOARD_PN532_RF_REST 45
#define BOARD_PN532_IRQ     17

// --------- SPI ---------
#define BOARD_SPI_SCK  11
#define BOARD_SPI_MOSI 9
#define BOARD_SPI_MISO 10

// nRF24L01
#define BOARD_NRF24_CS   44
#define BOARD_NRF24_CE   43
#define BOARD_NRF24_IRQ  -1
#define BOARD_NRF24_SCK  BOARD_SPI_SCK
#define BOARD_NRF24_MOSI BOARD_SPI_MOSI
#define BOARD_NRF24_MISO BOARD_SPI_MISO

// TF card
#define BOARD_SD_CS   13
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// LORA / CC1101 shared bus pins
#define BOARD_LORA_CS   12
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_IO2  38
#define BOARD_LORA_IO0  3
#define BOARD_LORA_SW1  47
#define BOARD_LORA_SW0  48

// Si4463
#define BOARD_Si4463_CS   12
#define BOARD_Si4463_SCK  BOARD_SPI_SCK
#define BOARD_Si4463_MOSI BOARD_SPI_MOSI
#define BOARD_Si4463_MISO BOARD_SPI_MISO
#define BOARD_Si4463_IRQ  38
#define BOARD_Si4463_SW1  48

void board_spi_init_shared_bus(void);
void board_spi_deselect_all(void);
void board_spi_prepare_display(void);
void board_spi_prepare_lora(void);
void board_spi_prepare_nrf24(void);
void board_spi_prepare_sd(void);
