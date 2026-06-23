# T-Embed-CC1101 引脚映射（V1.0 / 2026-06-23）
> 本文根据 `hardware/T-Embed-CC1101 V1.0 24-07-29.pdf`、`README.md`、`README_CN.md`、`examples/factory_test/utilities.h`、`docs/README_img/image-2.png` 整理。
> 仓库里的部分宏名沿用了旧命名：`BOARD_LORA_*` 实际对应板载 `CC1101`，`BOARD_PN532_*` 对应板载 `PN532`。
> 需要先说明：当前仓库里有两处定义不完全一致。其一是个别示例把 I2C `SDA/SCL` 写反；其二是 LCD `RST` 在原理图与软件配置之间存在歧义。文中会单独标注。

## 1. 主控与公共总线

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| 主 I2C SDA | `BOARD_I2C_SDA` | GPIO8 | 原理图 Page 1/2/3/4，`docs/README_img/image-2.png`，`examples/factory_test/utilities.h` | 板载 PN532、BQ27220、BQ25896 与外接 I2C 口共用；`examples/cc1101_recv/utilities.h` 把 SDA/SCL 对调了，建议以本表为准 |
| 主 I2C SCL | `BOARD_I2C_SCL` | GPIO18 | 原理图 Page 1/2/3/4，`docs/README_img/image-2.png`，`examples/factory_test/utilities.h` | 同上 |
| 主 SPI SCK | `BOARD_SPI_SCK` | GPIO11 | 原理图 Page 2/3/4，`examples/factory_test/utilities.h` | LCD、TF、PN532、CC1101、外接 nRF24 共用 |
| 主 SPI MOSI | `BOARD_SPI_MOSI` | GPIO9 | 原理图 Page 2/3/4，`examples/factory_test/utilities.h` | 同上 |
| 主 SPI MISO | `BOARD_SPI_MISO` | GPIO10 | 原理图 Page 2/3/4，`examples/factory_test/utilities.h` | 同上 |
| 开关 3.3 V 使能 | `BOARD_PWR_EN` | GPIO15 | 原理图 Page 1/2，`README.md`，`examples/factory_test/utilities.h` | 控制开关电源域 `VCC3V3`；仓库示例注释明确写到会给 CC1101 与 WS2812/LED 上电 |
| USB D- | `BOARD_USB_DM` | GPIO19 | 原理图 Page 1/2 | Type-C 直连 ESP32-S3 USB |
| USB D+ | `BOARD_USB_DP` | GPIO20 | 原理图 Page 1/2 | Type-C 直连 ESP32-S3 USB |
| 串口调试 TX | `BOARD_UART0_TXD` | `U0TXD` / GPIO43 | 原理图 Page 2，`docs/README_img/image-2.png` | 外接接口图片标成 `TXD: 43`；扩展 nRF24 示例中也把这脚复用为 `CE` |
| 串口调试 RX | `BOARD_UART0_RXD` | `U0RXD` / GPIO44 | 原理图 Page 2，`docs/README_img/image-2.png` | 外接接口图片标成 `RXD: 44`；扩展 nRF24 示例中也把这脚复用为 `CS` |

## 2. 按键与编码器

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| 用户按键 | `BOARD_USER_KEY` | GPIO6 | 原理图 Page 2，`README.md`，`examples/factory_test/utilities.h` | 独立按键 |
| 编码器 A 相 | `ENCODER_INA` | GPIO4 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 旋钮编码器 A 相 |
| 编码器 B 相 | `ENCODER_INB` | GPIO5 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 旋钮编码器 B 相 |
| 编码器按压 / BOOT | `ENCODER_KEY` | GPIO0 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 既是编码器按键又是启动脚，低电平会影响下载/启动模式 |
| 复位按键 | `BOARD_RST_EN` | `EN` | 原理图 Page 2 | 复位按键，不是普通 GPIO |

## 3. 扩展接口与外接模块

