
#include <RadioLib.h>
#include "utilities.h"

#define TEST_BOARD
#define BOARD_TEST_CS   41
#define BOARD_TEST_SCK  42
#define BOARD_TEST_MOSI 46
#define BOARD_TEST_MISO 45
#define BOARD_TEST_IO2  21
#define BOARD_TEST_IO0  40
#define BOARD_TEST_SW1  17
#define BOARD_TEST_SW0  18


static float lora_freq = 0;
SPIClass radioSPI =  SPIClass(HSPI);


#ifdef TEST_BOARD
CC1101 radio = new Module(BOARD_TEST_CS, BOARD_TEST_IO0, -1, BOARD_TEST_IO2, radioSPI);
#else
 CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);
#endif

int transmissionState = RADIOLIB_ERR_NONE;
volatile bool transmittedFlag = false;

void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

void setup()
{
    Serial.begin(115200);

#ifdef TEST_BOARD
    pinMode(BOARD_TEST_CS, OUTPUT);
    digitalWrite(BOARD_TEST_CS, HIGH);

    //Set antenna frequency settings
    pinMode(BOARD_TEST_SW1, OUTPUT);
    pinMode(BOARD_TEST_SW0, OUTPUT);
    // SW1:1  SW0:0 --- 315MHz
    // SW1:0  SW0:1 --- 868/915MHz
    // SW1:1  SW0:1 --- 434MHz
    digitalWrite(BOARD_TEST_SW1, HIGH);
    digitalWrite(BOARD_TEST_SW0, LOW);
    lora_freq = 315.0;
    radioSPI.begin(BOARD_TEST_SCK, BOARD_TEST_MISO, BOARD_TEST_MOSI);
#else
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    //Set antenna frequency settings
    pinMode(BOARD_LORA_SW1, OUTPUT);
    pinMode(BOARD_LORA_SW0, OUTPUT);
    // SW1:1  SW0:0 --- 315MHz
    // SW1:0  SW0:1 --- 868/915MHz
    // SW1:1  SW0:1 --- 434MHz
    digitalWrite(BOARD_LORA_SW1, HIGH);
    digitalWrite(BOARD_LORA_SW0, LOW);
    lora_freq = 315.0;
    radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);
#endif

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(lora_freq);
    Serial.println(" MHz ");

    int state = radio.begin(lora_freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(setFlag);

    // start transmitting the first packet
    Serial.print(F("[CC1101] Sending first packet ... "));

    // you can transmit C-string or Arduino string up to
    // 64 characters long
    transmissionState = radio.startTransmit("Hello World!");

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
    // check if the previous transmission finished
    if(transmittedFlag) {
        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

        // NOTE: when using interrupt-driven transmit method,
        //       it is not possible to automatically measure
        //       transmission data rate using getDataRate()

        } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // wait a second before transmitting again
        delay(1000);

        // send another one
        Serial.print(F("[CC1101] Sending another packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        String str = "Hello World! #" + String(count++);
        transmissionState = radio.startTransmit(str);

        // you can also transmit byte array up to 256 bytes long
        /*
        byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                            0x89, 0xAB, 0xCD, 0xEF};
        int state = radio.startTransmit(byteArr, 8);
        */
    }
}
