#include "bq27220_data_memory.h"

/* ******************************************************************************
 *              T-Embed-CC1101 CEDV Gauging Configuration
 ***************************************************************************** */
// CEDV：Compensated End-of-Discharge Voltage

const BQ27220DMGaugingConfig data_memory_gauging_config = {
    .CCT = 1,        // Use CC % of FullChargeCapacity().
    .CSYNC = 0,      // RM is not changed when charge termination is reached
    .EDV_CMP = 0,    // EDV compensation is disabled.
    .SC = 1,         // Learning cycle is optimized for independent charger.
    .FIXED_EDV0 = 1, // EDV0 will always use Fixed EDV0. EDV1 and EDV2 compensation will not go below Fixed EDV0
    .FCC_LIM = 1,    // FCC is limited to Design Capacity mAh.
    .FC_FOR_VDQ = 1, // FC is required to get VDQ
    .IGNORE_SD = 1,  // Coulomb counter increments only if there is a real discharge
    .SME0 = 0,       // Smoothing towards EDV0 enable. Used with SMEN and SMEXT.
};

const BQ27220DMData gauge_data_memory[] = {
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1GaugingConfig,
        .type = BQ27220DMTypePtr16,
        .value.u32 = (uint32_t)&data_memory_gauging_config,
    },
    {
        .address = BQ27220DMAddressConfigurationRegistersOperationConfigA,
        .type = BQ27220DMTypeU16,
        .value.u16 = 0x0C8C,
    },
    {
        .address = BQ27220DMAddressConfigurationRegistersOperationConfigB,
        .type = BQ27220DMTypeU8,
        .value.u8 = 0x4C,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1FullChargeCapacity,
        .type = BQ27220DMTypeU16,
        .value.u16 = 1300,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1DesignCapacity,
        .type = BQ27220DMTypeU16,
        .value.u16 = 1300,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1EMF,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3743,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1C0,
        .type = BQ27220DMTypeU16,
        .value.u16 = 149,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1R0,
        .type = BQ27220DMTypeU16,
        .value.u16 = 867,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1T0,
        .type = BQ27220DMTypeU16,
        .value.u16 = 4030,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1R1,
        .type = BQ27220DMTypeU16,
        .value.u16 = 316,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1TC,
        .type = BQ27220DMTypeU8,
        .value.u8 = 9,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1C1,
        .type = BQ27220DMTypeU8,
        .value.u8 = 0,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD0,
        .type = BQ27220DMTypeU16,
        .value.u16 = 4183,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD10,
        .type = BQ27220DMTypeU16,
        .value.u16 = 4043,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD20,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3925,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD30,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3821,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD40,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3725,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD50,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3665,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD60,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3619,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD70,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3585,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD80,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3515,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD90,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3439,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1StartDOD100,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3299,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1EDV0,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3300,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1EDV1,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3321,
    },
    {
        .address = BQ27220DMAddressGasGaugingCEDVProfile1EDV2,
        .type = BQ27220DMTypeU16,
        .value.u16 = 3355,
    },
    {
        .address = BQ27220DMAddressCalibrationCurrentDeadband,
        .type = BQ27220DMTypeU8,
        .value.u8 = 1,
    },
    {
        .address = BQ27220DMAddressConfigurationPowerSleepCurrent,
        .type = BQ27220DMTypeI16,
        .value.i16 = 1,
    },
    {
        .type = BQ27220DMTypeEnd,
    },
};