| 接口/功能 | 建议名 | 引脚/网络 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| 外接 UART TX | `BOARD_UART0_TXD` | GPIO43 | 原理图 Page 2，`docs/README_img/image-2.png` | 与 3V3 / GND 一起引出 |
| 外接 UART RX | `BOARD_UART0_RXD` | GPIO44 | 原理图 Page 2，`docs/README_img/image-2.png` | 与 3V3 / GND 一起引出 |
| 外接 I2C SDA | `BOARD_I2C_SDA` | GPIO8 | 原理图 Page 2，`docs/README_img/image-2.png` | 与 3V3 / GND 一起引出 |
| 外接 I2C SCL | `BOARD_I2C_SCL` | GPIO18 | 原理图 Page 2，`docs/README_img/image-2.png` | 与 3V3 / GND 一起引出 |
| 外接 nRF24 CE | `BOARD_NRF24_CE` | GPIO43 | `examples/factory_test/utilities.h`，`examples/extend_nrf2401/*` | 只在外接模块场景使用；与外接 UART `TXD` 复用 |
| 外接 nRF24 CS | `BOARD_NRF24_CS` | GPIO44 | `examples/factory_test/utilities.h`，`examples/extend_nrf2401/*` | 只在外接模块场景使用；与外接 UART `RXD` 复用 |
| 外接 nRF24 SPI | `BOARD_NRF24_SCK/MOSI/MISO` | GPIO11 / 9 / 10 | `examples/factory_test/utilities.h`，`examples/extend_nrf2401/*` | 共享主 SPI |
| 外接 nRF24 IRQ | `BOARD_NRF24_IRQ` | `-1` | `examples/factory_test/utilities.h` | 当前仓库未给外接 nRF24 分配独立中断脚 |

## 4. TF 卡

| 功能 | 建议名 | GPIO | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| SD CS | `BOARD_SD_CS` | GPIO13 | 原理图 Page 2，`examples/factory_test/utilities.h` | 共享主 SPI |
| SD SCK | `BOARD_SD_SCK` | GPIO11 | 原理图 Page 2，`examples/factory_test/utilities.h` | 共享主 SPI |
| SD MOSI | `BOARD_SD_MOSI` | GPIO9 | 原理图 Page 2，`examples/factory_test/utilities.h` | 共享主 SPI |
| SD MISO | `BOARD_SD_MISO` | GPIO10 | 原理图 Page 2，`examples/factory_test/utilities.h` | 共享主 SPI |

## 5. 音频与麦克风

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| I2S BCLK | `BOARD_VOICE_BCLK` | GPIO46 | 原理图 Page 2，`examples/factory_test/utilities.h` | ESP32-S3 -> MAX98357A |
| I2S LRCLK | `BOARD_VOICE_LRCLK` | GPIO40 | 原理图 Page 2，`examples/factory_test/utilities.h` | ESP32-S3 -> MAX98357A |
| I2S DIN | `BOARD_VOICE_DIN` | GPIO7 | 原理图 Page 2，`examples/factory_test/utilities.h` | ESP32-S3 -> MAX98357A |
| MIC DATA | `BOARD_MIC_DATA` | GPIO42 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 数字麦克风数据输入 |
| MIC CLK | `BOARD_MIC_CLK` | GPIO39 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 数字麦克风时钟 |
| 扬声器接口 | `BOARD_SPK_OUT` | `J4` | 原理图 Page 2 | 2 Pin 扬声器座，不是 GPIO |

## 6. PN532（NFC）

I2C 地址：`0x24`

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| I2C SDA | `BOARD_PN532_SDA` | GPIO8 | 原理图 Page 1/2/3/4，`examples/factory_test/utilities.h` | 与主 I2C 共用 |
| I2C SCL | `BOARD_PN532_SCL` | GPIO18 | 原理图 Page 1/2/3/4，`examples/factory_test/utilities.h` | 与主 I2C 共用 |
| RST / Power-Down | `BOARD_PN532_RF_REST` | GPIO45 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名是 `RF_RST`，芯片脚名是 `RSTPD_N` |
| IRQ | `BOARD_PN532_IRQ` | GPIO17 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `IRQ` |

## 7. CC1101（仓库中沿用 `LORA` 命名）

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| SPI SCK | `BOARD_LORA_SCK` | GPIO11 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 共享主 SPI |
| SPI MOSI | `BOARD_LORA_MOSI` | GPIO9 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 共享主 SPI |
| SPI MISO | `BOARD_LORA_MISO` | GPIO10 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 共享主 SPI |
| CS | `BOARD_LORA_CS` | GPIO12 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `HPD_CS` |
| GDO2 | `BOARD_LORA_IO2` | GPIO38 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `LORA_IO2` |
| GDO0 | `BOARD_LORA_IO0` | GPIO3 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `LORA_IO0` |
| 射频开关 1 | `BOARD_LORA_SW1` | GPIO47 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `SW_1` |
| 射频开关 0 | `BOARD_LORA_SW0` | GPIO48 | 原理图 Page 2/3，`examples/factory_test/utilities.h` | 原理图网名 `SW_0` |
| 供电域 | `BOARD_LORA_VDD` | `VCC3V3` | 原理图 Page 1/3 | 受 `BOARD_PWR_EN` 控制 |

