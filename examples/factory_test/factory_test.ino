
#include "utilities.h"
#include <XPowersLib.h>
#include "ui.h"
#include "TFT_eSPI.h"
#include "Audio.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t kIrLed = 2;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
const uint16_t kRecvPin = 1;
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
IRrecv irrecv(kRecvPin);
decode_results results;

uint64_t IR_recv_value = 0x10000000UL;

/*********************************************************************************
 *                               DEFINE
 *********************************************************************************/
#define FORMAT_SPIFFS_IF_FAILED true

#define NFC_PRIORITY     (configMAX_PRIORITIES - 1)
#define LORA_PRIORITY    (configMAX_PRIORITIES - 2)
#define WS2812_PRIORITY  (configMAX_PRIORITIES - 3)
#define BATTERY_PRIORITY (configMAX_PRIORITIES - 4)
#define INFARED_PRIORITY (configMAX_PRIORITIES - 5)

/*********************************************************************************
 *                              EXTERN
 *********************************************************************************/
extern uint8_t display_rotation;
extern uint8_t setting_theme;
/*********************************************************************************
 *                              TYPEDEFS
 *********************************************************************************/

Audio audio(false, 3, I2S_NUM_1);
XPowersPPM PPM;
BQ27220 bq27220;
uint32_t cycleInterval;
SemaphoreHandle_t radioLock;

TaskHandle_t nfc_handle;
TaskHandle_t lora_handle;
TaskHandle_t ws2812_handle;
TaskHandle_t battery_handle;
// TaskHandle_t infared_handle;

// wifi
// char wifi_ssid[WIFI_SSID_MAX_LEN] = "xinyuandianzi";
// char wifi_password[WIFI_PSWD_MAX_LEN] = "AA15994823428";
char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
char wifi_password[WIFI_PSWD_MAX_LEN] = {0};
const char *ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
bool wifi_is_connect = false;
bool wifi_eeprom_upd = false;
static struct tm timeinfo;
static uint32_t last_tick;

// eeprom
uint8_t eeprom_ssid[WIFI_SSID_MAX_LEN];
uint8_t eeprom_pswd[WIFI_PSWD_MAX_LEN];

// mic
int16_t i2s_readraw_buff[SAMPLE_SIZE] = {0};
size_t bytes_read = 0;
int i2s_mic_cnt = 0;

/*********************************************************************************
 *                              FUNCTION
 *********************************************************************************/

