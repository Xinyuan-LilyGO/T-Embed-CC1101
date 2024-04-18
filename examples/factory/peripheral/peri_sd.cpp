#include "peripheral.h"

bool sd_init_flag = false;
uint32_t sd_sum_Mbyte = 0;
uint32_t sd_used_Mbyte = 0;

void sd_init(void)
{
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); 

    if(!SD.begin(BOARD_SD_CS)){
        Serial.println("Card Mount Failed");
        return;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    sd_sum_Mbyte = (SD.totalBytes() / (1024 * 1024));
    sd_used_Mbyte = (SD.usedBytes() / (1024 * 1024));

    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    Serial.printf("Total space: %dMB\n", sd_sum_Mbyte);
    Serial.printf("Used space: %dMB\n", sd_used_Mbyte);

    sd_init_flag = true;
}

bool sd_is_valid(void)
{
    return sd_init_flag;
}

uint32_t sd_get_sum_Mbyte(void)
{
    return sd_sum_Mbyte;
}

uint32_t sd_get_used_Mbyte(void)
{
    return sd_used_Mbyte;
}