
#include "peripheral.h"

float lora_freq = 315.0;

CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2);
int lora_mode = LORA_MODE_SEND;
int lora_recv_success = 0;
int lora_recv_rssi = 0;
int lora_init_st = false;
String lora_recv_str;

// recv
static volatile bool receivedFlag = false;

static void recvSetFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

// send
static int transmissionState = RADIOLIB_ERR_NONE;
static volatile bool transmittedFlag = false;

static void sendSetFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}


void lora_init(void)
{
    //Set antenna frequency settings
    pinMode(BOARD_LORA_SW1, OUTPUT);
    pinMode(BOARD_LORA_SW0, OUTPUT);
    // SW1:1  SW0:0 --- 315MHz
    // SW1:0  SW0:1 --- 868/915MHz
    // SW1:1  SW0:1 --- 434MHz
    digitalWrite(BOARD_LORA_SW1, HIGH);
    digitalWrite(BOARD_LORA_SW0, LOW);
    lora_freq = 315.0;

    // 
    SPI.end();
    SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); 

    // initialize CC1101
    Serial.print(F("[CC1101] Initializing ... "));
    Serial.print(lora_freq);
    Serial.println(" MHz ");

    int state = radio.begin(lora_freq);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        lora_init_st = true;
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        lora_init_st = false;
        return;
    }

    if(lora_mode == LORA_MODE_SEND){     // send
        radio.setPacketSentAction(sendSetFlag);
         Serial.print(F("[CC1101] Sending first packet ... "));

        // you can transmit C-string or Arduino string up to
        // 64 characters long
        transmissionState = radio.startTransmit("Hello World!");
    } else if(lora_mode == LORA_MODE_RECV) { // recv
        
        radio.setPacketReceivedAction(recvSetFlag);
        // start listening for packets
        Serial.print(F("[CC1101] Starting to listen ... "));
        state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true);
        }
    }
}

void lora_mode_sw(int m)
{
    if(m == LORA_MODE_RECV) {
        radio.setPacketReceivedAction(recvSetFlag);
        // start listening for packets
        Serial.print(F("[CC1101] Starting to listen ... "));
        int state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("success!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true);
        }
    } else if(m == LORA_MODE_SEND) {
        radio.setPacketSentAction(sendSetFlag);
    }
    lora_mode = m;
}

int lora_get_mode(void)
{
    return lora_mode;
}

bool lora_is_init(void)
{
    return lora_init_st;
}

void lora_send(const char *str)
{
    // check if the previous transmission finished
    if(xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE){
        return;
    }

    if(transmittedFlag) {
        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));
            Serial.println(str);

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // wait a second before transmitting again

        // send another one
        Serial.print(F("[CC1101] Sending another packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio.startTransmit(str);
        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(F("transmission finished!"));

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }
    }
    xSemaphoreGive(radioLock);
}



void lora_recv(void)
{
    // check if the flag is set
    if(xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE){
        return;
    }

    if(receivedFlag) {
        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        int state = radio.readData(lora_recv_str);

        if (state == RADIOLIB_ERR_NONE) {
            lora_recv_success = 1;
        // packet was successfully received
        Serial.println(F("[CC1101] Received packet!"));

        // print data of the packet
        Serial.print(F("[CC1101] Data:\t\t"));
        Serial.println(lora_recv_str);

        // print RSSI (Received Signal Strength Indicator)
        // of the last received packet
        Serial.print(F("[CC1101] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        lora_recv_rssi = (int) radio.getRSSI();
        Serial.println(F(" dBm"));

        // print LQI (Link Quality Indicator)
        // of the last received packet, lower is better
        Serial.print(F("[CC1101] LQI:\t\t"));
        Serial.println(radio.getLQI());

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));

        } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);

        }

        // put module back to listen mode
        radio.startReceive();
    }
    xSemaphoreGive(radioLock);
}

void lora_task(void *param)
{
    int count = 0;
    vTaskSuspend(lora_handle);
    while (1)
    {
        if(lora_mode == LORA_MODE_RECV) {
            lora_recv();
        }
        // String str = "Hello World! #" + String(count++);
        // Serial.println(str);
        // lora_send(str.c_str());
        // lora_recv();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
