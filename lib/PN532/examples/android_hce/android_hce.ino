#if 0
#define NFC_INTERFACE_SPI
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532_SPI.cpp>
#include "PN532.h"

  PN532_SPI pn532spi(SPI, 10);
  PN532 nfc(pn532spi);
#elif 1
#define NFC_INTERFACE_HSU
#include <PN532_HSU.h>
#include <PN532_HSU.cpp>
#include <PN532.h>

PN532_HSU pn532hsu(Serial1);
PN532 nfc(pn532hsu);
#else
#define NFC_INTERFACE_I2C
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532_I2C.cpp>
#include <PN532.h>
#endif

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif


void setup()
{
  Serial.begin(115200);
  Serial.println("-------Peer to Peer HCE--------");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  //nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

void loop()
{
  bool success;

  uint8_t responseLength = 32;

  Serial.println("Waiting for an ISO14443A card");

  // set shield to inListPassiveTarget
  success = nfc.inListPassiveTarget();

  if (success)
  {

    Serial.println("Found something!");

    uint8_t selectApdu[] = {0x00,                                     /* CLA */
                            0xA4,                                     /* INS */
                            0x04,                                     /* P1  */
                            0x00,                                     /* P2  */
                            0x07,                                     /* Length of AID  */
                            0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, /* AID defined on Android App */
                            0x00 /* Le  */};

    uint8_t response[32];

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    if (success)
    {

      Serial.print("responseLength: ");
      Serial.println(responseLength);

      nfc.PrintHexChar(response, responseLength);

      do
      {
        uint8_t apdu[] = "Hello from Arduino";
        uint8_t back[32];
        uint8_t length = 32;

        success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);

        if (success)
        {

          Serial.print("responseLength: ");
          Serial.println(length);

          nfc.PrintHexChar(back, length);
        }
        else
        {

          Serial.println("Broken connection?");
        }
      } while (success);
    }
    else
    {

      Serial.println("Failed sending SELECT AID");
    }
  }
  else
  {

    Serial.println("Didn't find anything!");
  }

  delay(1000);
}

void printResponse(uint8_t *response, uint8_t responseLength)
{

  String respBuffer;

  for (int i = 0; i < responseLength; i++)
  {

    if (response[i] < 0x10)
      respBuffer = respBuffer + "0"; //Adds leading zeros if hex value is smaller than 0x10

    respBuffer = respBuffer + String(response[i], HEX) + " ";
  }

  Serial.print("response: ");
  Serial.println(respBuffer);
}

void setupNFC()
{

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}
