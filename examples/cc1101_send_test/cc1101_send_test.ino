
#include <RadioLib.h>
#include "utilities.h"

static float lora_freq = 0;

SPIClass radioSPI =  SPIClass(HSPI);

CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);
// CC1101 radio = new Module(41, 40, -1, 21, radioSPI);

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
    radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);
    // radioSPI.begin(42, 45, 46);

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(lora_freq);
    Serial.print(" MHz ");

    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // you can also change the settings at runtime
    // and check if the configuration was changed successfully

    // set carrier frequency
    // if (radio.setFrequency(lora_freq) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    //     Serial.println(F("[CC1101] Selected frequency is invalid for this module!"));
    //     while (true);
    // }

    // // Too high an air rate may result in data loss
    // // Set bit rate to 5 kbps
    // state = radio.setBitRate(5);
    // if (state == RADIOLIB_ERR_INVALID_BIT_RATE) {
    //     Serial.println(F("[CC1101] Selected bit rate is invalid for this module!"));
    //     while (true);
    // } else if (state == RADIOLIB_ERR_INVALID_BIT_RATE_BW_RATIO) {
    //     Serial.println(F("[CC1101] Selected bit rate to bandwidth ratio is invalid!"));
    //     Serial.println(F("[CC1101] Increase receiver bandwidth to set this bit rate."));
    //     while (true);
    // }

    // // set receiver bandwidth to 135.0 kHz
    // if (radio.setRxBandwidth(135.0) == RADIOLIB_ERR_INVALID_RX_BANDWIDTH) {
    //     Serial.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
    //     while (true);
    // }

    // // set allowed frequency deviation to 10.0 kHz
    // if (radio.setFrequencyDeviation(10.0) == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION) {
    //     Serial.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
    //     while (true);
    // }

    // // set output power to 10 dBm
    // if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    //     Serial.println(F("[CC1101] Selected output power is invalid for this module!"));
    //     while (true);
    // }

    // // 2 bytes can be set as sync word
    // if (radio.setSyncWord(0x01, 0x12) == RADIOLIB_ERR_INVALID_SYNC_WORD) {
    //     Serial.println(F("[CC1101] Selected sync word is invalid for this module!"));
    //     while (true);
    // }
}

// counter to keep track of transmitted packets
int count = 0;

void loop()
{
    Serial.print(F("[CC1101] Transmitting packet ... "));

    // you can transmit C-string or Arduino string up to 63 characters long
    String str = "Hello World! #" + String(count++);
    int state = radio.transmit(str);

    // you can also transmit byte array up to 63 bytes long
    /*
        byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
        int state = radio.transmit(byteArr, 8);
    */

    if (state == RADIOLIB_ERR_NONE) {
        // the packet was successfully transmitted
        Serial.println(F("success!"));

    } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
        // the supplied packet was longer than 64 bytes
        Serial.println(F("too long!"));

    } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);

    }

    // wait for a second before transmitting again
    delay(1000);
}
