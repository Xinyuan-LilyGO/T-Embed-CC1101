#ifndef __DEBUG_H__
#define __DEBUG_H__

// #define DEBUG

#include "Arduino.h"

#ifdef SEEED_XIAO_M0
    #define PN532_DEBUG_SERIAL Serial
#elif defined(ARDUINO_SAMD_VARIANT_COMPLIANCE)
    #define PN532_DEBUG_SERIAL SerialUSB
#else
    #define PN532_DEBUG_SERIAL Serial
#endif


#ifdef DEBUG
#define DMSG(args...) PN532_DEBUG_SERIAL.print(args)
#define DMSG_STR(str) PN532_DEBUG_SERIAL.println(str)
#define DMSG_HEX(num)                                 \
    PN532_DEBUG_SERIAL.print(' ');                    \
    PN532_DEBUG_SERIAL.print((num >> 4) & 0x0F, HEX); \
    PN532_DEBUG_SERIAL.print(num & 0x0F, HEX)
#define DMSG_INT(num)              \
    PN532_DEBUG_SERIAL.print(' '); \
    PN532_DEBUG_SERIAL.print(num)
#else
#define DMSG(args...)
#define DMSG_STR(str)
#define DMSG_HEX(num)
#define DMSG_INT(num)
#endif

#endif
