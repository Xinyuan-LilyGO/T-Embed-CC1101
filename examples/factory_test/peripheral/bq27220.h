

#ifndef BQ27227_H
#define BQ27227_H

#include "Arduino.h"
#include <Wire.h>
#include "bq27220.h"

#define DEFAULT_SCL  18
#define DEFAULT_SDA  8


// device addr
#define BQ27220_I2C_ADDRESS 0x55

// device id
#define BQ27220_DEVICE_ID 0x0220

// commands
#define BQ27220_COMMAND_CONTROL         0X00 // Control()
#define BQ27220_COMMAND_TEMP            0X06 // Temperature()
#define BQ27220_COMMAND_BATTERY_ST      0X0A // BatteryStatus()
#define BQ27220_COMMAND_VOLT            0X08 // Voltage()
#define BQ27220_COMMAND_BAT_STA         0X0A // BatteryStatus()
#define BQ27220_COMMAND_CURR            0X0C // Current()
#define BQ27220_COMMAND_REMAIN_CAPACITY 0X10 // RemaininfCapacity()
#define BQ27220_COMMAND_FCHG_CAPATICY   0X12 // FullCharageCapacity()
#define BQ27220_COMMAND_AVG_CURR        0x14 // AverageCurrent();
#define BQ27220_COMMAND_TTE             0X16 // TimeToEmpty()
#define BQ27220_COMMAND_TTF             0X18 // TimeToFull()
#define BQ27220_COMMAND_STANDBY_CURR    0X1A // StandbyCurrent()
#define BQ27220_COMMAND_STTE            0X1C // StandbyTimeToEmpty()
#define BQ27220_COMMAND_STATE_CHARGE    0X2C
#define BQ27220_COMMAND_STATE_HEALTH    0X2E
#define BQ27220_COMMAND_CHARGING_VOLT   0X30
#define BQ27220_COMMAND_CHARGING_CURR   0X32
#define BQ27220_COMMAND_RAW_CURR        0X7A
#define BQ27220_COMMAND_RAW_VOLT        0X7C

// sub-command of control


enum CURR_MODE{
    CURR_RAW,
    CURR_INSTANT,
    CURR_STANDBY,
    CURR_CHARGING,
    CURR_AVERAGE,
};

enum VOLT_MODE{
    VOLT,
    VOLT_CHARGING,
    VOLT_RWA
};

union battery_state {
    struct __st
    {
        uint16_t DSG : 1;
        uint16_t SYSDWN : 1;
        uint16_t TDA : 1;
        uint16_t BATTPRES : 1;
        uint16_t AUTH_GD : 1;
        uint16_t OCVGD : 1;
        uint16_t TCA : 1;
        uint16_t RSVD : 1;
        uint16_t CHGING : 1;
        uint16_t FC : 1;
        uint16_t OTD : 1;
        uint16_t OTC : 1;
        uint16_t SLEEP : 1;
        uint16_t OCVFALL : 1;
        uint16_t OCVCOMP : 1;
        uint16_t FD : 1;
    } st;
    uint16_t full;
};

class BQ27220{
public:
    BQ27220() : addr{BQ27220_I2C_ADDRESS}, wire(&Wire), scl(DEFAULT_SCL), sda(DEFAULT_SDA)
    {}

    bool begin()
    {
        Wire.begin(DEFAULT_SDA, DEFAULT_SCL);
        return true;
    }

    uint16_t getTemp() {
        return readWord(BQ27220_COMMAND_TEMP);
    }

    uint16_t getBatterySt(void){
        return readWord(BQ27220_COMMAND_BATTERY_ST);
    }

    bool getIsCharging(void){
        uint16_t ret = readWord(BQ27220_COMMAND_BATTERY_ST);
        bat_st.full = ret;
        return !bat_st.st.DSG;
    }

    uint16_t getRemainCap() {
        return readWord(BQ27220_COMMAND_REMAIN_CAPACITY);
    }

    uint16_t getFullChargeCap(void){
        return readWord(BQ27220_COMMAND_FCHG_CAPATICY);
    }

    uint16_t getVolt(VOLT_MODE type) {
        switch (type)
        {
        case VOLT:
            return readWord(BQ27220_COMMAND_VOLT);
            break;
        case VOLT_CHARGING:
            return readWord(BQ27220_COMMAND_CHARGING_VOLT);
            break;
        case VOLT_RWA:
            return readWord(BQ27220_COMMAND_RAW_VOLT);
            break;
        default:
            break;
        }
        return 0;
    }

    int16_t getCurr(CURR_MODE type) {
        switch (type)
        {
        case CURR_RAW:
            return (int16_t)readWord(BQ27220_COMMAND_RAW_CURR);
            break;
        case CURR_INSTANT:
            return (int16_t)readWord(BQ27220_COMMAND_CURR);
            break;
        case CURR_STANDBY:
            return (int16_t)readWord(BQ27220_COMMAND_STANDBY_CURR);
            break;
        case CURR_CHARGING:
            return (int16_t)readWord(BQ27220_COMMAND_CHARGING_CURR);
            break;
        case CURR_AVERAGE:
            return (int16_t)readWord(BQ27220_COMMAND_AVG_CURR);
            break;
        
        default:
            break;
        }
        return -1;
    }

    uint16_t readWord(uint16_t subAddress) {
        uint8_t data[2];
        i2cReadBytes(subAddress, data, 2);
        return ((uint16_t) data[1] << 8) | data[0];
    }

// sub-commands
    uint16_t getId() {
        return 0;
    }

    uint16_t readCtrlWord(uint16_t fun) {
        uint8_t msb = (fun >> 8);
        uint8_t lsb = (fun & 0x00FF);
        uint8_t cmd[2] = { lsb, msb };
        uint8_t data[2] = {0};

        i2cWriteBytes((uint8_t)BQ27220_COMMAND_CONTROL, cmd, 2);

        if (i2cReadBytes((uint8_t) 0, data, 2)) {
            return ((uint16_t)data[1] << 8) | data[0];
        }
        return 0;
    }

private:
    TwoWire *wire = NULL;
    uint8_t addr = 0;
    int scl = -1;
    int sda = -1;
    union battery_state bat_st;

    bool i2cReadBytes(uint8_t subAddress, uint8_t * dest, uint8_t count) {
        Wire.beginTransmission(addr);
        Wire.write(subAddress);
        Wire.endTransmission(true);

        Wire.requestFrom(addr, count);
        for(int i = 0; i < count; i++) {
            dest[i] = Wire.read();
        }
        return true;
    }

    bool i2cWriteBytes(uint8_t subAddress, uint8_t * src, uint8_t count) {
        Wire.beginTransmission(addr);
        Wire.write(subAddress);
        for(int i = 0; i < count; i++) {
            Wire.write(src[i]);
        }
        Wire.endTransmission(true);
        return true;
    }
    
};

#endif

