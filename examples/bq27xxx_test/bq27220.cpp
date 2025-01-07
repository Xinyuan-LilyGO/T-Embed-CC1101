#include "bq27220.h"
#include "bq27220_data_memory.h"

#define BQ27220_ID (0x0220u)

/** Delay between we ask chip to load data to MAC and it become valid. Fails at ~500us. */
#define BQ27220_SELECT_DELAY_US (1000u)

/** Delay between 2 control operations(like unseal or full access). Fails at ~2500us.*/
#define BQ27220_MAGIC_DELAY_US (5000u)

/** Timeout for common operations. */
#define BQ27220_TIMEOUT_COMMON_US (2000000u)

/** Timeout for reset operation. Normally reset takes ~2s. */
#define BQ27220_TIMEOUT_RESET_US (4000000u)

/** Timeout cycle interval  */
#define BQ27220_TIMEOUT_CYCLE_INTERVAL_US (1000u)

/** Timeout cycles count helper */
#define BQ27220_TIMEOUT(timeout_us) ((timeout_us) / (BQ27220_TIMEOUT_CYCLE_INTERVAL_US))


bool BQ27220::parameterCheck(uint16_t addr, uint32_t value, bool update)
{
    bool ret = false;

    uint8_t buffer[6] = {0};
    uint8_t old_data[4] = {0};


    return ret;
}

bool BQ27220::dateMemoryCheck(const BQ27220DMData *data_memory, bool update)
{
    if(update) {

    }

    // Process data memory records
    bool result = true;
    while (data_memory->type != BQ27220DMTypeEnd)
    {
        if(data_memory->type == BQ27220DMTypeWait) {
            delayMicroseconds(data_memory->value.u32);
        } else if(data_memory->type == BQ27220DMTypeU8) {

        }
        
        data_memory++;
    }
    

    return result;
}

bool BQ27220::init(const BQ27220DMData *data_memory)
{
    bool result = false;
    bool reset_and_provisioning_required = false;

    do{
        uint16_t data = getDeviceNumber();
        if(data != BQ27220_ID) {
            Serial.printf("(%d) Invalid Device Number %04x != 0x0220\n", __LINE__, data);
            break;
        }
        
        // Unseal device since we are going to read protected configuration
        if(!unsealAccess()) {
            break;
        }

        // Try to recover gauge from forever init
        BQ27220OperationStatus operat;
        if(!getOperationStatus(&operat)) {
            break;
        }
        if(!operat.reg.INITCOMP || operat.reg.CFGUPDATE) {
            Serial.printf("(%d) Incorrect state, reset needed\n", __LINE__);
            reset_and_provisioning_required = true;
        }

        // Ensure correct profile is selected


        // Ensure correct configuration loaded into gauge DataMemory
        // Only if reset is not required, otherwise we don't
        if(!reset_and_provisioning_required) {

        }

        // Reset needed
        if(reset_and_provisioning_required) {
            if(!reset()) {
                Serial.printf("(%d) Failed to reset device\n", __LINE__);
            }

            // Get full access to read and modify parameters
            // Also it looks like this step is totally unnecessary
            if(!fullAccess()) {
                break;
            }

            // Update memory
            // TODO
        }

        if(!sealAccess()) {
            Serial.printf("(%d) Seal failed\n", __LINE__);
            break;
        }

    } while(0);
    return result;
}

bool BQ27220::reset(void)
{
    bool result = false;
    do{
        controlSubCmd(Control_RESET);
        // delay(10);

        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_RESET_US);
        BQ27220OperationStatus operat = {0};
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
                Serial.printf("Failed to get operation status, retries left %lu\n", timeout);
            }else if(operat.reg.INITCOMP == true){
                break;
            }
            delayMicroseconds(BQ27220_TIMEOUT_CYCLE_INTERVAL_US); // delay(2);
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

// Sealed Access
bool BQ27220::sealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC == Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(Control_SEALED);
        // delay(10);
        delayMicroseconds(BQ27220_SELECT_DELAY_US);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            Serial.printf("Seal failed %u\n", operat.reg.SEC);
            break;
        }
        result = true;
    } while(0);

    return result;
}

