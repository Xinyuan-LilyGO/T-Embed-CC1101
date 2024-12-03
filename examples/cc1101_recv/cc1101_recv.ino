
#include <RadioLib.h>
#include "utilities.h"
#include "TFT_eSPI.h"

static float CC1101_freq = 868;
SPIClass radioSPI =  SPIClass(HSPI);
CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);

void setup()
{
    // LORA、SD and LCD use the same spi, in order to avoid mutual influence; 
    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    // Init system
    Serial.begin(115200);

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);  // Power on CC1101 and WS2812

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    //Set antenna frequency settings
    pinMode(BOARD_LORA_SW1, OUTPUT);
    pinMode(BOARD_LORA_SW0, OUTPUT);
    
    // SW1:1  SW0:0 --- 315MHz
    // SW1:0  SW0:1 --- 868/915MHz
    // SW1:1  SW0:1 --- 434MHz
    if (CC1101_freq - 315 < 0.1)
    {
        digitalWrite(BOARD_LORA_SW1, HIGH);
        digitalWrite(BOARD_LORA_SW0, LOW);
    }
    else if (CC1101_freq - 434 < 0.1)
    {
        digitalWrite(BOARD_LORA_SW1, HIGH);
        digitalWrite(BOARD_LORA_SW0, HIGH);
    }
    else if (CC1101_freq - 868 < 0.1)
    {
        digitalWrite(BOARD_LORA_SW1, LOW);
        digitalWrite(BOARD_LORA_SW0, HIGH);
    }
    else if (CC1101_freq - 915 < 0.1)
    {
        digitalWrite(BOARD_LORA_SW1, LOW);
        digitalWrite(BOARD_LORA_SW0, HIGH);
    }
    
    // Initialize SPI
    radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(CC1101_freq);
    Serial.print(" MHz ");

    // initialize CC1101 with default settings
    Serial.print(F("[CC1101] Initializing ... "));
    int state = radio.begin(CC1101_freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
}


void loop()
{
    Serial.print(F("[CC1101] Waiting for incoming transmission ... "));

    // you can receive data as an Arduino String
    String str;
    int state = radio.receive(str);

    // you can also receive data as byte array
    /*
        byte byteArr[8];
        int state = radio.receive(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("success!"));

        // print the data of the packet
        Serial.print(F("[CC1101] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        // of the last received packet
        Serial.print(F("[CC1101] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print LQI (Link Quality Indicator)
        // of the last received packet, lower is better
        Serial.print(F("[CC1101] LQI:\t\t"));
        Serial.println(radio.getLQI());

    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        // timeout occurred while waiting for a packet
        Serial.println(F("timeout!"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));

    } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);

    }
    delay(1000);
}
