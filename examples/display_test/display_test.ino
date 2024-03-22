#include "TFT_eSPI.h" 

TFT_eSPI tft = TFT_eSPI(170, 320);

void setup(void)
{
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
}

void loop(void)
{

}