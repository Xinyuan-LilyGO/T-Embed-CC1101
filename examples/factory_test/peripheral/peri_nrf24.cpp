

#include "peripheral.h"

#define NRF24L01_CS 44  // SPI Chip Select
#define NRF24L01_CE 43  // Chip Enable Activates RX or TX(High) mode
#define NRF24L01_IQR -1 // Maskable interrupt pin. Active low
#define NRF24L01_MOSI 9
#define NRF24L01_MISO 10
#define NRF24L01_SCK 11

int nrf24_mode = NRF24_MODE_SEND;
bool nrf24_init_flag = false;

nRF24 radio24 = new Module(NRF24L01_CS, NRF24L01_IQR, NRF24L01_CE);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

void setFlag(void)
{
    // we sent a packet, set the flag
    transmittedFlag = true;
}

bool containsSubstring(const std::string &mainStr, const std::string &subStr)
{
    if (subStr.empty())
        return true;
    return mainStr.find(subStr) != std::string::npos;
}

void nrf24_init(void)
{
    SPI.end();
    SPI.begin(NRF24L01_SCK, NRF24L01_MISO, NRF24L01_MOSI);

    // initialize nRF24 with default settings
    Serial.print(F("[nRF24] Initializing ... "));
    int state = radio24.begin();
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
        nrf24_init_flag = true;
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        nrf24_init_flag = false;
        return;
    }

    radio24.setOutputPower(0);

    // while(1){
    //     radio24.transmitDirect();
    // }

    nrf24_set_mode(nrf24_mode);
}

bool nrf24_is_init(void)
{
    return nrf24_init_flag;
}

int count = 0;

void nrf24_send(const char *str)
{
    // check if the previous transmission finished
    if (xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    if (transmissionState == RADIOLIB_ERR_NONE)
    {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));

        // NOTE: when using interrupt-driven transmit method,
        //       it is not possible to automatically measure
        //       transmission data rate using getDataRate()
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio24.finishTransmit();

    // wait a second before transmitting again
    // delay(1000);

    // send another one
    Serial.print(F("[nRF24] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 32 characters long
    // String str = "Hello World! #" + String(count++);
    transmissionState = radio24.startTransmit(str);

    //
    xSemaphoreGive(radioLock);
}

void nrf24_recv(void)
{
    // check if the previous transmission finished
    if (xSemaphoreTake(radioLock, portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    String str;
    int state = radio24.readData(str);

    // you can also read received data as byte array
    /*
      byte byteArr[8];
      int numBytes = radio24.getPacketLength();
      int state = radio24.readData(byteArr, numBytes);
    */

    if (state == RADIOLIB_ERR_NONE)
    {
        if (containsSubstring(str.c_str(), "Hello World! #"))
        {
            // packet was successfully received
            Serial.println(F("[nRF24] Received packet!"));
            // print data of the packet
            Serial.print(F("[nRF24] Data:\t\t"));
            Serial.println(str);
        }
    }
    else
    {
        // some other error occurred
        Serial.print(F("[nRF24] Failed, code "));
        Serial.println(state);
    }

    // put module back to listen mode
    radio24.startReceive();

    //
    xSemaphoreGive(radioLock);
}

void nrf24_task(void *param)
{
    int cont = 0;
    vTaskSuspend(nrf24_handle);
    while (1)
    {
        if(nrf24_is_init()) {
            if (nrf24_mode == NRF24_MODE_RECV)
            {
                // you can read received data as an Arduino String
                String str;
                int state = radio24.readData(str);

                // you can also read received data as byte array
                /*
                byte byteArr[8];
                int numBytes = radio24.getPacketLength();
                int state = radio24.readData(byteArr, numBytes);
                */

                if (state == RADIOLIB_ERR_NONE)
                {
                    if (containsSubstring(str.c_str(), "Hello World! #"))
                    {
                        // packet was successfully received
                        Serial.println(F("[nRF24] Received packet!"));
                        // print data of the packet
                        Serial.print(F("[nRF24] Data:\t\t"));
                        Serial.println(str);

                        ws2812_pos_demo1();
                    }
                }
                else
                {
                    // some other error occurred
                    Serial.print(F("[nRF24] Failed, code "));
                    Serial.println(state);
                }

                // put module back to listen mode
                radio24.startReceive();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int nrf24_get_mode(void)
{
    return nrf24_mode;
}

void nrf24_set_mode(int mode)
{
    nrf24_mode = mode;
    if (nrf24_mode == NRF24_MODE_SEND)
    {
        // set transmit address
        // NOTE: address width in bytes MUST be equal to the
        //       width set in begin() or setAddressWidth()
        //       methods (5 by default)
        byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
        Serial.print(F("[nRF24] Setting transmit pipe ... "));
        int state = radio24.setTransmitPipe(addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
            {
                delay(10);
            }
        }

        // set the function that will be called
        // when packet transmission is finished
        radio24.setPacketSentAction(setFlag);

        // start transmitting the first packet
        Serial.print(F("[nRF24] Sending first packet ... "));

        // you can transmit C-string or Arduino string up to
        // 256 characters long
        transmissionState = radio24.startTransmit("Hello World!");
    }
    else
    {
        byte addr[] = {0x01, 0x23, 0x45, 0x67, 0x89};
        int state = radio24.setReceivePipe(0, addr);
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
                ;
        }

        // set the function that will be called
        // when new packet is received
        radio24.setPacketReceivedAction(setFlag);

        // start listening
        Serial.print(F("[nRF24] Starting to listen ... "));
        state = radio24.startReceive();
        if (state == RADIOLIB_ERR_NONE)
        {
            Serial.println(F("success!"));
        }
        else
        {
            Serial.print(F("failed, code "));
            Serial.println(state);
            while (true)
                ;
        }
    }
}
