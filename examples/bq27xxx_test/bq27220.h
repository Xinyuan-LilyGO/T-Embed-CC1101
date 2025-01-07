

#ifndef BQ27227_H
#define BQ27227_H

#include "Arduino.h"
#include <Wire.h>
#include "bq27220_def.h"
#include "bq27220_data_memory.h"

#define DEFAULT_SCL  18
#define DEFAULT_SDA  8

typedef union {
    struct __reg
    {
        // Low byte, Low bit first
        uint8_t BATT_ID : 3; /**< Battery Identification */
        bool SNOOZE     : 1; /**< SNOOZE mode is enabled */
        bool BCA        : 1; /**< fuel gauge board calibration routine is active */
        bool CCA        : 1; /**< Coulomb Counter Calibration routine is active */
        uint8_t RSVD0   : 2; /**< Reserved */
        // High byte, Low bit first
        uint8_t RSVD1; /**< Reserved */
    } reg;
    uint16_t full;
} BQ27220ControlStatus;

typedef union BatteryStatus {
    struct __reg
    {
        // Low byte, Low bit first
        uint16_t DSG        : 1; /**< The device is in DISCHARGE */
        uint16_t SYSDWN     : 1; /**< System down bit indicating the system should shut down */
        uint16_t TDA        : 1; /**< Terminate Discharge Alarm */
        uint16_t BATTPRES   : 1; /**< Battery Present detected */
        uint16_t AUTH_GD    : 1; /**< Detect inserted battery */
        uint16_t OCVGD      : 1; /**< Good OCV measurement taken */
        uint16_t TCA        : 1; /**< Terminate Charge Alarm */
        uint16_t RSVD       : 1; /**< Reserved */
        // High byte, Low bit first
        uint16_t CHGING     : 1; /**< Charge inhibit */
        uint16_t FC         : 1; /**< Full-charged is detected */
        uint16_t OTD        : 1; /**< Overtemperature in discharge condition is detected */
        uint16_t OTC        : 1; /**< Overtemperature in charge condition is detected */
        uint16_t SLEEP      : 1; /**< Device is operating in SLEEP mode when set */
        uint16_t OCVFALL    : 1; /**< Status bit indicating that the OCV reading failed due to current */
        uint16_t OCVCOMP    : 1; /**< An OCV measurement update is complete */
        uint16_t FD         : 1; /**< Full-discharge is detected */
    } reg;
    uint16_t full;
}BQ27220BatteryStatus;

typedef enum {
    Bq27220OperationStatusSecSealed = 0b11,
    Bq27220OperationStatusSecUnsealed = 0b10,
    Bq27220OperationStatusSecFull = 0b01,
} Bq27220OperationStatusSec;

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
        return wire->begin(DEFAULT_SDA, DEFAULT_SCL);
    }

    bool getIsCharging(void){
        BQ27220BatteryStatus batt;
        if(getBatteryStatus(&batt)) {
            return !batt.reg.DSG;
        }
        return false;
    }

    bool parameterCheck(uint16_t addr, uint32_t value, bool update);
    bool dateMemoryCheck(const BQ27220DMData *data_memory, bool update);

    bool init(const BQ27220DMData *data_memory);
    bool reset(void);

    // Sealed Access
    bool sealAccess(void);
    bool unsealAccess(void);
    bool fullAccess(void);

    // get
    uint16_t getDeviceNumber(void);  // sub-commands
    uint16_t getVoltage(void);
    int16_t getCurrent(void);
    bool getControlStatus(BQ27220ControlStatus *ctrl_sta);
    bool getBatteryStatus(BQ27220BatteryStatus *batt_sta);
    bool getOperationStatus(BQ27220OperationStatus *oper_sta);
    bool getGaugingStatus(void);
    uint16_t getTemperature(void);
    uint16_t getFullChargeCapacity(void);
    uint16_t getDesignCapacity(void);
    uint16_t getRemainingCapacity(void);
    uint16_t getStateOfCharge(void);
    uint16_t getStateOfHealth(void);

    // i2c
    uint16_t readRegU16(uint16_t reg) {
        uint8_t data[2];
        i2cReadBytes(reg, data, 2);
        return ((uint16_t) data[1] << 8) | data[0];
    }

    // uint16_t readCtrlWord(uint16_t fun) {
    //     uint8_t msb = (fun >> 8);
    //     uint8_t lsb = (fun & 0x00FF);
    //     uint8_t cmd[2] = { lsb, msb };
    //     uint8_t data[2] = {0};

    //     i2cWriteBytes((uint8_t)BQ27220_COMMAND_CONTROL, cmd, 2);

    //     if (i2cReadBytes((uint8_t) 0, data, 2)) {
    //         return ((uint16_t)data[1] << 8) | data[0];
    //     }
    //     return 0;
    // }

    bool controlSubCmd(uint16_t sub_cmd)
    {
        uint8_t msb = (sub_cmd >> 8);
        uint8_t lsb = (sub_cmd & 0x00FF);
        uint8_t buf[2] = { lsb, msb };
        i2cWriteBytes(CommandControl, buf, 2);
        return true;
    }

    bool i2cReadBytes(uint8_t reg, uint8_t * dest, uint8_t count) {
        wire->beginTransmission(addr);
        wire->write(reg);
        wire->endTransmission(true);

        wire->requestFrom(addr, count);
        for(int i = 0; i < count; i++) {
            dest[i] = wire->read();
        }
        return true;
    }

    bool i2cWriteBytes(uint8_t reg, uint8_t * src, uint8_t count) {
        wire->beginTransmission(addr);
        wire->write(reg);
        for(int i = 0; i < count; i++) {
            wire->write(src[i]);
        }
        wire->endTransmission(true);
        return true;
    }

private:
    TwoWire *wire = NULL;
    uint8_t addr = 0;
    int scl = -1;
    int sda = -1;
    BQ27220BatteryStatus bat_st;
};

#endif

