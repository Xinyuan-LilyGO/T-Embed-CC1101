#include "TFT_eSPI.h" 
#include <RotaryEncoder.h>
#define ENCODER_INA 4
#define ENCODER_INB 5
#define ENCODER_KEY 0
#define BOARD_USER_KEY 6

// About LCD definition in the file: lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h
TFT_eSPI tft;
RotaryEncoder encoder(ENCODER_INA, ENCODER_INB, RotaryEncoder::LatchMode::TWO03);

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
    tft.setTextSize(2);
    tft.setCursor(50, 40); 
    tft.print("encode = 0     ");

    pinMode(ENCODER_KEY, INPUT);
    pinMode(BOARD_USER_KEY, INPUT);
}

void loop(void)
{
    static int pos = 0;
    encoder.tick();

    int newPos = encoder.getPosition();

    if (pos != newPos) {
        // tft.fillScreen(TFT_BLACK);
        String str = String(newPos);
        int txt_width = tft.textWidth(str.c_str());
        int scr_width = tft.width();
        // tft.setCursor((scr_width - txt_width) / 2, 60); 
        // tft.print(newPos);
        // tft.print(" ");

        tft.setCursor(50, 40); 
        tft.printf("encode = %d     ", newPos);
        pos = newPos;

        Serial.printf("Direction = %d\n", encoder.getDirection());
    }

    if (digitalRead(ENCODER_KEY) == LOW) 
    {
        tft.setCursor(50, 70); 
        tft.printf("encode btn press");
    } 
    else if (digitalRead(ENCODER_KEY) == HIGH) 
    { 
        tft.setCursor(50, 70); 
        tft.printf("encode btn release");
    }

    if (digitalRead(BOARD_USER_KEY) == LOW) 
    {
        tft.setCursor(50, 100); 
        tft.printf("user btn press");
    } 
    else if (digitalRead(BOARD_USER_KEY) == HIGH) 
    { 
        tft.setCursor(50, 100); 
        tft.printf("user btn release");
    }
    delay(1);
}