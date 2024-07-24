<h1 align = "center">🏆T-Embed-PN532🏆</h1>

<p> 
  <a href="https://platformio.org/"> <img src="./hardware/image/PlatformIO_badge.png" height="20px"> </a>
  <a href="https://www.arduino.cc/en/software"> <img src="./hardware/image/Arduino_badge.png" height="20px"></a>
  <a href=""> <img src="https://img.shields.io/badge/Liiygo-T_Embed_PN532-blue" height="20px"></a>
  <a href=""> <img src="https://img.shields.io/badge/language-c++-brightgreen" height="20px"></a>
</p>

* [切换到中文版](./README_CN.md)


## :one:Product 🎁

| Version |   v1.0 24-03-15   |
|:-------:|:-----------------:|
| Module  | ESP32-S3-WROOM-1U |
|  Flash  |       16MB        |
|  PSRAM  |        8MB        |
| Screen  |      320x170      |

### Wireless transceiver：

T-Embed-PN532 has a built-in Sub-GHz module based on a `CC1101` transceiver and a radio antenna (the maximum range is 50 meters). Both the CC1101 chip and the antenna are designed to operate at frequencies in the 300-348 MHz, 387-464 MHz, and 779-928 MHz bands.

The Sub-GHz application supports external radio modules based on the CC1101 transceiver.

### Folder structure:
~~~
├─boards  : Some information about the board for the platformio.ini configuration project;
├─data    : Picture resources used by the program;
├─example : Some examples;
├─firmare : `factory` compiled firmware;
├─hardware: Schematic diagram of the board, chip data;
├─lib     : Libraries used in the project;
~~~

## :two: Example 🎯

Some examples are provided under the Project example folder, which can run on PlatformIO (PIO) and Arduion, but I prefer to use PIO because these examples are developed on PIO, **All examples can run on PIO**, However, you may encounter compilation errors on Arduion, but don't worry, the author will step up to test the compilation of Arduion environment.

**Examples of compilation in Arduion environment:**


- ✅ bq25896_test : Battery management test. Print the battery status in the serial port.
- ✅ cc1101_recv_irq ：Wireless reception test, display received messages in the serial port.
- ✅ cc1101_send_irq ：Wireless sending test, display sent messages in the serial port.
- ✅ display_test ：Screen display test;
- ✅ encode_test ：encoder tester
- ❌ factory ：Factory programs can currently only be compiled and downloaded on PlatformIO;
- ✅ infrared_recv_test: Infrared test
- ✅ infrared_send_test: Infrared test
- ✅ lvgl_test ：lvgl benchmark and stress testing;
- ✅ pn532_test ：NFC test, display the IC card information in the serial port.
- ✅ tf_card_test ：SD card test, the file name is displayed in the serial port.
- ✅ record_test : Record 15 seconds of audio and save it to your SD card.
- ✅ voice_test : Speaker test, read audio from SD card.
- ✅ ws2812_test ：LED light test;


## :three: PlatformIO Quick Start

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [Python](https://www.python.org/), and clone or download the project;
2. Search for the `PlatformIO` plugin in the `VisualStudioCode` extension and install it;
3. After the installation is complete, you need to restart `VisualStudioCode`
4. After opening this project, PlatformIO will automatically download the required tripartite libraries and dependencies, the first time this process is relatively long, please wait patiently;
5. After all the dependencies are installed, you can open the `platformio.ini` configuration file, uncomment in `example` to select a routine, and then press `ctrl+s` to save the `.ini` configuration file;
6. Click :ballot_box_with_check: under VScode to compile the project, then plug in USB and select COM under VScode;
7. Finally, click the :arrow_right:  button to download the program to Flash;

## :four: Arduion Quick Start

:exclamation: :exclamation: :exclamation: <font color="red"> **Notice:**</font>
PlatformIO is more recommended because it may not be compiled with Arduion. You can refer to **2️⃣-Example** to see which examples are successfully compiled in Arduion environment.

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install the esp32 toolkit, open the Arduion IDE, click on `File->Perferences`, Then `https://espressif.github.io/arduino-esp32/package_esp32_index.json ` paste to the position of the diagram below, then click :ok:, waiting for the toolkit download is complete;

![alt text](./hardware/image/image.png)

3. Copy all files under `this project/lib/` and paste them into the Arduion library path (generally `C:\Users\YourName\Documents\Arduino\libraries`);
4. Open the Arduion IDE and click `File->Open` in the upper left corner to open an example in `this project/example/xxx/xxx.ino` under this item;
5. Then configure Arduion. After the configuration is completed in the following way, you can click the button in the upper left corner of Arduion to compile and download;

![](./hardware/image/Arduion_config_en.png)

## :five: Other
To be added...
