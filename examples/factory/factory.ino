
#include "utilities.h"
#include <XPowersLib.h>
#include "ui.h"

PowersSY6970 PMU;
uint32_t cycleInterval;

void setup(void)
{
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
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
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

    // SPI init
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, -1);

    bool result =  PMU.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_ADDR_2);

    if (result == false) {
        while (1) {
            Serial.println("PMU is not online...");
            delay(50);
        }
    }
    // Set the charging target voltage, Range:3840 ~ 4608mV ,step:16 mV
    PMU.setChargeTargetVoltage(3856);

    // Set the precharge current , Range: 64mA ~ 1024mA ,step:64mA
    PMU.setPrechargeCurr(64);

    // Set the charging current , Range:0~5056mA ,step:64mA
    PMU.setChargerConstantCurr(320);

    // To obtain voltage data, the ADC must be enabled first
    PMU.enableADCMeasure();
    
    lora_init();
    
    ws2812_init();
    // ui display
    ui_entry();
}

void loop(void)
{
    lv_timer_handler();

    // SY6970 When VBUS is input, the battery voltage detection will not take effect
    if (millis() > cycleInterval) {

        Serial.println("Sats        VBUS    VBAT   SYS    VbusStatus      String   ChargeStatus     String      TargetVoltage       ChargeCurrent       Precharge       NTCStatus           String");
        Serial.println("            (mV)    (mV)   (mV)   (HEX)                         (HEX)                    (mV)                 (mA)                   (mA)           (HEX)           ");
        Serial.println("--------------------------------------------------------------------------------------------------------------------------------");
        Serial.print(PMU.isVbusIn() ? "Connected" : "Disconnect"); Serial.print("\t");
        Serial.print(PMU.getVbusVoltage()); Serial.print("\t");
        Serial.print(PMU.getBattVoltage()); Serial.print("\t");
        Serial.print(PMU.getSystemVoltage()); Serial.print("\t");
        Serial.print("0x");
        Serial.print(PMU.getBusStatus(), HEX); Serial.print("\t");
        Serial.print(PMU.getBusStatusString()); Serial.print("\t");
        Serial.print("0x");
        Serial.print(PMU.chargeStatus(), HEX); Serial.print("\t");
        Serial.print(PMU.getChargeStatusString()); Serial.print("\t");

        Serial.print(PMU.getChargeTargetVoltage()); Serial.print("\t");
        Serial.print(PMU.getChargerConstantCurr()); Serial.print("\t");
        Serial.print(PMU.getPrechargeCurr()); Serial.print("\t");
        Serial.print(PMU.getNTCStatus()); Serial.print("\t");
        Serial.print(PMU.getNTCStatusString()); Serial.print("\t");


        Serial.println();
        Serial.println();
        cycleInterval = millis() + 1000;
    }
}
