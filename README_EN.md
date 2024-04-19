<h1 align = "center">🌟T-Embed-PN532🌟</h1>

* [切换到中文版](./README_CN.md)

## :one:Product

| Version |   v1.0 24-03-15   |
|:-------:|:-----------------:|
| Module  | ESP32-S3-WROOM-1U |
|  Flash  |       16MB        |
|  PSRAM  |        8MB        |
| Screen  |      320x170      |


## :two: Example

~~~
example
├─audio_test
├─bq25896_test
├─cc1101_recv_irq
├─cc1101_recv_test
├─cc1101_send_irq
├─cc1101_send_test
├─display_test
├─encode_test
├─factory
├─infrared_test
├─lvgl_test
├─pn532_test
├─tf_card_test
└─ws2812_test
~~~


## :three: PlatformIO 快速开始

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [Python](https://www.python.org/)
2. Search for the `PlatformIO` plugin in the `VisualStudioCode` extension and install it.
3. After the installation is complete, you need to restart `VisualStudioCode`
4. After restarting `VisualStudioCode`, select `File` in the upper left corner of `VisualStudioCode` -> `Open Folder` -> select the `T-Display-S3-Long` directory
5. Wait for the installation of third-party dependent libraries to complete
6. Click on the `platformio.ini` file, and in the `platformio` column
7. Uncomment one of the lines `src_dir = xxxx` to make sure only one line works
8. Click the (✔) symbol in the lower left corner to compile
9. Connect the board to the computer USB
10. Click (→) to upload firmware
11. Click (plug symbol) to monitor serial output
12. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below

## :four: Arduion 快速开始

* It is recommended to use platformio without cumbersome steps

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Download or clone project `T-Display-S3-Long`
3. Copy all the files in `T-Display-S3-Long/lib` and paste them into Arduion library folder(e.g. C:\Users\YourName\Documents\Arduino\libraries).
4. Open Arduino IDE, select the `examples\xxx` example of project `T-Display-S3-Long` throught `"File->Open"`.
5. Configuration of board is as follows:
