
#include "peripheral.h"

extern int lv_port_indev_get_pos(void);

int ws2812_effs_mode = 0;
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
    if(ws2812_effs_mode == 0)
        FastLED.show();
}

void ws2812_set_light(uint8_t light)
{
    FastLED.setBrightness(light);

    if(ws2812_effs_mode == 0)
        FastLED.show();
}

void ws2812_set_mode(int m)
{
    m &= 0x3;
    ws2812_effs_mode = m;
}

int ws2812_get_mode(void)
{
    return ws2812_effs_mode;
}

CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch ((h / 60) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    }

    CRGB c;
    c.red = r;
    c.green = g;
    c.blue = b;
    return c;
}

void ws2812_task(void *param)
{
    uint16_t temp, mode = 0;
    int8_t last_led = 0;
    EventBits_t bit;
    int encoder_pos = 0;

    vTaskSuspend(ws2812_handle);
    
    while(1){
        uint8_t time = millis() >> 4;
        switch (ws2812_effs_mode) {
            case 0:
                // for (uint16_t i = 0; i < WS2812_NUM_LEDS; i++) {
                //     leds[i] = CRGB::Black;
                // }
                FastLED.show();
                break;
            case 1:
                for (uint16_t i = 0; i < WS2812_NUM_LEDS; i++) {
                    leds[i] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
                }
                FastLED.show();
                break;
            case 2:
                for (uint16_t i = 0; i < WS2812_NUM_LEDS; i++) {
                    uint8_t p = time - i * 8;
                    leds[i] = hsvToRgb((uint32_t)p * 359 / 256, 255, 255);
                }
                FastLED.show();
                break;
            case 3:{
                int curr_pos = lv_port_indev_get_pos();
                int led_pos;
                ws2812_set_color(CRGB::Black);
                led_pos = abs(curr_pos) % WS2812_NUM_LEDS;
                leds[led_pos] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
                FastLED.show();
            }
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}