
#include <RadioLib.h>
#include "utilities.h"

static float lora_freq = 0;
SPIClass radioSPI =  SPIClass(HSPI);
CC1101 radio = new Module(41, 40, -1, 21, radioSPI);

void setup()
{
    Serial.begin(115200);

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    //Set antenna frequency settings
    // pinMode(BOARD_LORA_SW1, OUTPUT);
    // pinMode(BOARD_LORA_SW0, OUTPUT);
    // SW1:1  SW0:0 --- 315MHz
    // SW1:0  SW0:1 --- 868/915MHz
    // SW1:1  SW0:1 --- 434MHz
    // digitalWrite(BOARD_LORA_SW1, LOW);
    // digitalWrite(BOARD_LORA_SW0, HIGH);
    lora_freq = 868.0;
    
    // Initialize SPI
    radioSPI.begin(42, 45, 46);

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(lora_freq);
    Serial.print(" MHz ");

    // initialize CC1101 with default settings
    Serial.print(F("[CC1101] Initializing ... "));
    int state = radio.begin();
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
    delay(1);
}
