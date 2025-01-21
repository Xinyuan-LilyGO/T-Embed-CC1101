// Clean resets a tag back to factory-like state
// For Mifare Classic, tag is zero'd and reformatted as Mifare Classic
// For Mifare Ultralight, tags is zero'd and left empty

#if 0
#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);
#else

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
#endif

void setup(void) {
    Serial.begin(9600);
    Serial.println("NFC Tag Cleaner");
    nfc.begin();
}

void loop(void) {

    Serial.println("\nPlace a tag on the NFC reader to clean.");

    if (nfc.tagPresent()) {

        bool success = nfc.clean();
        if (success) {
            Serial.println("\nSuccess, tag restored to factory state.");
        } else {
            Serial.println("\nError, unable to clean tag.");
        }

    }
    delay(5000);
}
