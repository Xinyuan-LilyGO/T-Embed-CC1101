#include "TFT_eSPI.h" 

// About LCD definition in the file: lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h
TFT_eSPI tft;

void setup(void)
{
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
}

void loop(void)
{
    tft.fillScreen(TFT_RED);
    delay(1000);
    tft.fillScreen(TFT_GREEN);
    delay(1000);
    tft.fillScreen(TFT_BLUE);
    delay(1000);
    tft.fillScreen(TFT_LIGHTGREY);
    delay(1000);
    tft.fillScreen(TFT_BLACK);
    delay(1000);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setRotation(3);
    tft.drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    tft.drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    tft.drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    tft.drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    delay(1000);

    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    tft.drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    tft.drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    tft.drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    delay(1000);

    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    tft.drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    tft.drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    tft.drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    delay(1000);

    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(" !\"#$%&'()*+,-./0123456", 0, 0, 2);
    tft.drawString("789:;<=>?@ABCDEFGHIJKL", 0, 16, 2);
    tft.drawString("MNOPQRSTUVWXYZ[\\]^_`", 0, 32, 2);
    tft.drawString("abcdefghijklmnopqrstuvw", 0, 48, 2);
    delay(1000);
}