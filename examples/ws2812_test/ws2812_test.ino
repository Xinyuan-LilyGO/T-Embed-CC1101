

#include <FastLED.h>

#define BOARD_PWR_EN   15

#define NUM_LEDS 8
#define DATA_PIN 14

CRGB leds[NUM_LEDS];

void setup() { 
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);  // Power on CC1101 and WS2812

    FastLED.addLeds<WS2813, DATA_PIN, GBR>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    FastLED.show();
}

void loop() { 
    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Red;
    }
    FastLED.show();
    delay(1000);

    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
    delay(1000);

    for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Green;
    }
    FastLED.show();
    delay(1000);
}


