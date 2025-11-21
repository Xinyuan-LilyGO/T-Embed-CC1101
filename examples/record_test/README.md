| Supported Targets | ESP32 | ESP32-S3 |
| ----------------- | ----- | -------- |

# I2S Digital Microphone Recording Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

In this example, we record a sample audio file captured from the digital MEMS microphone on the I2S peripheral using PDM data format.

The audio is recorded into the SDCard using WAVE file format.

| Audio Setting | Value |
|:---:|:---:|
| Sample Rate |44100 Hz|
| Bits per Sample |16 bits|

## How to Use Example

### Hardware Required

* A development board with ESP32 or ESP32S3 SoC (e.g., ESP32-DevKitC, ESP-WROVER-KIT, etc.)
* A USB cable for power supply and programming
* A digital microphone (SPK0838HT4H PDM output was used in this example)

The digital PDM microphone is connected on the I2S interface `I2S_NUM_0`.

The default GPIO configuration is the following:

|Mic        | GPIO   |
|:---------:|:------:|
| PDM Clock | GPIO39 |
| PDM Data  | GPIO42 |

The SDCard is connected using SPI peripheral.

| SPI  | SDCard |  GPIO  |
|:----:|:------:|:------:|
| MISO | DAT0   | GPIO10 |
| MOSI | CMD    | GPIO9  |
| SCLK | CLK    | GPIO11 |
| CS   | CD     | GPIO13 |


## Example Output


Open the serial port and start recording for 15 seconds. You can use `i2s_set_clk` to change the Bits per Sample and the Sample Rate. The output log can be seen below:

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x4202ed26
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x44c
load:0x403c9700,len:0xbd8
load:0x403cc700,len:0x2a80
entry 0x403c98d0
[   105][I][esp32-hal-psram.c:96] psramInit(): PSRAM enabled
SD Card Type: SDHC
SD Card Size: 30436MB
Starting recording for 15 seconds!
Opening File
Recording done!
File written on SDCard

```

