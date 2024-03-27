#pragma once
#include "utilities.h"
/**----------------------------- WS2812 ----------------------------------**/
#include <FastLED.h>
void ws2812_init(void);
void ws2812_set_color(CRGB c);
void ws2812_set_light(uint8_t light);
