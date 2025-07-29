
/*
  RadioLib nRF24 Transmit with Interrupts Example

  This example transmits packets using nRF24 2.4 GHz radio module.
  Each packet contains up to 32 bytes of data, in the form of:
  - Arduino String
  - null-terminated char array (C-string)
  - arbitrary binary data (byte array)

  Packet delivery is automatically acknowledged by the receiver.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#nrf24

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>
#include "utilities.h"
#include <FastLED.h>

#define BOARD_PWR_EN 15

#define NUM_LEDS 8
#define DATA_PIN 14

#define NRF24L01_CS 44  // SPI Chip Select
#define NRF24L01_CE 43  // Chip Enable Activates RX or TX(High) mode
#define NRF24L01_IQR -1 // Maskable interrupt pin. Active low
#define NRF24L01_MOSI 9
#define NRF24L01_MISO 10
#define NRF24L01_SCK 11

// nRF24 has the following connections:
// CS pin:    10
// IRQ pin:   2
// CE pin:    3

nRF24 radio = new Module(NRF24L01_CS, NRF24L01_IQR, NRF24L01_CE);

CRGB leds[NUM_LEDS];

std::string info_head = "Hello World! #";

// or detect the pinout automatically using RadioBoards
// https://github.com/radiolib-org/RadioBoards
/*
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>
Radio radio = new RadioModule();
*/

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    // we sent a packet, set the flag
    receivedFlag = true;
}

void setup()
{
    Serial.begin(115200);

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH); // Power on CC1101 and WS2812

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    pinMode(TFT_CS, OUTPUT);
    digitalWrite(TFT_CS, HIGH);
    pinMode(BOARD_SD_CS, OUTPUT);
    digitalWrite(BOARD_SD_CS, HIGH);
    pinMode(BOARD_LORA_CS, OUTPUT);
    digitalWrite(BOARD_LORA_CS, HIGH);

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH); // Power on CC1101 and WS2812

    FastLED.addLeds<WS2813, DATA_PIN, GBR>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    FastLED.show();

    // pinMode(BOARD_NRF24_CS, OUTPUT);
    // digitalWrite(BOARD_NRF24_CS, HIGH);

    // SPI.end();
    SPI.begin(NRF24L01_SCK, NRF24L01_MISO, NRF24L01_MOSI);

    // initialize nRF24 with default settings
    Serial.print(F("[nRF24] Initializing ... "));
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
        {
            // delay(10);
        }
    }

    radio.setOutputPower(0);

    // while(1){
    //     radio.transmitDirect();
    // }

    // set receive pipe 0 address
    // NOTE: address width in bytes MUST be equal to the
    //       width set in begin() or setAddressWidth()
    //       methods (5 by default)
    Serial.print(F("[nRF24] Setting address for receive pipe 0 ... "));
    byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
    state = radio.setReceivePipe(0, addr);
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

    // set the function that will be called
    // when new packet is received
    radio.setPacketReceivedAction(setFlag);

    // start listening
    Serial.print(F("[nRF24] Starting to listen ... "));
    state = radio.startReceive();
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
}

// counter to keep track of transmitted packets
int count = 0;

uint32_t curr_tick = 0;

CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch ((h / 60) % 6)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }

    CRGB c;
    c.red = r;
    c.green = g;
    c.blue = b;
    return c;
}

void ws2812_set_color(CRGB c)
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = c;
    }
    FastLED.show();
}

void ws2812_pos_demo(int pos)
{
    uint8_t time = millis() >> 4;
    int curr_pos = pos;
    int led_pos;
    ws2812_set_color(CRGB::Black);
    led_pos = abs(curr_pos) % NUM_LEDS;
    leds[led_pos] = hsvToRgb((uint32_t)time * 359 / 256, 255, 255);
    FastLED.show();
}

void ws2812_demo1(void)
{
    uint8_t time = millis() >> 4;
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        uint8_t p = time - i * 8;
        leds[i] = hsvToRgb((uint32_t)p * 359 / 256, 255, 255);
    }
    FastLED.show();
}

