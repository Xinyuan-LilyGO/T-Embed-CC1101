
#include "bq27220.h"
#include "TFT_eSPI.h"
#include "Wire.h"

#define BOARD_I2C_SDA  8
#define BOARD_I2C_SCL  18

#define I2C_DEV_ADDR 0x55

#define LINEY(x) ((x++)*16)

char buf[32];
int line = 0;

BQ27220 bq;
TFT_eSPI tft;

BQ27220BatteryStatus batt;

void i2c_scan(void);
void showLine(char *buf);

void setup()
{
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&FreeSans12pt7b); 

    delay(3000);

    i2c_scan();

    Serial.printf("device number:0x%x\n", bq.getDeviceNumber());

    bq.init();
}

void loop() 
{
    line = 0;

    bq.getBatteryStatus(&batt);
    snprintf(buf, 32, "Status = %x", batt.full);
    showLine(buf);

    snprintf(buf, 32, "Status = %s", bq.getIsCharging() ? "Charging" : "Discharging");
    showLine(buf);

    snprintf(buf, 32, "Volt = %d mV", bq.getVoltage());
    showLine(buf);

    snprintf(buf, 32, "Curr = %d mA", bq.getCurrent());
    showLine(buf);

    snprintf(buf, 32, "Temp = %.2f K", (float)(bq.getTemperature() / 10.0));
    showLine(buf);

    snprintf(buf, 32, "full cap= %d mAh", bq.getFullChargeCapacity());
    showLine(buf);

    snprintf(buf, 32, "design cap = %d mAh", bq.getDesignCapacity());
    showLine(buf);

    snprintf(buf, 32, "remain cap = %d mAh", bq.getRemainingCapacity());
    showLine(buf);

    snprintf(buf, 32, "state of charge = %d", bq.getStateOfCharge());
    showLine(buf);

    snprintf(buf, 32, "state of health = %d", bq.getStateOfHealth());
    showLine(buf);

    

    delay(1000);
}

void showLine(char *buf)
{
    int len = strlen(buf);
    for(int i = len; i < 23; i++) {
        buf[i] = ' ';
    }
    buf[23] = '\0';
    tft.drawString(buf, 10, LINEY(line), 2);
}

void i2c_scan(void)
{
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
            Serial.printf("I2C devices found [0x%x]\n", address);
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found");
    }
}



