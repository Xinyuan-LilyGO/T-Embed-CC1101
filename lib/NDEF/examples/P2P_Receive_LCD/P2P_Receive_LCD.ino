// Receive a NDEF message from a Peer and 
// display the payload of the first record on a LCD
//
// SeeedStudio NFC shield http://www.seeedstudio.com/depot/NFC-Shield-V20-p-1370.html
// LCD using the Adafruit backpack http://adafru.it/292
// Adafruit Liquid Crystal library https://github.com/adafruit/LiquidCrystal
// Use a Android of BlackBerry phone to send a message to the NFC shield

#include "SPI.h"
#include "PN532_SPI.h"
#include "snep.h"
#include "NdefMessage.h"

#include "Wire.h"
#include "LiquidCrystal.h"

PN532_SPI pn532spi(SPI, 10);
SNEP nfc(pn532spi);
uint8_t ndefBuf[128];

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal lcd(0);

void setup() {
    Serial.begin(9600);
    // set up the LCD's number of rows and columns: 
    lcd.begin(16, 2);
    Serial.println("NFC Peer to Peer Example - Receive Message");
}

void loop() {
    Serial.println("Waiting for message from a peer");
    int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf));
    if (msgSize > 0) {
        NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
        msg.print();
            
        NdefRecord record = msg.getRecord(0);

        int payloadLength = record.getPayloadLength();
        byte payload[payloadLength];
        record.getPayload(payload);        
        
        // The TNF and Type are used to determine how your application processes the payload
        // There's no generic processing for the payload, it's returned as a byte[]
        int startChar = 0;        
        if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "T") { // text message
          // skip the language code
          startChar = payload[0] + 1;
        } else if (record.getTnf() == TNF_WELL_KNOWN && record.getType() == "U") { // URI
          // skip the url prefix (future versions should decode)
          startChar = 1;
        }
                          
        // Force the data into a String (might fail for some content)
        // Real code should use smarter processing
        String payloadAsString = "";
        for (int c = startChar; c < payloadLength; c++) {
          payloadAsString += (char)payload[c];
        }
          
        // print on the LCD display
        lcd.setCursor(0, 0);
        lcd.print(payloadAsString);
        
        Serial.println("\nSuccess");
    } else {
        Serial.println("Failed");
    }
    delay(3000);
}