bool containsSubstring(const std::string &mainStr, const std::string &subStr)
{
    // 如果子串为空，通常认为包含关系成立
    if (subStr.empty())
        return true;

    // 在 mainStr 中查找 subStr
    return mainStr.find(subStr) != std::string::npos;
}

void loop()
{
    // check if the flag is set
    if (millis() - curr_tick > 100)
    {
        curr_tick = millis();
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

        if (state == RADIOLIB_ERR_NONE)
        {
            if (containsSubstring(str.c_str(), info_head))
            {
                // packet was successfully received
                Serial.println(F("[nRF24] Received packet!"));
                // print data of the packet
                Serial.print(F("[nRF24] Data:\t\t"));
                Serial.println(str);

                ws2812_demo1();
            }
        }
        else
        {
            // some other error occurred
            Serial.print(F("[nRF24] Failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();
    }

    delay(1);
}

#if 0

/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */

/**
 * A simple example of sending data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#include "utilities.h"

#define ASSERT_FUN(name, fun)                                                       \
    if (fun == 0)                                                                   \
    {                                                                               \
        Serial.printf("[%s] Selected bit rate is invalid for this module\n", name); \
        while (1)                                                                   \
            ;                                                                       \
    }

RF24 radio(BOARD_NRF24_CE, BOARD_NRF24_CS, 1000000);
const byte address[6] = "00001";
byte send_data = 0;
// NRF24L01 buffer limit is 32 bytes (max struct size)
struct payload
{
    byte data1;
    char data2;
};
payload payload;

void setup()
{
    // LORA、SD and LCD use the same spi, in order to avoid mutual influence;
    // before powering on, all CS signals should be pulled high and in an unselected state;
    pinMode(TFT_CS, OUTPUT);
    pinMode(BOARD_SD_CS, OUTPUT);
    pinMode(BOARD_LORA_CS, OUTPUT);
    pinMode(BOARD_NRF24_CS, OUTPUT);

    digitalWrite(TFT_CS, HIGH);
    digitalWrite(BOARD_SD_CS, HIGH);
    digitalWrite(BOARD_LORA_CS, HIGH);
    digitalWrite(BOARD_NRF24_CS, HIGH);

    // pinMode(TFT_BL, OUTPUT);
    // digitalWrite(TFT_BL, LOW); // Power on CC1101 and WS2812

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH); // Power on CC1101 and WS2812

    SPI.end();
    SPI.begin(BOARD_NRF24_SCK, BOARD_NRF24_MISO, BOARD_NRF24_MOSI);

    Serial.begin(115200);
    while (!Serial)
    {
        // some boards need to wait to ensure access to serial over USB
    }

    ASSERT_FUN("begin", radio.begin(&SPI, BOARD_NRF24_CE, BOARD_NRF24_CS));

    // 50-1000-50000   -> 100-1000-100000

    // set channel
    radio.setChannel(0);
    // Append ACK packet from the receiving radio back to the transmitting radio
    radio.setAutoAck(false); //(true|false)
    // Set the transmission datarate
    radio.setDataRate(RF24_2MBPS); //(RF24_250KBPS|RF24_1MBPS|RF24_2MBPS)
    // Greater level = more consumption = longer distance
    radio.setPALevel(RF24_PA_MAX); //(RF24_PA_MIN|RF24_PA_LOW|RF24_PA_HIGH|RF24_PA_MAX)
    // Default value is the maximum 32 bytes
    radio.setPayloadSize(sizeof(payload));
    // Act as transmitter
    radio.openWritingPipe(address);
    radio.stopListening();
}

void loop()
{
    payload.data1 = send_data++;
    payload.data2 = 'x';
    radio.write(&payload, sizeof(payload));
    Serial.print("Data1:");
    Serial.println(payload.data1);
    Serial.print("Data2:");
    Serial.println(payload.data2);
    Serial.println("Sent");
    delay(300);
}
#endif