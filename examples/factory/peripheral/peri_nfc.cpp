
#include "peripheral.h"

Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);
uint32_t versiondata = 0;
bool nfc_upd_falg = false;

uint8_t success;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
uint32_t cardid = 0;
bool nfc_init_st = false;

void nfc_init(void)
{
    // Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    nfc.begin();

    versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.println("Didn't find PN53x board");
    } else {
        // Got ok data, print it out!
        Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
        Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
        Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

        // Set the max number of retry attempts to read from a card
        // This prevents us from waiting forever for a card, which is
        // the default behaviour of the PN532.

        Serial.println("Waiting for an ISO14443A Card ...");
        nfc_init_st = true;
    }
}

bool nfc_is_init(void)
{
    return nfc_init_st;
}

uint32_t nfc_get_ver_data(void)
{
    return versiondata;
}

void nfc_task(void *param)
{
    vTaskSuspend(nfc_handle);
    while (1)
    {
        
        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
        Serial.print("readPassiveTargetID:");
        Serial.println(success);
        if (success) {
            nfc_upd_falg = true;
            // Display some basic information about the card
            Serial.println("Found an ISO14443A card");
            Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
            Serial.print("  UID Value: ");
            nfc.PrintHex(uid, uidLength);

            String str = "NFC";
            str = "UID:";
            str.concat(uid[0]); str.concat(':');
            str.concat(uid[1]); str.concat(':');
            str.concat(uid[2]); str.concat(':');
            str.concat(uid[3]);

            if (uidLength == 4) {
                // We probably have a Mifare Classic card ...
                cardid = uid[0];
                cardid <<= 8;
                cardid |= uid[1];
                cardid <<= 8;
                cardid |= uid[2];
                cardid <<= 8;
                cardid |= uid[3];
                Serial.print("Seems to be a Mifare Classic card #");
                Serial.println(cardid);
            }
            Serial.println("");
        } else {
            // PN532 probably timed out waiting for a card
            Serial.println("Timed out waiting for a card");
        }

        delay(50);
    }
}