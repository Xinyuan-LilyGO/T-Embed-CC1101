#pragma once
#include "../utilities.h"

/**----------------------------- WS2812 ----------------------------------**/
#include <FastLED.h>
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
void lora_task(void *param);



/**------------------------------ LORA -----------------------------------**/
// CC1101
#include <RadioLib.h>
extern TaskHandle_t lora_handle;
extern SemaphoreHandle_t radioLock;

void lora_init(void);
void lora_send(const char *str);
void lora_task(void *param);


/**------------------------------ LVGL -----------------------------------**/
extern TaskHandle_t ui_handle;

