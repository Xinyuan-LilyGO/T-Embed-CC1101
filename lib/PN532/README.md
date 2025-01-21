# NFC library

This is an library for PN532 to use NFC technology.
It is for [NFC Shield](https://www.seeedstudio.com/NFC-Shield-V2-0.html) and [Grove - NFC](https://www.seeedstudio.com/Grove-NFC.html).

[![NFC Shield](https://statics3.seeedstudio.com/images/113030001%201.jpg)](https://www.seeedstudio.com/NFC-Shield-V2-0.html)
[![Grove - NFC](https://statics3.seeedstudio.com/images/product/grove%20nfc.jpg)](https://www.seeedstudio.com/Grove-NFC.html)

## Features

- Support all interfaces of PN532 (I2C, SPI, HSU)
- Read/write Mifare Classic Card
- Works with [Don's NDEF Library](https://github.com/don/NDEF)
- Communicate with android 4.0+([Lists of devices supported](https://github.com/Seeed-Studio/PN532/wiki/List-of-devices-supported))
- Support [mbed platform](https://os.mbed.com/teams/Seeed/code/PN532/)
- Card emulation (NFC Type 4 tag)

## To Do

- To support more than one INFO PDU of P2P communication
- To read/write NFC Type 4 tag

## Getting Started

### Using Arduino IDE

1. Download [zip file](https://github.com/Seeed-Studio/PN532/archive/refs/heads/arduino.zip)， extract it into Arduino's libraries and rename it to PN532-Arduino.
2. Download [Don's NDEF library](https://github.com/don/NDEF/archive/refs/heads/master.zip)， extract it into Arduino's libraries and rename it to NDEF.
3. Add the `NFC_INTERFACE_<interface>` build flag to your build system or define it in your code using `#define NFC_INTERFACE_<interface>` like

   ```cpp
   #define NFC_INTERFACE_I2C
   ```

4. Follow the examples of the two libraries.

### PlatformIO library

Add `https://github.com/Seeed-Studio/PN532.git` to your `lib_deps` variable in `platformio.ini` like so. This library will automatically include Don's NDEF library as well.

```
lib_deps =
    https://github.com/Seeed-Studio/PN532.git
```

> ⚠️ Besides using the correct `PN532_<interface>.h` include file, you have to add `-DNFC_INTERFACE_<interface>` to `build_flags` to select what interface you want to use. This is done to prevent requiring unnecessary dependencies on e.g. `SoftwareSerial` or `SPI` when you are not using those interfaces.

```
build_flags =
    -DNFC_INTERFACE_HSU
```

### Git way for Linux/Mac (recommended)

1.  Get PN532 library and NDEF library

        cd {Arduino}\libraries
        git clone --recursive https://github.com/Seeed-Studio/PN532.git NFC
        git clone --recursive https://github.com/don/NDEF.git NDEF
        ln -s NFC/PN532 ./
        ln -s NDEF/NDEF ./

1.  Add the `NFC_INTERFACE_<interface>` build flag to your build system or define it in your code using `#define NFC_INTERFACE_<interface>` like

    ```cpp
    #define NFC_INTERFACE_I2C
    ```

1.  Follow the examples of the two libraries

## Interfaces

This library offers four ways to interface with the PN532 board:

- `HSU` (High Speed Uart)
- `I2C`
- `SPI`
- `SWHSU` (Software-based High Speed Uart)

Read the section for the interface you want to use.

> Make sure to add the `PN532_<interface>.h` include file and the `NFC_INTERFACE_<interface>` define to your code like the example below:

```cpp
#define NFC_INTERFACE_HSU

#include <PN532_HSU.h>
#include <PN532.h>
```

## HSU Interface

HSU is short for High Speed Uart. HSU interface needs only 4 wires to connect PN532 with Arduino, [Sensor Shield](http://goo.gl/i0EQgd) can make it more easier. For some Arduino boards like [Leonardo][leonardo], [DUE][due], [Mega][mega] ect, there are more than one `Serial` on these boards, so we can use this additional Serial to control PN532, HSU uses 115200 baud rate.

To use the `Serial1` control PN532, refer to the code below.

```c++
#define NFC_INTERFACE_HSU

#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial1);
PN532 nfc(pn532hsu);

void setup(void)
{
	nfc.begin();
	//...
}
```

If your Arduino has only one serial interface and you want to keep it for control or debugging with the Serial Monitor, you can use the [`SoftwareSerial`][softwareserial] library to control the PN532 by emulating a serial interface. Include `PN532_SWHSU.h` instead of `PN532_HSU.h`:

```c++
#define NFC_INTERFACE_SWHSU

#include <SoftwareSerial.h>
#include <PN532_SWHSU.h>
#include <PN532.h>

SoftwareSerial SWSerial( 10, 11 ); // RX, TX

PN532_SWHSU pn532swhsu( SWSerial );
PN532 nfc( pn532swhsu );

void setup(void)
{
	nfc.begin();
	//...
}
```

## Attribution

This library is based on [Adafruit_NFCShield_I2C](https://github.com/adafruit/Adafruit_NFCShield_I2C).
[Seeed Studio](hhttps://www.seeedstudio.com/) rewrite the library to make it easy to support different interfaces and platforms.
[@Don](https://github.com/don) writes the [NDEF library](https://github.com/don/NDEF) to make it more easy to use.
[@JiapengLi](https://github.com/JiapengLi) adds HSU interface.
[@awieser](https://github.com/awieser) adds card emulation function.

[mega]: http://arduino.cc/en/Main/arduinoBoardMega
[due]: http://arduino.cc/en/Main/arduinoBoardDue
[leonardo]: http://arduino.cc/en/Main/arduinoBoardLeonardo
[softwareserial]: https://www.arduino.cc/en/Reference/softwareSerial
