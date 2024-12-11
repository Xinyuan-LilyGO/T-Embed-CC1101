

#ifndef BQ27227_H
#define BQ27227_H

#include "Arduino.h"
#include <Wire.h>
#include "bq27220.h"
#include "bq27220_reg.h"

#define DEFAULT_SCL  18
#define DEFAULT_SDA  8


// device addr
#define BQ27220_I2C_ADDRESS 0x55

// device id
#define BQ27220_ID (0x0220u)

/** Timeout for common operations. */
#define BQ27220_TIMEOUT_COMMON_US (2000000u)

/** Timeout cycle interval  */
#define BQ27220_TIMEOUT_CYCLE_INTERVAL_US (1000u)

/** Timeout cycles count helper */
#define BQ27220_TIMEOUT(timeout_us) ((timeout_us) / (BQ27220_TIMEOUT_CYCLE_INTERVAL_US))


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

typedef union OperationStatus{
    struct __reg
    {
        // Low byte, Low bit first
        bool CALMD      : 1; /**< Calibration mode enabled */
        uint8_t SEC     : 2; /**< Current security access */
        bool EDV2       : 1; /**< EDV2 threshold exceeded */
        bool VDQ        : 1; /**< Indicates if Current discharge cycle is NOT qualified or qualified for an FCC updated */
        bool INITCOMP   : 1; /**< gauge initialization is complete */
        bool SMTH       : 1; /**< RemainingCapacity is scaled by smooth engine */
        bool BTPINT     : 1; /**< BTP threshold has been crossed */
        // High byte, Low bit first
        uint8_t RSVD1   : 2; /**< Reserved */
        bool CFGUPDATE  : 1; /**< Gauge is in CONFIG UPDATE mode */
        uint8_t RSVD0   : 5; /**< Reserved */
    } reg;
    uint16_t full;
} BQ27220OperationStatus;

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

    bool getOperationStatus(BQ27220OperationStatus *oper_sta)
    {
        bool result = false;
        uint16_t data = ReadRegU16(CommandOperationStatus);
        if(data != 0)
        {
            (*oper_sta).full = data;
            result = true;
        }
        return result;
    }

    bool reset(void)
    {
        bool result = false;
        BQ27220OperationStatus operat = {0};
        do{
            controlSubCmd(Control_RESET);
            delay(10);

            uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
            while (--timeout)
            {
                if(!getOperationStatus(&operat)){
                    Serial.printf("Failed to get operation status, retries left %lu\n", timeout);
                }else if(operat.reg.INITCOMP == true){
                    break;
                }
                delay(2);
            }
            if(timeout == 0) {
                Serial.println("INITCOMP timeout after reset");
                break;
            }
            Serial.printf("Cycles left: %lu\n", timeout);
            result = true;
        } while(0);
        return result;
    }

    bool controlSubCmd(uint16_t sub_cmd)
    {
        uint8_t msb = (sub_cmd >> 8);
        uint8_t lsb = (sub_cmd & 0x00FF);
        uint8_t buf[2] = { lsb, msb };
        i2cWriteBytes(CommandControl, buf, 2);
        return true;
    }

    uint16_t ReadRegU16(uint16_t subAddress) {
        uint8_t data[2];
        i2cReadBytes(subAddress, data, 2);
        return ((uint16_t) data[1] << 8) | data[0];
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

