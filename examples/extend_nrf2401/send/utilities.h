
#pragma once

#define BOARD_PWR_EN   15

#define BOARD_SDA_PIN                       18
#define BOARD_SCL_PIN                       8

// --------- SPI ---------
#define BOARD_SPI_SCK  11
#define BOARD_SPI_MOSI 9
#define BOARD_SPI_MISO 10

// TFT screen
#define TFT_BL     21   // LED back-light
#define TFT_MISO   10   
#define TFT_MOSI   9
#define TFT_SCLK   11
#define TFT_CS     41 
#define TFT_DC     16
#define TFT_RST    -1 // Connect reset to ensure display initialises

// TF card
#define BOARD_SD_CS   13
#define BOARD_SD_SCK  BOARD_SPI_SCK
#define BOARD_SD_MOSI BOARD_SPI_MOSI
#define BOARD_SD_MISO BOARD_SPI_MISO

// LORA
#define BOARD_LORA_CS   12
#define BOARD_LORA_SCK  BOARD_SPI_SCK
#define BOARD_LORA_MOSI BOARD_SPI_MOSI
#define BOARD_LORA_MISO BOARD_SPI_MISO
#define BOARD_LORA_IO2  38
#define BOARD_LORA_IO0  3
#define BOARD_LORA_SW1  47
#define BOARD_LORA_SW0  48

// NRF24 
#define BOARD_NRF24_CS   44
#define BOARD_NRF24_CE   43
#define BOARD_NRF24_SCK  BOARD_SPI_SCK  // 11
#define BOARD_NRF24_MOSI BOARD_SPI_MOSI // 9
#define BOARD_NRF24_MISO BOARD_SPI_MISO // 10









