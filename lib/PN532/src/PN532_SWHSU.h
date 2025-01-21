
#ifndef __PN532_SWHSU_H__
#define __PN532_SWHSU_H__

#include <SoftwareSerial.h>

#include "PN532Interface.h"
#include "Arduino.h"

#define PN532_SWHSU_DEBUG

#define PN532_SWHSU_READ_TIMEOUT (1000)

class PN532_SWHSU : public PN532Interface
{
public:
    PN532_SWHSU(SoftwareSerial &serial);

    void begin();
    void wakeup();
    virtual int8_t writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0);
    int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout);

private:
    SoftwareSerial *_serial;
    uint8_t command;

    int8_t readAckFrame();

    int8_t receive(uint8_t *buf, int len, uint16_t timeout = PN532_SWHSU_READ_TIMEOUT);
};

#endif
