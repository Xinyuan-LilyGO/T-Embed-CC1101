
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

volatile bool receivedFlag = false;

void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

void setup()
{
    Serial.begin(115200);

#ifdef TEST_BOARD
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

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
    // Initialize SPI
    radioSPI.begin(BOARD_LORA_SCK, BOARD_LORA_MISO, BOARD_LORA_MOSI);
#endif

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(lora_freq);
    Serial.println(" MHz");

    // initialize CC1101 with default settings
    Serial.print(F("[CC1101] Initializing ... "));
    int state = radio.begin(lora_freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
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
}
