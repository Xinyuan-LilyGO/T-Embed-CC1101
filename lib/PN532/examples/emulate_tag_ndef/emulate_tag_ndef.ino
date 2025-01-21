#include "emulatetag.h"
#include "NdefMessage.h"

#ifdef NRF52840_XXAA
#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif
#endif

#if 0
#define NFC_INTERFACE_SPI
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532_SPI.cpp>
#include "PN532.h"

  PN532_SPI pn532spi(SPI, 10);
  EmulateTag nfc(pn532spi);
#elif 1
#define NFC_INTERFACE_HSU
#include <PN532_HSU.h>
#include <PN532_HSU.cpp>
#include <PN532.h>

PN532_HSU pn532hsu(Serial1);
EmulateTag nfc(pn532hsu);
#endif

uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = {0x12, 0x34, 0x56};

void setup()
{
  Serial.begin(115200);
  Serial.println("------- Emulate Tag --------");

  message = NdefMessage();
  message.addUriRecord("http://www.seeedstudio.com");
  messageSize = message.getEncodedSize();
  if (messageSize > sizeof(ndefBuf))
  {
    Serial.println("ndefBuf is too small");
    while (1)
    {
    }
  }

  Serial.print("Ndef encoded message size: ");
  Serial.println(messageSize);

  message.encode(ndefBuf);

  // comment out this command for no ndef message
  nfc.setNdefFile(ndefBuf, messageSize);

  // uid must be 3 bytes!
  nfc.setUid(uid);

  nfc.init();
}

void loop()
{
  // uncomment for overriding ndef in case a write to this tag occured
  //nfc.setNdefFile(ndefBuf, messageSize);

  // start emulation (blocks)
  nfc.emulate();

  // or start emulation with timeout
  /*if(!nfc.emulate(1000)){ // timeout 1 second
      Serial.println("timed out");
    }*/

  // deny writing to the tag
  // nfc.setTagWriteable(false);

  if (nfc.writeOccured())
  {
    Serial.println("\nWrite occured !");
    uint8_t *tag_buf;
    uint16_t length;

    nfc.getContent(&tag_buf, &length);
    NdefMessage msg = NdefMessage(tag_buf, length);
    // msg.print();
  }

  delay(1000);
}
