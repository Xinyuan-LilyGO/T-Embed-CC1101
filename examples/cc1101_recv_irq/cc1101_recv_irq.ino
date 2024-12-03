
#include <RadioLib.h>
#include "utilities.h"

#include "TFT_eSPI.h" 

/**
 *  315MHz
 *  868/915MHz
 *  434MHz
 */
static float CC1101_freq = 315;

SPIClass radioSPI =  SPIClass(HSPI);

CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);

volatile bool receivedFlag = false;

void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
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
    digitalWrite(BOARD_PWR_EN, HIGH);  // Power on CC1101 and WS2812

    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

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
    Serial.println(" MHz");

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
    if (radio.setFrequency(CC1101_freq) == RADIOLIB_ERR_INVALID_FREQUENCY)
    {
        Serial.println(F("[CC1101] Selected frequency is invalid for this module!"));
        while (true)
            ;
    }

    radio.setOOK(true);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE)
    {
        Serial.println(F("[CC1101] set OOK is invalid for this module!"));
        while (true)
            ;
    }

    // Too high an air rate may result in data loss
    // Set bit rate to 5 kbps
    state = radio.setBitRate(1.2);
    if (state == RADIOLIB_ERR_INVALID_BIT_RATE)
    {
        Serial.println(F("[CC1101] Selected bit rate is invalid for this module!"));
        while (true)
            ;
    }

    // set receiver bandwidth to 135.0 kHz
    if (radio.setRxBandwidth(58.0) == RADIOLIB_ERR_INVALID_RX_BANDWIDTH)
    {
        Serial.println(F("[CC1101] Selected receiver bandwidth is invalid for this module!"));
        while (true)
            ;
    }

    // set allowed frequency deviation to 10.0 kHz
    if (radio.setFrequencyDeviation(5.2) == RADIOLIB_ERR_INVALID_FREQUENCY_DEVIATION)
    {
        Serial.println(F("[CC1101] Selected frequency deviation is invalid for this module!"));
        while (true)
            ;
    }

    // set output power to 10 dBm
    if (radio.setOutputPower(10) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
    {
        Serial.println(F("[CC1101] Selected output power is invalid for this module!"));
        while (true)
            ;
    }

    // 2 bytes can be set as sync word
    if (radio.setSyncWord(0x01, 0x23) == RADIOLIB_ERR_INVALID_SYNC_WORD)
    {
        Serial.println(F("[CC1101] Selected sync word is invalid for this module!"));
        while (true)
            ;
    }

    // set the function that will be called
    // when new packet is received
    radio.setPacketReceivedAction(setFlag);

    // start listening for packets
    Serial.print(F("[CC1101] Starting to listen ... "));
    state = radio.startReceive();
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
    // check if the flag is set
    if(receivedFlag) {
        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);

        // you can also read received data as byte array
        /*
        byte byteArr[8];
        int numBytes = radio.getPacketLength();
        int state = radio.readData(byteArr, numBytes);
        */

        if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("[CC1101] Received packet!"));

        // print data of the packet
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

        // put module back to listen mode
        radio.startReceive();
    }
    delay(1);
}
