#include "TFT_eSPI.h" 
#include <RotaryEncoder.h>
#define ENCODER_INA 4
#define ENCODER_INB 5
#define ENCODER_KEY 0

// About LCD definition in the file: lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h
TFT_eSPI tft;
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

int display_rotation = 3;

void encoder_handler(void)
{
    if(display_rotation == 3){
        tft.setRotation(1);
        display_rotation = 1;
    } else if(display_rotation == 1){
        tft.setRotation(3);
        display_rotation = 3;
    }
    Serial.printf("display_rotation = %d\n", display_rotation);
}

void setup(void)
{
    Serial.begin(115200);
    int start_delay = 2;
    while (start_delay) {
        Serial.print(start_delay);
        delay(1000);
        start_delay--;
    }

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(7);
    tft.setCursor(150, 60); 
    tft.print("0");

    pinMode(ENCODER_KEY, INPUT);
    attachInterrupt(ENCODER_KEY, encoder_handler, FALLING);
}

void loop(void)
{
    static int pos = 0;
    encoder.tick();

    int newPos = encoder.getPosition();
    if (pos != newPos) {
        tft.fillScreen(TFT_BLACK);
        String str = String(newPos);
        int txt_width = tft.textWidth(str.c_str());
        int scr_width = tft.width();
        tft.setCursor((scr_width - txt_width) / 2, 60); 
        tft.print(newPos);
        tft.print(" ");
        pos = newPos;

        Serial.printf("Direction = %d\n", encoder.getDirection());
    }
}