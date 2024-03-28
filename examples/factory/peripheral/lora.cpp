
#include "peripheral.h"

CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, RADIOLIB_NC, BOARD_LORA_IO2, SPI);

void lora_init(void)
{
    pinMode(BOARD_LORA_SW1, OUTPUT);
    pinMode(BOARD_LORA_SW0, OUTPUT);

    // SW1:1  SW0:0 --- 315MHz
    // digitalWrite(BOARD_LORA_SW1, HIGH);
    // digitalWrite(BOARD_LORA_SW0, LOW);
    // SW1:0  SW0:1 --- 868/915MHz
    digitalWrite(BOARD_LORA_SW1, LOW);
    digitalWrite(BOARD_LORA_SW0, HIGH);
    // SW1:1  SW0:1 --- 434MHz
    // digitalWrite(BOARD_LORA_SW1, HIGH);
    // digitalWrite(BOARD_LORA_SW0, HIGH);

    // initialize CC1101 with default settings
    Serial.print(F("[CC1101] Initializing ... "));
    // carrier frequency:                   868.0 MHz
    // bit rate:                            32.0 kbps
    // frequency deviation:                 60.0 kHz
    // Rx bandwidth:                        250.0 kHz
    // output power:                        7 dBm
    // preamble length:                     32 bits
    // int state = radio.begin(868.0, 32.0, 60.0, 250.0, 7, 32);
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
}

void lora_task(void *param)
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

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));

    } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);

    }
}

volatile bool lora_task_flag = false;
void lora_task_start(void)
{
    if(!lora_task_flag){
        lora_task_flag = true;
        xTaskCreatePinnedToCore(lora_task, "lora_task", 1024 * 2, NULL, 0, NULL, 0);
    }
}


