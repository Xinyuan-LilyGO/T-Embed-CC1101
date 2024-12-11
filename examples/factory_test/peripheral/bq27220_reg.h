

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
#define BQ27220_COMMAND_OPERATION_STATUS   0X3A
#define BQ27220_COMMAND_RAW_CURR        0X7A
#define BQ27220_COMMAND_RAW_VOLT        0X7C

#define CommandControl             (0x00u)
#define CommandAtRate              (0x02u)
#define CommandAtRateTimeToEmpty   (0x04u)
#define CommandTemperature         (0x06u)
#define CommandVoltage             (0x08u)
#define CommandBatteryStatus       (0x0Au)
#define CommandCurrent             (0x0Cu)
#define CommandRemainingCapacity   (0x10u)
#define CommandFullChargeCapacity  (0x12u)
#define CommandAverageCurrent      (0x14u)
#define CommandTimeToEmpty         (0x16u)
#define CommandTimeToFull          (0x18u)
#define CommandStandbyCurrent      (0x1Au)
#define CommandStandbyTimeToEmpty  (0x1Cu)
#define CommandMaxLoadCurrent      (0x1Eu)
#define CommandMaxLoadTimeToEmpty  (0x20u)
#define CommandRawCoulombCount     (0x22u)
#define CommandAveragePower        (0x24u)
#define CommandInternalTemperature (0x28u)
#define CommandCycleCount          (0x2Au)
#define CommandStateOfCharge       (0x2Cu)
#define CommandStateOfHealth       (0x2Eu)
#define CommandChargeVoltage       (0x30u)
#define CommandChargeCurrent       (0x32u)
#define CommandBTPDischargeSet     (0x34u)
#define CommandBTPChargeSet        (0x36u)
#define CommandOperationStatus     (0x3Au)
#define CommandDesignCapacity      (0x3Cu)
#define CommandSelectSubclass      (0x3Eu)
#define CommandMACData             (0x40u)
#define CommandMACDataSum          (0x60u)
#define CommandMACDataLen          (0x61u)
#define CommandAnalogCount         (0x79u)
#define CommandRawCurrent          (0x7Au)
#define CommandRawVoltage          (0x7Cu)
#define CommandRawIntTemp          (0x7Eu)

// sub-command of CommandControl
#define Control_CONTROL_STATUS         (0x0000u)
#define Control_DEVICE_NUMBER          (0x0001u)
#define Control_FW_VERSION             (0x0002u)
#define Control_HW_VERSION             (0x0003u)
#define Control_BOARD_OFFSET           (0x0009u)
#define Control_CC_OFFSET              (0x000Au)
#define Control_CC_OFFSET_SAVE         (0x000Bu)
#define Control_OCV_CMD                (0x000Cu)
#define Control_BAT_INSERT             (0x000Du)
#define Control_BAT_REMOVE             (0x000Eu)
#define Control_SET_SNOOZE             (0x0013u)
#define Control_CLEAR_SNOOZE           (0x0014u)
#define Control_SET_PROFILE_1          (0x0015u)
#define Control_SET_PROFILE_2          (0x0016u)
#define Control_SET_PROFILE_3          (0x0017u)
#define Control_SET_PROFILE_4          (0x0018u)
#define Control_SET_PROFILE_5          (0x0019u)
#define Control_SET_PROFILE_6          (0x001Au)
#define Control_CAL_TOGGLE             (0x002Du)
#define Control_SEALED                 (0x0030u)
#define Control_RESET                  (0x0041u)
#define Control_OERATION_STATUS        (0x0054u)
#define Control_GAUGING_STATUS         (0x0056u)
#define Control_EXIT_CAL               (0x0080u)
#define Control_ENTER_CAL              (0x0081u)
#define Control_ENTER_CFG_UPDATE       (0x0090u)
#define Control_EXIT_CFG_UPDATE_REINIT (0x0091u)
#define Control_EXIT_CFG_UPDATE        (0x0092u)
#define Control_RETURN_TO_ROM          (0x0F00u)

#define UnsealKey1 (0x0414u)
#define UnsealKey2 (0x3672u)

#define FullAccessKey (0xffffu)