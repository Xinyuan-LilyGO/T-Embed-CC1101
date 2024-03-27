
#include "peripheral.h"


CRGB leds[WS2812_NUM_LEDS];

void ws2812_init(void)
{
    FastLED.addLeds<WS2812, WS2812_DATA_PIN, GRB>(leds, WS2812_NUM_LEDS);
    FastLED.setBrightness(10);
    ws2812_set_color(CRGB::Red);
    FastLED.show();
}

void ws2812_set_color(CRGB c)
{
    for(int i = 0; i < WS2812_NUM_LEDS; i++){
        leds[i] = c;
    }
    FastLED.show();
}

void ws2812_set_light(uint8_t light)
{
    FastLED.setBrightness(light);
    FastLED.show();
}