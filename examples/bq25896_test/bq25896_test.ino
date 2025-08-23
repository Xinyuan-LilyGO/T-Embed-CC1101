#include <Wire.h>
#define XPOWERS_CHIP_BQ25896
#include <XPowersLib.h>
#include "utilities.h"

XPowersPPM PPM;

const uint8_t i2c_sda = BOARD_I2C_SDA;
const uint8_t i2c_scl = BOARD_I2C_SCL;
uint32_t cycleInterval;
bool pmu_irq = false;

void setup() 
{
    Serial.begin(115200);
    while (!Serial);


    bool result =  PPM.init(Wire, i2c_sda, i2c_scl, BQ25896_SLAVE_ADDRESS);

    if (result == false) {
        while (1) {
            Serial.println("PPM is not online...");
            delay(50);
        }
    }

    PPM.setChargeTargetVoltage(4208);
    PPM.enableMeasure();
    PPM.enableCharge();

    // Set the minimum operating voltage. Below this voltage, the PPM will protect
    // PPM.setSysPowerDownVoltage(3300);

    // Set input current limit, default is 500mA
    // PPM.setInputCurrentLimit(3250);

    // Serial.printf("getInputCurrentLimit: %d mA\n",PPM.getInputCurrentLimit());

    // Disable current limit pin
    // PPM.disableCurrentLimitPin();

    // Set the charging target voltage, Range:3840 ~ 4608mV ,step:16 mV
    // PPM.setChargeTargetVoltage(4208);

    // Set the precharge current , Range: 64mA ~ 1024mA ,step:64mA
    // PPM.setPrechargeCurr(64);

    // The premise is that Limit Pin is disabled, or it will only follow the maximum charging current set by Limi tPin.
    // Set the charging current , Range:0~5056mA ,step:64mA
    // PPM.setChargerConstantCurr(832);

    // Get the set charging current
    // PPM.getChargerConstantCurr();
    // Serial.printf("getChargerConstantCurr: %d mA\n",PPM.getChargerConstantCurr());


    // To obtain voltage data, the ADC must be enabled first
    // PPM.enableADCMeasure();
    
    // Turn on charging function
    // If there is no battery connected, do not turn on the charging function
    // PPM.enableCharge();

    // Turn off charging function
    // If USB is used as the only power input, it is best to turn off the charging function, 
    // otherwise the VSYS power supply will have a sawtooth wave, affecting the discharge output capability.
    // PPM.disableCharge();


    // The OTG function needs to enable OTG, and set the OTG control pin to HIGH
    // After OTG is enabled, if an external power supply is plugged in, OTG will be turned off

    // PPM.enableOTG();
    // PPM.disableOTG();
    // pinMode(OTG_ENABLE_PIN, OUTPUT);
    // digitalWrite(OTG_ENABLE_PIN, HIGH);


    delay(2000);
}

void loop() 
{
    // When VBUS is input, the battery voltage detection will not take effect
    if (millis() > cycleInterval) {

        Serial.println("Sats        VBUS    VBAT   SYS    VbusStatus      String   ChargeStatus     String      TargetVoltage       ChargeCurrent       Precharge       NTCStatus           String");
        Serial.println("            (mV)    (mV)   (mV)   (HEX)                         (HEX)                    (mV)                 (mA)                   (mA)           (HEX)           ");
        Serial.println("--------------------------------------------------------------------------------------------------------------------------------");
        Serial.print(PPM.isVbusIn() ? "Connected" : "Disconnect"); Serial.print("\t");
        Serial.print(PPM.getVbusVoltage()); Serial.print("\t");
        Serial.print(PPM.getBattVoltage()); Serial.print("\t");
        Serial.print(PPM.getSystemVoltage()); Serial.print("\t");
        Serial.print("0x");
        Serial.print(PPM.getBusStatus(), HEX); Serial.print("\t");
        Serial.print(PPM.getBusStatusString()); Serial.print("\t");
        Serial.print("0x");
        Serial.print(PPM.chargeStatus(), HEX); Serial.print("\t");
        Serial.print(PPM.getChargeStatusString()); Serial.print("\t");

        Serial.print(PPM.getChargeTargetVoltage()); Serial.print("\t");
        Serial.print(PPM.getChargeCurrent()); Serial.print("\t");
        Serial.print(PPM.getPrechargeCurr()); Serial.print("\t");
        Serial.print(PPM.getNTCStatus()); Serial.print("\t");
        Serial.print(PPM.getNTCStatusString()); Serial.print("\t");


        Serial.println();
        Serial.println();
        cycleInterval = millis() + 1000;
    }
}