
#include <RadioLib.h>
#include "utilities.h"
#include "TFT_eSPI.h"

static float CC1101_freq = 868;

SPIClass radioSPI =  SPIClass(HSPI);

CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2, radioSPI);
// CC1101 radio = new Module(41, 40, -1, 21, radioSPI);

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

    int state = radio.begin(CC1101_freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
}

// counter to keep track of transmitted packets
int count = 0;

void loop()
{
    Serial.print(F("[CC1101] Transmitting packet ... "));

    // you can transmit C-string or Arduino string up to 63 characters long
    String str = "Hello World! #" + String(count++);
    int state = radio.transmit(str);
    Serial.println(str.c_str());


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