CC1101 频段切换在仓库示例里约定如下：

- `SW1=1, SW0=0` -> `315 MHz`
- `SW1=1, SW0=1` -> `434 MHz`
- `SW1=0, SW0=1` -> `868 / 915 MHz`

以上映射来自 `examples/cc1101_send/cc1101_send.ino` 与 `examples/factory_test/ui.cpp`。

## 8. LCD 与背光

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| LCD CS | `DISPLAY_CS` | GPIO41 | 原理图 Page 2/4，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 原理图网名 `LCD_CS` |
| LCD DC | `DISPLAY_DC` | GPIO16 | 原理图 Page 2，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 原理图 Page 2 标成 `LCD_DC` |
| 背光使能 | `DISPLAY_BL` | GPIO21 | 原理图 Page 2/4，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 原理图网名 `BL_EN`，驱动芯片为 `AW9364` |
| SPI SCK | `DISPLAY_SCLK` | GPIO11 | 原理图 Page 2/4，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 共享主 SPI |
| SPI MOSI | `DISPLAY_MOSI` | GPIO9 | 原理图 Page 2/4，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 共享主 SPI |
| SPI MISO | `DISPLAY_MISO` | GPIO10 | 原理图 Page 2/4，`lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h` | 共享主 SPI |
| LCD RST | `DISPLAY_RST` | `-1`（软件配置） | `lib/TFT_eSPI/User_Setups/Setup214_LilyGo_T_Embed_PN532.h`，原理图 Page 4 | 软件里没有单独控制 RST；原理图第 4 页文字提取又出现了 `LCD_RES -> IO40`，但仓库同时把 `IO40` 用作音频 `LRCLK`，这里视为“存在歧义，当前仓库未启用独立 LCD RST” |

补充说明：原理图第 4 页还出现 `T_RST -> IO46`、`T_INT -> IO16` 这类触摸相关标注，但当前仓库没有触摸驱动配置，而且 `IO46` / `IO16` 在软件里分别已经被音频 `BCLK` / `LCD_DC` 使用。后续如果要接入触摸，建议先重新核对实际屏模组与 FFC 引脚定义。

## 9. RGB 与红外

| 功能 | 建议名 | GPIO/映射 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| WS2812 数据 | `WS2812_DATA_PIN` | GPIO14 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 板上共有 `8` 颗 WS2812 |
| 红外发射使能 / PWM | `BOARD_IR_EN` | GPIO2 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 原理图网名 `IR_EN` |
| 红外接收 | `BOARD_IR_RX` | GPIO1 | 原理图 Page 2/4，`examples/factory_test/utilities.h` | 原理图网名 `IR_RX` |

## 10. I2C 地址表

| 设备 | 地址 | 来源 | 备注 |
| --- | --- | --- | --- |
| PN532 | `0x24` | `README.md`，`examples/factory_test/utilities.h` | NFC |
| BQ27220 | `0x55` | 原理图 Page 1，`README.md`，`examples/factory_test/utilities.h` | 电量计 |
| BQ25896 | `0x6B` | 原理图 Page 1，`README.md`，`examples/factory_test/utilities.h` | 充电管理 |

## 11. 命名与风险提示

- `BOARD_LORA_*` 这组宏在本板上实际服务的是 `CC1101`，如果后续做公共板级抽象，建议改成 `BOARD_CC1101_*` 一类更直观的命名。
- `BOARD_PWR_EN` 控制的是一个开关 3.3 V 电源域，不是“整机主电源键”；仓库里多个示例都把它当成 `CC1101/WS2812` 的上电开关。
- `examples/cc1101_recv/utilities.h` 中的 `BOARD_SDA_PIN` / `BOARD_SCL_PIN` 与仓库主定义相反，写新代码时建议统一使用 `GPIO8 = SDA`、`GPIO18 = SCL`。
- `GPIO43/44` 既被当作外接串口 `TXD/RXD`，也在外接 `nRF24L01` 示例中被复用成 `CE/CS`。如果你要同时接串口和外设，需要先明确复用策略。
- `Setup214_LilyGo_T_Embed_PN532.h` 这个文件名虽然带 `PN532`，但在当前仓库里承担的是 `T-Embed-CC1101` 的 LCD 配置，不要被文件名误导。
- 当前仓库的引脚定义散落在多个 `utilities.h` 和 README 片段里；如果后续继续扩展项目，建议收敛到单一 `board_pins.h` 或 `board_config.h`。
