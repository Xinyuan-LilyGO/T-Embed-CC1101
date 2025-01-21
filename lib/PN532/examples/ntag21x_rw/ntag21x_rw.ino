
// Clean resets a tag back to factory-like state
// For Mifare Classic, tag is zero'd and reformatted as Mifare Classic
// For Mifare Ultralight, tags is zero'd and left empty
#include <PN532.h>
#include <NfcAdapter.h>

#if 0 // Using PN532's SPI (Seeed NFC shield)
#define NFC_INTERFACE_SPI
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532_SPI.cpp>

PN532_SPI intf(SPI, 10);
PN532 nfc = PN532(intf);
#else // Using PN532's I2C
#define NFC_INTERFACE_I2C
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532_I2C.cpp>
PN532_I2C intf(Wire);
PN532 nfc = PN532(intf);
#endif

// Using PN532's UART (Grove NFC)

// #include <PN532_I2C.h>
// #include <PN532.h>
// #include <NfcAdapter.h>
// PN532_HSU intf(Serial1);
// PN532 nfc = PN532(intf);

uint8_t password[4] = {0x12, 0x34, 0x56, 0x78};
uint8_t buf[4];
uint8_t uid[7];
uint8_t uidLength;

void setup(void)
{
    Serial.begin(9600);
    Serial.println("NTAG21x R/W");

    nfc.begin();
    nfc.SAMConfig();
}

void loop(void)
{
    Serial.println("wait for a tag");
    // wait until a tag is present
    while (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
    }

    // if NTAG21x enables r/w protection, uncomment the following line
    // nfc.ntag21x_auth(password);

    nfc.mifareultralight_ReadPage(3, buf);
    int capacity = buf[2] * 8;
    Serial.print(F("Tag capacity "));
    Serial.print(capacity);
    Serial.println(F(" bytes"));

    for (int i = 4; i < capacity / 4; i++)
    {
        nfc.mifareultralight_ReadPage(i, buf);
        nfc.PrintHexChar(buf, 4);
    }

    // wait until the tag is removed
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
    }
}
