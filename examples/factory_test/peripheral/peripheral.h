#pragma once
#include "../utilities.h"
#include "FS.h"
#include "SPIFFS.h"

/**----------------------------- WS2812 ----------------------------------**/
#include <FastLED.h>
#define WS2812_DEFAULT_LIGHT 10
extern TaskHandle_t ws2812_handle;

void ws2812_init(void);
void ws2812_set_color(CRGB c);
void ws2812_set_light(uint8_t light);
void ws2812_set_mode(int m);
int ws2812_get_mode(void);
void ws2812_task(void *param);

/**------------------------------- NFC -----------------------------------**/
// PN532
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
extern TaskHandle_t nfc_handle;
void nfc_init(void);
bool nfc_is_init(void);
uint32_t nfc_get_ver_data(void);
void nfc_task(void *param);

/**------------------------------ LORA -----------------------------------**/
// CC1101
#include <RadioLib.h>

#define LORA_MODE_SEND 1
#define LORA_MODE_RECV 2

extern TaskHandle_t lora_handle;
extern SemaphoreHandle_t radioLock;
extern CC1101 radio;

void lora_init(void);
void lora_mode_sw(int m);
int lora_get_mode(void);
bool lora_is_init(void);
void lora_send(const char *str);
void lora_task(void *param);

/**---------------------------- BATTERY ----------------------------------**/
#include "XPowersLib.h"

extern TaskHandle_t battery_handle;
extern XPowersPPM PPM;
void battery_task(void *param);

#include "bq27220.h"
extern BQ27220 bq27220;

/**------------------------------ MIC ------------------------------------**/
#define SAMPLE_SIZE         (20)
#define EXAMPLE_I2S_CH      0        // I2S Channel Number

void init_microphone(void);

/**------------------------------- IR ------------------------------------**/
#define IR_MODE_SEND 1
#define IR_MODE_RECV 2
// void infared_init();
// uint16_t infared_get_cmd(void);
// void infared_task(void *param);

/**------------------------------- SD ------------------------------------**/
#include "FS.h"
#include "SD.h"
#include "SPI.h"
extern bool sd_init_flag;
void sd_init(void);
bool sd_is_valid(void);
uint32_t sd_get_sum_Mbyte(void);
uint32_t sd_get_used_Mbyte(void);

/**----------------------------- WIFI ------------------------------------**/
#include <WiFi.h>
#define WIFI_SSID_MAX_LEN 30
#define WIFI_PSWD_MAX_LEN 30
extern char wifi_ssid[WIFI_SSID_MAX_LEN];
extern char wifi_password[WIFI_PSWD_MAX_LEN];
extern bool wifi_is_connect;

/**---------------------------- EEPROM -----------------------------------**/
#include <EEPROM.h>
#define EEPROM_UPDATA_FLAG_NUM   0xAA
#define WIFI_SSID_EEPROM_ADDR    (1)
#define WIFI_PSWD_EEPROM_ADDR    (WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN)
#define UI_THEME_EEPROM_ADDR     (WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN)
#define UI_ROTATION_EEPROM_ADDR  (UI_THEME_EEPROM_ADDR + 1)
#define EEPROM_SIZE_MAX 128

void eeprom_wr(int addr, uint8_t val);
void eeprom_wr_wifi(const char *ssid, uint16_t ssid_len, const char *pwsd, uint16_t pwsd_len);