void eeprom_default_val(void)
{
    char wifi_ssid[WIFI_SSID_MAX_LEN] = "xinyuandianzi";
    char wifi_password[WIFI_PSWD_MAX_LEN] = "AA15994823428";

    EEPROM.write(0, EEPROM_UPDATA_FLAG_NUM);
    for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
        int k = i - WIFI_SSID_EEPROM_ADDR;
        if(k < WIFI_SSID_MAX_LEN) {
            EEPROM.write(i, wifi_ssid[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
        int k = i - WIFI_PSWD_EEPROM_ADDR;
        if(k < WIFI_PSWD_MAX_LEN) {
            EEPROM.write(i, wifi_password[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    EEPROM.commit();
    wifi_eeprom_upd = true;
}

void eeprom_wr(int addr, uint8_t val)
{
    if(wifi_eeprom_upd == false) {
        eeprom_default_val();
    }
    EEPROM.write(addr, val);
    EEPROM.commit();
    Serial.printf("eeprom_wr %d:%d\n", addr, val);
}

void eeprom_wr_wifi(const char *ssid, uint16_t ssid_len, const char *pwsd, uint16_t pwsd_len)
{
    Serial.printf("eeprom_wr_wifi \n%s:%d\n%s:%d\n", ssid, ssid_len, pwsd, pwsd_len);
    if(ssid_len > WIFI_SSID_MAX_LEN) 
        ssid_len = WIFI_SSID_MAX_LEN;
    if(pwsd_len > WIFI_PSWD_MAX_LEN)
        pwsd_len = WIFI_PSWD_MAX_LEN;

    if(wifi_eeprom_upd == false) {
        EEPROM.write(0, EEPROM_UPDATA_FLAG_NUM);
        wifi_eeprom_upd = true;
    }

    for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
        int k = i - WIFI_SSID_EEPROM_ADDR;
        if(k < ssid_len) {
            EEPROM.write(i, ssid[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
        int k = i - WIFI_PSWD_EEPROM_ADDR;
        if(k < pwsd_len) {
            EEPROM.write(i, pwsd[k]);
        } else {
            EEPROM.write(i, 0x00);
        }
    }
    EEPROM.commit();
}

void eeprom_init()
{
    if (!EEPROM.begin(EEPROM_SIZE_MAX)) {
        Serial.println("failed to initialise EEPROM"); delay(1000000);
    }
    uint8_t frist_flag = EEPROM.read(0);
    if(frist_flag == EEPROM_UPDATA_FLAG_NUM) {
        for(int i = WIFI_SSID_EEPROM_ADDR; i < WIFI_SSID_EEPROM_ADDR + WIFI_SSID_MAX_LEN; i++) {
            wifi_ssid[i - WIFI_SSID_EEPROM_ADDR] = EEPROM.read(i);
        }
        for(int i = WIFI_PSWD_EEPROM_ADDR; i < WIFI_PSWD_EEPROM_ADDR + WIFI_PSWD_MAX_LEN; i++) {
            wifi_password[i - WIFI_PSWD_EEPROM_ADDR] = EEPROM.read(i);
        }

        wifi_eeprom_upd = true;
        

        uint8_t theme = EEPROM.read(UI_THEME_EEPROM_ADDR);
        uint8_t rotation = EEPROM.read(UI_ROTATION_EEPROM_ADDR);

        setting_theme = (theme == 255 ? 0 : theme);
        display_rotation = (rotation == 1 ? 1 : 3);

        Serial.println("*************** eeprom ****************");
        Serial.printf("eeprom flag: %d\n", frist_flag);
        Serial.printf("eeprom SSID: %s\n", wifi_ssid);
        Serial.printf("eeprom PWSD: %s\n", wifi_password);
        Serial.printf("eeprom theme: %d\n", theme);
        Serial.printf("eeprom rotation: %d\n", rotation);
        Serial.println("***************************************");
    }
}

void multi_thread_create(void)
{
    xTaskCreate(nfc_task, "nfc_task", 1024 * 3, NULL, NFC_PRIORITY, &nfc_handle);
    xTaskCreate(lora_task, "lora_task", 1024 * 2, NULL, LORA_PRIORITY, &lora_handle);
    xTaskCreate(ws2812_task, "ws2812_task", 1024 * 2, NULL, WS2812_PRIORITY, &ws2812_handle);
    xTaskCreate(battery_task, "battery_task", 1024 * 2, NULL, BATTERY_PRIORITY, &battery_handle);
    // xTaskCreate(infared_task, "infared_task", 1024 * 2, NULL, INFARED_PRIORITY, &infared_handle);

}

void wifi_init(void)
{
    Serial.printf("SSID len: %d\n", strlen(wifi_ssid));
    Serial.printf("PWSD len: %d\n", strlen(wifi_password));
    if(strlen(wifi_ssid) == 0 || strlen(wifi_password) == 0) {
        return;
    }

    WiFi.begin(wifi_ssid, wifi_password);
    wl_status_t wifi_state = WiFi.status();
    last_tick = millis();
    while (wifi_state != WL_CONNECTED){
        delay(500);
        Serial.print(".");
        wifi_state = WiFi.status();
        if(wifi_state == WL_CONNECTED){
            wifi_is_connect = true;
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
    if(wifi_is_connect == true){
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
}

static void msg_subsribe_event(void * s, lv_msg_t * msg)
{
    LV_UNUSED(s);

    switch (msg->id)
    {
        case MSG_UI_ROTATION_ST:{ 
            
        }break;
        case MSG_UI_THEME_MODE:{
            
        }break;
    
        default:
            break;
    }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing spiffs directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

extern int music_idx;
extern char *music_list[20];

void scr8_read_music_from_SD(void)
{
    File root = SD.open("/music");
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }

    int i = 0;
    File file = root.openNextFile();
    while(file) {
        if(!file.isDirectory()) {
            
            char *file_name = (char *)file.name();
            uint16_t file_name_len = strlen(file_name);
            char *buf = (char *)ps_malloc(file_name_len + 1);
            strcpy(buf, file_name);
            music_list[i] = buf;
            i++;

            Serial.println(music_list[i-1]);
        }
        file = root.openNextFile();
    }
}

void setup(void)
{
    bool pmu_ret = false;
    bool nfc_ret = false;
    bool lora_ret = false;

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);  // Power on CC1101 and LED

    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH); 

    pinMode(ENCODER_KEY, INPUT);
    pinMode(BOARD_USER_KEY, INPUT);

    pinMode(BOARD_PN532_IRQ, OPEN_DRAIN);

    Serial.begin(115200);
    Serial.print("setup() running core ID: ");
    Serial.println(xPortGetCoreID());

    irsend.begin();
    irrecv.enableIRIn(); 

    radioLock = xSemaphoreCreateBinary();
    assert(radioLock);
    xSemaphoreGive(radioLock);

    init_microphone();

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
                nfc_ret = true;
                log_i("I2C device found PN532 at address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_2) {
                pmu_ret = true;
                log_i("I2C device found at BQ27220 address 0x%x\n", address);
            } else if(address == BOARD_I2C_ADDR_3) {
                lora_ret = true;
                log_i("I2C device found at BQ25896 address 0x%x\n", address);
            }
        }
    }
    if (nDevices == 0){
        Serial.println("No I2C devices found");
    }

    if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    Serial.println("*************** SPIFFS ****************");
    listDir(SPIFFS, "/", 0);
    Serial.println("**************************************");

    eeprom_init();

    // wifi_init();
    configTime(8 * 3600, 0, ntpServer1, ntpServer2);

    pmu_ret = PPM.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, BQ25896_SLAVE_ADDRESS);
    if(pmu_ret) {
        PPM.setSysPowerDownVoltage(3300);
        PPM.setInputCurrentLimit(3250);
        Serial.printf("getInputCurrentLimit: %d mA\n",PPM.getInputCurrentLimit());
        PPM.disableCurrentLimitPin();
        PPM.setChargeTargetVoltage(4208);
        PPM.setPrechargeCurr(64);
        PPM.setChargerConstantCurr(832);
        PPM.getChargerConstantCurr();
        Serial.printf("getChargerConstantCurr: %d mA\n",PPM.getChargerConstantCurr());
        PPM.enableADCMeasure();
        PPM.enableCharge();
    }
   
    if(lora_ret)
        lora_init(); 

    if(nfc_ret)
        nfc_init();

    ws2812_init();

    // infared_init();

    multi_thread_create();

    audio.setPinout(BOARD_VOICE_BCLK, BOARD_VOICE_LRCLK, BOARD_VOICE_DIN);
    audio.setVolume(21); // 0...21

    // audio.connecttoFS(SD, "/music/My Anata.mp3");

    // init UI and display
    ui_entry(); 
    lv_timer_create(msg_send_event, 5000, NULL);
    // lvgl msg
    lv_msg_subsribe(MSG_UI_ROTATION_ST, msg_subsribe_event, NULL);
    lv_msg_subsribe(MSG_UI_THEME_MODE, msg_subsribe_event, NULL);

    digitalWrite(TFT_BL, HIGH);

    sd_init();
    scr8_read_music_from_SD();
}

int file_cnt = 0;

void loop(void)
{
    lv_timer_handler();

    audio.loop();

    if (irrecv.decode(&results)) {
        // print() & println() can't handle printing long longs. (uint64_t)
        serialPrintUint64(results.value, HEX);
        IR_recv_value = results.value;
        Serial.println("");
        irrecv.resume();  // Receive the next value
    }

    i2s_read((i2s_port_t)EXAMPLE_I2S_CH, (char *)i2s_readraw_buff, SAMPLE_SIZE, &bytes_read, 100);
    for(int i = 0; i < 10; i++) {
        // Serial.printf("%d  ", i2s_readraw_buff[i]);
        // if(i == 9) {
        //     Serial.println(" ");
        // }
        if(i2s_readraw_buff[i] > 0) i2s_mic_cnt++;
    }
    delay(1);
}
