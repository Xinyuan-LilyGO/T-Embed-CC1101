
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "utilities.h"


#define PN532_TSET
#define PN532_TSET_IQR  7
#define PN532_TSET_REST 8
#define PN532_TSET_SCL  10
#define PN532_TSET_SDA  11


#ifdef PN532_TSET
Adafruit_PN532 nfc(PN532_TSET_IQR, PN532_TSET_REST);
#else
Adafruit_PN532 nfc(BOARD_PN532_IRQ, BOARD_PN532_RF_REST);
#endif

void setup() {
    Serial.begin(115200);

    int start_delay = 3;
    while (start_delay) {
        Serial.print(start_delay);
        delay(1000);
        start_delay--;
    }

    // iic scan
    byte error, address;
    int nDevices = 0;
    Serial.println("Scanning for I2C devices ...");
#ifdef PN532_TSET
    Wire.begin(PN532_TSET_SDA, PN532_TSET_SCL);
#else
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
#endif
    for(address = 0x01; address < 0x7F; address++){
        Wire.beginTransmission(address);
        // 0: success.
        // 1: data too long to fit in transmit buffer.
        // 2: received NACK on transmit of address.
        // 3: received NACK on transmit of data.
        // 4: other error.
        // 5: timeout
        error = Wire.endTransmission();
        if(error == 0){ // 0: success.
            nDevices++;
            log_i("I2C device found at address 0x%x\n", address);
        } else if(error != 2){
            Serial.printf("Error %d at address 0x%02X\n", error, address);
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found");
    }

    Serial.println("Looking for PN532...");

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.println("Didn't find PN53x board");
        while (1); // halt
    } else {
        // Got ok data, print it out!
        Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
        Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
        Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);


        // Set the max number of retry attempts to read from a card
        // This prevents us from waiting forever for a card, which is
        // the default behaviour of the PN532.
        // nfc.setPassiveActivationRetries(0xFF);

        Serial.println("Waiting for an ISO14443A Card ...");
    }
}

void loop() {
   uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
    Serial.print("readPassiveTargetID:");
    Serial.println(success);
    if (success) {
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
            uint32_t cardid = uid[0];
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
}
