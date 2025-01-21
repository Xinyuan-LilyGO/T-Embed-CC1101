// NTAG21x supports 4 bytes password to protect pages started from AUTH0
// AUTH0 defines the page address from which the password verification is required.
// Valid address range for byte AUTH0 is from 00h to FFh.
// If AUTH0 is set to a page address which is higher than the last page from the user configuration,
// the password protection is effectively disabled
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

    uint8_t cfg_page_base = 0x29; // NTAG213
    if (capacity == 0x3E)
    {
        cfg_page_base = 0x83; // NTAG215
    }
    else if (capacity == 0x6D)
    {
        cfg_page_base = 0xE3; // NTAG216
    }

    // PWD page, set new password
    nfc.mifareultralight_WritePage(cfg_page_base + 2, password);

    // disable r/w
    // | PROT | CFG_LCK | RFUI | NFC_CNT_EN | NFC_CNT_PWD_PROT | AUTHLIM (2:0) |
    buf[0] = (1 << 7) | 0x0;
    nfc.mifareultralight_WritePage(cfg_page_base + 1, buf);

    // protect pages started from AUTH0
    uint8_t auth0 = 0x10;
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = auth0;
    nfc.mifareultralight_WritePage(cfg_page_base, buf);

    // wait until the tag is removed
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    {
    }
}
