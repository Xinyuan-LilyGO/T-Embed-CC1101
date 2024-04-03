
#include "utilities.h"
#include <XPowersLib.h>
#include "ui.h"
#include "BQ25896.h"

#define NFC_PRIORITY     (configMAX_PRIORITIES - 1)
#define LORA_PRIORITY    (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY  (configMAX_PRIORITIES - 3)
#define BATTERY_PRIORITY (configMAX_PRIORITIES - 4)

PowersSY6970 PMU;
uint32_t cycleInterval;
BQ25896  battery_charging(Wire);

SemaphoreHandle_t radioLock;

TaskHandle_t nfc_handle;
TaskHandle_t lora_handle;
TaskHandle_t ws2812_handle;
TaskHandle_t battery_handle;

// void vTaskSuspend( TaskHandle_t xTaskToSuspend ); // 阻塞
// void vTaskResume( TaskHandle_t xTaskToResume );   // 唤醒

void battery_task(void *param)
{
    vTaskSuspend(battery_handle);
    while (1)
    {
        battery_charging.properties();
        Serial.println("Battery Management System Parameter : \n===============================================================");
        Serial.print("VBUS : "); Serial.println(battery_charging.getVBUS());
        Serial.print("VSYS : "); Serial.println(battery_charging.getVSYS());
        Serial.print("VBAT : "); Serial.println(battery_charging.getVBAT());
        Serial.print("ICHG : "); Serial.println(battery_charging.getICHG(),4);
        Serial.print("TSPCT : "); Serial.println(battery_charging.getTSPCT());
        Serial.print("Temperature : "); Serial.println(battery_charging.getTemperature());
        
        Serial.print("FS_Current Limit : "); Serial.println(battery_charging.getFast_Charge_Current_Limit());
        Serial.print("IN_Current Limit : "); Serial.println(battery_charging.getInput_Current_Limit());
        Serial.print("PRE_CHG_Current Limit : "); Serial.println(battery_charging.getPreCharge_Current_Limit());
        Serial.print("TERM_Current Limit : "); Serial.println(battery_charging.getTermination_Current_Limit());

        Serial.print("Charging Status : "); Serial.println(battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::NOT_CHARGING?" not charging":
                                                                (battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::PRE_CHARGE ?" pre charging":
                                                                (battery_charging.getCHG_STATUS()==BQ25896::CHG_STAT::FAST_CHARGE?" Fast charging":"charging done"))); 
        
        Serial.print("VBUS Status : "); Serial.println(battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::NO_INPUT?" not input":
                                                                (battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::USB_HOST ?" USB host":
                                                                (battery_charging.getVBUS_STATUS()==BQ25896::VBUS_STAT::ADAPTER?" Adapter":"OTG")));
        
        Serial.print("VSYS Status : "); Serial.println(battery_charging.getVSYS_STATUS()==BQ25896::VSYS_STAT::IN_VSYSMIN?" In VSYSMIN regulation (BAT < VSYSMIN)":
                                                            "Not in VSYSMIN regulation (BAT > VSYSMIN)");
        
        Serial.print("Temperature rank : "); Serial.println(battery_charging.getTemp_Rank()==BQ25896::TS_RANK::NORMAL?" Normal":
                                                                (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::WARM ?" Warm":
                                                                (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::COOL?" Cool":
                                                                (battery_charging.getTemp_Rank()==BQ25896::TS_RANK::COLD?" Cold":"HOT"))));
        
        Serial.print("Charger fault status  : "); Serial.println(battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::NORMAL?" Normal":
                                                                (battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::INPUT_FAULT ?" Input Fault":
                                                                (battery_charging.getCHG_Fault_STATUS()==BQ25896::CHG_FAULT::THERMAL_SHUTDOWN?" Thermal Shutdown":"TIMER_EXPIRED")));
        delay(1000);
    }
}


void multi_thread_create(void)
{
    xTaskCreate(nfc_task, "nfc_task", 1024 * 3, NULL, NFC_PRIORITY, &nfc_handle);
    xTaskCreate(lora_task, "lora_task", 1024 * 2, NULL, LORA_PRIORITY, &lora_handle);
    xTaskCreate(ws2812_task, "ws2812_task", 1024 * 2, NULL, WS2812_PRIORITY, &ws2812_handle);
    xTaskCreate(battery_task, "battery_task", 1024 * 2, NULL, BATTERY_PRIORITY, &battery_handle);
    
    // vTaskSuspend(nfc_handle);
    // vTaskSuspend(lora_handle);
    // vTaskSuspend(ws2812_handle);
    // vTaskSuspend(battery_handle);
}

void setup(void)
{
    Serial.begin(115200);
    int start_delay = 3;
    while (start_delay) {
        Serial.print(start_delay);
        delay(1000);    
        start_delay--;
    }

    Serial.print("setup() running core ID: ");
    Serial.println(xPortGetCoreID());

    radioLock = xSemaphoreCreateBinary();
    assert(radioLock);
    xSemaphoreGive(radioLock);

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
            if(address == BOARD_I2C_ADDR_1) {
                log_i("I2C device found PN532 at address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_2) {
                log_i("I2C device found at PMU address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_3) {
                log_i("I2C device found at BQ25896 address 0x%x\n", address);
            }
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found");
    }

    battery_charging.begin();

    ui_entry(); // init UI and display     SPI.begin(BOARD_SPI_SCK, -1, BOARD_SPI_MOSI); 

    lora_init(); // SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    nfc_init();

    ws2812_init();

    multi_thread_create();
}

uint32_t prev_tick = 0;
void loop(void)
{
    lv_timer_handler();

    if(millis() - prev_tick > 3000){
        prev_tick = millis();
        
    }
}
