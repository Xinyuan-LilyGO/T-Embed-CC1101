
#include "utilities.h"
#include <XPowersLib.h>
#include "ui.h"
#include <WiFi.h>
/*********************************************************************************
 *                               DEFINE
 *********************************************************************************/

#define NFC_PRIORITY     (configMAX_PRIORITIES - 1)
#define LORA_PRIORITY    (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY  (configMAX_PRIORITIES - 3)
#define BATTERY_PRIORITY (configMAX_PRIORITIES - 4)

/*********************************************************************************
 *                              TYPEDEFS
 *********************************************************************************/
PowersSY6970 PMU;
uint32_t cycleInterval;

SemaphoreHandle_t radioLock;

TaskHandle_t nfc_handle;
TaskHandle_t lora_handle;
TaskHandle_t ws2812_handle;
TaskHandle_t battery_handle;

// wifi
const char* ssid = "xinyuandianzi";
const char* password = "AA15994823428";
const char *ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
bool is_connect_wifi = false;
static struct tm timeinfo;
static uint32_t last_tick;

// void vTaskSuspend( TaskHandle_t xTaskToSuspend ); // 阻塞
// void vTaskResume( TaskHandle_t xTaskToResume );   // 唤醒

/*********************************************************************************
 *                              FUNCTION
 *********************************************************************************/
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

void wifi_init(void)
{
    WiFi.begin(ssid, password);
    wl_status_t wifi_state = WiFi.status();
    last_tick = millis();
    while (wifi_state != WL_CONNECTED){
        delay(500);
        Serial.print(".");
        wifi_state = WiFi.status();
        if(wifi_state == WL_CONNECTED){
            is_connect_wifi = true;
            Serial.println("WiFi connected!");
            configTime(8 * 3600, 0, ntpServer1, ntpServer2);
            break;
        }
        if (millis() - last_tick > 5000) {
            Serial.println("WiFi connected falied!");
            last_tick = millis();
            break;
        }
    }
}

static void msg_send_event(lv_timer_t *t)
{
    if(is_connect_wifi == true){
        if (!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain time");
            return;
        }
        // Serial.println(&timeinfo, "%F %T %A"); // 格式化输出
        timeinfo.tm_hour = timeinfo.tm_hour % 12;
        lv_msg_send(MSG_CLOCK_HOUR, &timeinfo.tm_hour);
        lv_msg_send(MSG_CLOCK_MINUTE, &timeinfo.tm_min);
        lv_msg_send(MSG_CLOCK_SECOND, &timeinfo.tm_sec);
    }
    // eeprom_write(-1, 0);
}

void setup(void)
{
    Serial.begin(115200);
    // int start_delay = 3;
    // while (start_delay) {
    //     Serial.print(start_delay);
    //     delay(1000);    
    //     start_delay--;
    // }

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

    wifi_init();

    ui_entry(); // init UI and display     SPI.begin(BOARD_SPI_SCK, -1, BOARD_SPI_MOSI); 

    lora_init(); // SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);

    nfc_init();

    ws2812_init();

    lv_timer_create(msg_send_event, 10000, NULL);

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
