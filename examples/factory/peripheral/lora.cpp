
#include "peripheral.h"
CC1101 radio = new Module(BOARD_LORA_CS, BOARD_LORA_IO0, -1, BOARD_LORA_IO2);

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
    float lora_freq = 0;

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
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // set the function that will be called
    // when packet transmission is finished
    radio.setPacketSentAction(sendSetFlag);

    // start transmitting the first packet
    // Serial.print(F("[CC1101] Sending first packet ... "));

    // // you can transmit C-string or Arduino string up to
    // // 64 characters long
    // transmissionState = radio.startTransmit("Hello World!");
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

void lora_task(void *param)
{
    int count = 0;
    vTaskSuspend(lora_handle);
    while (1)
    {
        String str = "Hello World! #" + String(count++);
        Serial.println(str);
        // lora_send(str.c_str());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


