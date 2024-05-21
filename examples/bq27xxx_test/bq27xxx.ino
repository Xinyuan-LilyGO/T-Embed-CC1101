
#include "bq27220.h"
#include "TFT_eSPI.h" 
#include "Free_Fonts.h" 

#define BOARD_I2C_SDA  8
#define BOARD_I2C_SCL  18

#define I2C_DEV_ADDR 0x55

BQ27220 bq;
TFT_eSPI tft;

char buf[32];

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

void setup()
{
    Serial.begin(115200);

    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(FF18); 

    delay(3000);

    i2c_scan();
}

int line = 0;
#define LINEY ((line++)*16)

void showLine(char *buf)
{
    int len = strlen(buf);
    for(int i = len; i < 23; i++) {
        buf[i] = ' ';
    }
    buf[23] = '\0';
    tft.drawString(buf, 10, LINEY, 2);
}

union battery_state batt;



void loop() 
{
    line = 0;

    

    batt.full = bq.getBatterySt();
    snprintf(buf, 32, "Status = %x", batt.full);
    showLine(buf);

    printf("%d %d %d %d %d %d %d %d\n", batt.st.DSG,batt.st.SYSDWN, batt.st.TDA, batt.st.BATTPRES,
    batt.st.AUTH_GD, batt.st.OCVGD, batt.st.TCA, batt.st.RSVD);

    printf("%d\n", batt.st.FD);

    snprintf(buf, 32, "Status = %s", bq.getIsCharging() ? "Charging" : "Discharging");
    showLine(buf);

    snprintf(buf, 32, "Temp = %.2f K", (float)(bq.getTemp() / 10));
    showLine(buf);

    snprintf(buf, 32, "battery = %d mAh", bq.getRemainCap());
    showLine(buf);

    snprintf(buf, 32, "battery full= %d mAh", bq.getRemainCap());
    showLine(buf);

    tft.drawString("------- Voltage -------", 10, LINEY, 2);

    snprintf(buf, 32, "Volt = %d mV", bq.getVolt(VOLT));
    showLine(buf);

    snprintf(buf, 32, "Volt Charg= %d mV", bq.getVolt(VOLT_CHARGING));
    showLine(buf);

    tft.drawString("------- Current -------", 10, LINEY, 2);

    snprintf(buf, 32, "Curr Average=%d mA", bq.getCurr(CURR_AVERAGE));
    showLine(buf);

    snprintf(buf, 32, "Curr Instant=%d mA", bq.getCurr(CURR_INSTANT));
    showLine(buf);

    snprintf(buf, 32, "Curr Standby=%d mA", bq.getCurr(CURR_STANDBY));
    showLine(buf);

    snprintf(buf, 32, "Curr Charging=%d mA", bq.getCurr(CURR_CHARGING));
    showLine(buf);

    delay(1000);
}


