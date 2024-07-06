// /*
//  * SimpleReceiver.cpp
//  *
//  * Demonstrates receiving ONLY NEC protocol IR codes with IRremote
//  * If no protocol is defined, all protocols (except Bang&Olufsen) are active.
//  *
//  *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
//  *
//  ************************************************************************************
//  * MIT License
//  *
//  * Copyright (c) 2020-2023 Armin Joachimsmeyer
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy
//  * of this software and associated documentation files (the "Software"), to deal
//  * in the Software without restriction, including without limitation the rights
//  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  * copies of the Software, and to permit persons to whom the Software is furnished
//  * to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in all
//  * copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
//  * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
//  * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//  *
//  ************************************************************************************
//  */

// /*
//  * Specify which protocol(s) should be used for decoding.
//  * If no protocol is defined, all protocols (except Bang&Olufsen) are active.
//  * This must be done before the #include <IRremote.hpp>
//  */
// //#define DECODE_DENON        // Includes Sharp
// //#define DECODE_JVC
// //#define DECODE_KASEIKYO
// //#define DECODE_PANASONIC    // alias for DECODE_KASEIKYO
// //#define DECODE_LG
// #define DECODE_NEC          // Includes Apple and Onkyo. To enable all protocols , just comment/disable this line.
// //#define DECODE_SAMSUNG
// //#define DECODE_SONY
// //#define DECODE_RC5
// //#define DECODE_RC6

// //#define DECODE_BOSEWAVE
// //#define DECODE_LEGO_PF
// //#define DECODE_MAGIQUEST
// //#define DECODE_WHYNTER
// //#define DECODE_FAST

// //#define DECODE_DISTANCE_WIDTH // Universal decoder for pulse distance width protocols
// //#define DECODE_HASH         // special decoder for all protocols

// //#define DECODE_BEO          // This protocol must always be enabled manually, i.e. it is NOT enabled if no protocol is defined. It prevents decoding of SONY!

// //#define DEBUG               // Activate this for lots of lovely debug output from the decoders.

// //#define RAW_BUFFER_LENGTH  180  // Default is 112 if DECODE_MAGIQUEST is enabled, otherwise 100.

// #include <Arduino.h>
// #include "utilities.h"

// /*
//  * This include defines the actual pin number for pins like IR_RECEIVE_PIN, IR_SEND_PIN for many different boards and architectures
//  */
// // #include "PinDefinitionsAndMore.h"
// #if !defined (FLASHEND)
// #define FLASHEND 0xFFFF // Dummy value for platforms where FLASHEND is not defined
// #endif
// #if !defined (RAMEND)
// #define RAMEND 0xFFFF // Dummy value for platforms where RAMEND is not defined
// #endif
// #if !defined (RAMSIZE)
// #define RAMSIZE 0xFFFF // Dummy value for platforms where RAMSIZE is not defined
// #endif

// /*
//  * Helper macro for getting a macro definition as string
//  */
// #if !defined(STR_HELPER)
// #define STR_HELPER(x) #x
// #define STR(x) STR_HELPER(x)
// #endif


// #include <IRremote.hpp> // include the library

// uint16_t infared_cmd = 0;

// void infared_init() {
//     pinMode(BOARD_IR_EN, OUTPUT);
//     digitalWrite(BOARD_IR_EN, HIGH);
    
//     // Just to know which program is running on my Arduino
//     Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

//     // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
//     IrReceiver.begin(BOARD_IR_RX, DISABLE_LED_FEEDBACK);

//     Serial.print(F("Ready to receive IR signals of protocols: "));
//     printActiveIRProtocols(&Serial);
//     Serial.println(F("at pin " STR(BOARD_IR_RX)));
// }

// uint16_t infared_get_cmd(void)
// {
//     uint16_t cmd = 0;

//     if(infared_cmd != 0) {
//         cmd = infared_cmd;
//         infared_cmd = 0;
//     }

//     return cmd; 
// }

// void infared_task(void *param)
// {
    
//     delay(100);
// }
