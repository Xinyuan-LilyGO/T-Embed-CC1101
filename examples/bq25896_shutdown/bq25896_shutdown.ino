#include <Wire.h>
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include "utilities.h"

XPowersPPM PPM;

const uint8_t i2c_sda = BOARD_I2C_SDA;
const uint8_t i2c_scl = BOARD_I2C_SCL;
uint32_t cycleInterval;
uint32_t countdown = 5;


void setup()
{
    Serial.begin(115200);
    // while (!Serial);

    bool result =  PPM.init(Wire, i2c_sda, i2c_scl, BQ25896_SLAVE_ADDRESS);
    if (result == false) {
        while (1) {
            Serial.println("PPM is not online...");
            delay(1000);
        }
    }
}


void loop()
{
    if (millis() > cycleInterval) {
        Serial.printf("%d\n", countdown);
        if (!(countdown--)) {
            Serial.println("Shutdown .....");
            // The shutdown function can only be used when the battery is connected alone,
            // and cannot be shut down when connected to USB.
            // It can only be powered on in the following two ways:
            // 1. Press the PPM/QON button
            // 2. Connect to USB
            PPM.shutdown();
            countdown = 10000;
        }
        cycleInterval = millis() + 1000;
    }
    delay(1);
}





