

#pragma once

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
