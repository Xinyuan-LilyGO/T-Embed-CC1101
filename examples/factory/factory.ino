
#include "utilities.h"
#include <XPowersLib.h>
#include "ui.h"

#define NFC_PRIORITY    (configMAX_PRIORITIES - 1)
#define LORA_PRIORITY   (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY (configMAX_PRIORITIES - 3)

PowersSY6970 PMU;
uint32_t cycleInterval;

SemaphoreHandle_t radioLock;

TaskHandle_t nfc_handle;
TaskHandle_t lora_handle;
TaskHandle_t ws2812_handle;
SPIClass radioSPI =  SPIClass(HSPI);

// void vTaskSuspend( TaskHandle_t xTaskToSuspend ); // 阻塞
// void vTaskResume( TaskHandle_t xTaskToResume );   // 唤醒

void multi_thread_create(void)
{
    xTaskCreate(nfc_task, "nfc_task", 1024 * 2, NULL, NFC_PRIORITY, &nfc_handle);
    xTaskCreate(lora_task, "lora_task", 1024 * 2, NULL, LORA_PRIORITY, &lora_handle);
    xTaskCreate(ws2812_task, "ws2812_task", 1024 * 2, NULL, WS2812_PRIORITY, &ws2812_handle);
    
    vTaskSuspend(nfc_handle);
    vTaskSuspend(lora_handle);
    vTaskSuspend(ws2812_handle);
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
    // byte error, address;
    // int nDevices = 0;
    // Serial.println("Scanning for I2C devices ...");
    // Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    // for(address = 0x01; address < 0x7F; address++){
    //     Wire.beginTransmission(address);
    //     // 0: success.
    //     // 1: data too long to fit in transmit buffer.
    //     // 2: received NACK on transmit of address.
    //     // 3: received NACK on transmit of data.
    //     // 4: other error.
    //     // 5: timeout
    //     error = Wire.endTransmission();
    //     if(error == 0){ // 0: success.
    //         nDevices++;
    //         log_i("I2C device found at address 0x%x\n", address);
    //     } else if(error != 2){
    //         Serial.printf("Error %d at address 0x%02X\n", error, address);
    //     }
    // }
    // if (nDevices == 0){
    //     Serial.println("No I2C devices found");
    // }

    ui_entry(); // init UI and display     SPI.begin(BOARD_SPI_SCK, -1, BOARD_SPI_MOSI); 

    lora_init(); // SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    ws2812_init();

    multi_thread_create();
}

void loop(void)
{
    lv_timer_handler();
}
