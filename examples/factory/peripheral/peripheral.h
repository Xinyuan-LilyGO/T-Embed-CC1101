#pragma once
#include "utilities.h"
/**----------------------------- WS2812 ----------------------------------**/
#include <FastLED.h>
void ws2812_init(void);
void ws2812_set_color(CRGB c);
void ws2812_set_light(uint8_t light);
void ws2812_set_mode(int m);
int ws2812_get_mode(void);
void ws2812_effect_task(void);

/**------------------------------ PN532 ----------------------------------**/



/**------------------------------ LORA -----------------------------------**/
#include <RadioLib.h>
void lora_init(void);
void lora_task_start(void);


