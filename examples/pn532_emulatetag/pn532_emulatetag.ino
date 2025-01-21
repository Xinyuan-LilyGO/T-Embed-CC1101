/**************************************************************************/
/*!
    This example will attempt to connect to an ISO14443A
    card or tag and retrieve some basic information about it
    that can be used to determine what type of card it is.

    Note that you need the baud rate to be 115200 because we need to print
    out the data and read from the card at the same time!

    To enable debug message, define DEBUG in PN532/PN532_debug.h

*/
/**************************************************************************/


#define NFC_INTERFACE_I2C
#define BOARD_PN532_RF_REST 45
#define BOARD_PN532_IRQ     17
#define BOARD_I2C_SDA       8
#define BOARD_I2C_SCL       18

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532_I2C.cpp>
#include <PN532.h>

#include "NdefMessage.h"
#include "emulatetag.h"
uint8_t ndefBuf[120];
NdefMessage message;
int messageSize;

uint8_t uid[3] = {0x12, 0x34, 0x56};

PN532_I2C pn532i2c(Wire);
EmulateTag nfc(pn532i2c);


void setup(void)
{
    Serial.begin(115200);

    int start_delay = 3;
    while (start_delay)
    {
        Serial.print(start_delay);
        delay(1000);
        start_delay--;
    }
    Serial.println("Hello!");

    // pinMode(_irq, INPUT);
    pinMode(BOARD_PN532_RF_REST, OUTPUT);

    digitalWrite(BOARD_PN532_RF_REST, LOW);
    delay(1); // min 20ns
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    delay(2); // max 2ms

    // iic scan
    Wire.setPins(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.begin();

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

void loop(void)
{
    // uncomment for overriding ndef in case a write to this tag occured
    // nfc.setNdefFile(ndefBuf, messageSize);

    // start emulation (blocks)
    // nfc.emulate();

    // or start emulation with timeout
    if(!nfc.emulate(1000)){ // timeout 1 second
        Serial.println("timed out");
      }

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

    delay(20);
}
