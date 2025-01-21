
#include <RadioLib.h>
#include "utilities.h"

#include "TFT_eSPI.h"

#define ASSERT_FUN(name, fun)                                                       \
    if (fun == RADIOLIB_ERR_INVALID_BIT_RATE)                                       \
    {                                                                               \
        Serial.printf("[%s] Selected bit rate is invalid for this module\n", name); \
        while (true)                                                                \
            ;                                                                       \
    }

/**
 *  315MHz
 *  868/915MHz
 *  434MHz
 */
static float CC1101_freq = 315;

SPIClass radioSPI = SPIClass(HSPI);
CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);

int transmissionState = RADIOLIB_ERR_NONE;
volatile bool transmittedFlag = false;

void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
}

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
    digitalWrite(BOARD_PWR_EN, HIGH); // Power on CC1101 and WS2812

    // Set antenna frequency settings
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

    radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(CC1101_freq);
    Serial.println(" MHz ");

    int state = radio.begin(CC1101_freq);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }

    // you can also change the settings at runtime
    // and check if the configuration was changed successfully

    // set carrier frequency
    ASSERT_FUN("setFrequency", radio.setFrequency(CC1101_freq));

    // Enables/disables OOK modulation instead of FSK.
    ASSERT_FUN("setOOK", radio.setOOK(true));

    // Set bit rate to 5 kbps
    ASSERT_FUN("setBitRate", radio.setBitRate(1.2));

    // set receiver bandwidth to 135.0 kHz
    ASSERT_FUN("setRxBandwidth", radio.setRxBandwidth(58.0));

    // set allowed frequency deviation to 10.0 kHz
    ASSERT_FUN("setFrequencyDeviation", radio.setFrequencyDeviation(5.2));

    // set output power to 10 dBm
    ASSERT_FUN("setOutputPower", radio.setOutputPower(10));

    // 2 bytes can be set as sync word
    ASSERT_FUN("setSyncWord", radio.setSyncWord(0x01, 0x23));

    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(setFlag);

    // start transmitting the first packet
    Serial.print(F("[CC1101] Sending first packet ... "));

    // you can transmit C-string or Arduino string up to
    // 64 characters long
    transmissionState = radio.startTransmit("Hello World!");
}

// counter to keep track of transmitted packets
int count = 0;

void loop()
{
    // check if the previous transmission finished
    if (transmittedFlag)
    {
        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE)
        {
            // packet was successfully sent
            // Serial.println(F("transmission finished!"));

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // wait a second before transmitting again
        delay(500);

        // send another one
        Serial.print(F("[CC1101] Sending another packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        String str = "Hello World! #" + String(count++);
        transmissionState = radio.startTransmit(str);

        Serial.println(str.c_str());

        // you can also transmit byte array up to 256 bytes long
        /*
        byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                            0x89, 0xAB, 0xCD, 0xEF};
        int state = radio.startTransmit(byteArr, 8);
        */
    }
    delay(1);
}