bool BQ27220::unsealAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};
    do{
        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecSealed)
        {
            result = true;
            break;
        }

        controlSubCmd(UnsealKey1);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); // delay(10);
        controlSubCmd(UnsealKey2);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US);  // delay(10);

        getOperationStatus(&operat);
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed)
        {
            Serial.printf("Unseal failed %u\n", operat.reg.SEC);
            break;
        }
        result = true;
    } while (0);

    return result;
}

bool BQ27220::fullAccess(void) 
{
    bool result = false;
    BQ27220OperationStatus operat = {0};

    do{
        uint32_t timeout = BQ27220_TIMEOUT(BQ27220_TIMEOUT_COMMON_US);
        while (--timeout > 0)
        {
            if(!getOperationStatus(&operat)){
                Serial.printf("Failed to get operation status, retries left %lu\n", timeout);
            }else {
                break;
            }
        }
        if(timeout == 0) {
            Serial.println("Failed to get operation status");
            break;
        }
        // Serial.printf("Cycles left: %lu\n", timeout);

        // Already full access
        if(operat.reg.SEC == Bq27220OperationStatusSecFull){
            result = true;
            break;
        }
        // Must be unsealed to get full access
        if(operat.reg.SEC != Bq27220OperationStatusSecUnsealed){
            Serial.println("Not in unsealed state");
            break;
        }

        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);
        controlSubCmd(FullAccessKey);
        delayMicroseconds(BQ27220_MAGIC_DELAY_US); //delay(10);

        if(!getOperationStatus(&operat)){
            Serial.println("Status query failed");
            break;
        }
        if(operat.reg.SEC != Bq27220OperationStatusSecFull){
            Serial.printf("Full access failed %u\n", operat.reg.SEC);
            break;
        }
        result = true;
    } while (0);
    return result;
}

uint16_t BQ27220::getDeviceNumber(void)
{
    uint16_t devid = 0;
    // Request device number(chip PN)
    controlSubCmd(Control_DEVICE_NUMBER);
    // Enterprise wait(MAC read fails if less than 500us)
    // bqstudio uses ~15ms 
    delayMicroseconds(BQ27220_SELECT_DELAY_US); // delay(15);
    // Read id data from MAC scratch space
    i2cReadBytes(CommandMACData, (uint8_t *)&devid, 2);

    // Serial.printf("device number:0x%x\n", devid);
    return devid;
}

uint16_t BQ27220::getVoltage(void)
{
    return readRegU16(CommandVoltage);
}
int16_t BQ27220::getCurrent(void)
{
    return readRegU16(CommandCurrent);
}
bool BQ27220::getControlStatus(BQ27220ControlStatus *ctrl_sta)
{
    bool result = false;
    uint16_t data = readRegU16(CommandControl);
    if(data != 0)
    {
        (*ctrl_sta).full = data;
        result = true;
    }
    return result;
}
bool BQ27220::getBatteryStatus(BQ27220BatteryStatus *batt_sta)
{
    bool result = false;
    uint16_t data = readRegU16(CommandBatteryStatus);
    if(data != 0)
    {
        (*batt_sta).full = data;
        result = true;
    }
    return result;
}
bool BQ27220::getOperationStatus(BQ27220OperationStatus *oper_sta)
{
    bool result = false;
    uint16_t data = readRegU16(CommandOperationStatus);
    if(data != 0)
    {
        (*oper_sta).full = data;
        result = true;
    }
    return result;
}
bool BQ27220::getGaugingStatus(void)
{
    return 0;
}
uint16_t BQ27220::getTemperature(void)
{
    return readRegU16(CommandTemperature);
}
uint16_t BQ27220::getFullChargeCapacity(void)
{
    return readRegU16(CommandFullChargeCapacity);
}
uint16_t BQ27220::getDesignCapacity(void)
{
    return readRegU16(CommandDesignCapacity);
}
uint16_t BQ27220::getRemainingCapacity(void)
{
    return readRegU16(CommandRemainingCapacity);
}
uint16_t BQ27220::getStateOfCharge(void)
{
    return readRegU16(CommandStateOfCharge);
}
uint16_t BQ27220::getStateOfHealth(void)
{
    return readRegU16(CommandStateOfHealth);
}
