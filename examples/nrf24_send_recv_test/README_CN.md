[English](./README.md) | [简体中文](./README_CN.md)

# nRF24 收发测试

`nrf24_send_recv_test` 是为 `LilyGo T-Embed PN532` 适配的 nRF24L01 收发测试示例。

这个示例提供了：

- 屏幕 UI，显示当前信道、工作模式、最近一次接收文本和收发统计
- `RX` 与 `TX BURST` 两种工作模式
- 通过串口命令动态修改信道、速率、发射功率、ACK 和发送前缀
- WS2812 状态提示
  - `RX` = 绿色
  - `TX` = 蓝色

## 硬件说明

示例使用 `examples/utilities.h` 中的板级定义。

### nRF24L01 引脚定义

| 信号 | GPIO |
| --- | --- |
| `CS` | `44` |
| `CE` | `43` |
| `SCK` | `11` |
| `MOSI` | `9` |
| `MISO` | `10` |
| `IRQ` | 未连接 |

说明：

- 本示例使用轮询方式检查接收状态，不依赖 nRF24 的 `IRQ` 引脚。
- nRF24 与屏幕共用 SPI 总线，示例内部已经处理了总线切换。

## 默认参数

| 项目 | 默认值 |
| --- | --- |
| 模式 | `RX` |
| 信道 | `76` |
| 频率 | `2476 MHz` |
| 速率 | `1000 kbps` |
| 发射功率 | `0 dBm` |
| Auto ACK | `ON` |
| 地址宽度 | `5 bytes` |
| Pipe 地址 | `01 23 45 67 89` |
| 发送前缀 | `T-Embed nRF24` |
| 发送周期 | `1000 ms` |

频率与信道的关系：

```text
Frequency(MHz) = 2400 + Channel
```

## 编译与烧录

项目中的 [platformio.ini](../../platformio.ini) 已经指向本示例：

```ini
src_dir = examples/nrf24_send_recv_test
```

常用命令：

```bash
pio run -e T_Embed_CC1101
pio run -e T_Embed_CC1101 -t upload
pio device monitor -b 115200
```

## 屏幕显示

屏幕上会显示：

- 当前信道
- 当前模式：`RX` 或 `TX BURST`
- 最近一次收到的文本
- 接收长度和接收计数
- 当前速率 / 发射功率 / ACK 状态

## 操作方式

### 按键

- `USR`：循环切换预设信道 `0 -> 40 -> 76 -> 125`
- `ENC`：切换 `RX <-> TX BURST`

### 串口命令

串口波特率：`115200`

支持的命令：

```text
help
status
rx
tx
send <text>
ch <0-125>
rate 250|1000|2000
power -18|-12|-6|0
ack on|off
prefix <text>
```

命令说明：

- `help`：打印帮助信息
- `status`：打印当前无线参数
- `rx`：切换到接收模式
- `tx`：切换到周期发送模式
- `send <text>`：立即发送一包文本
- `ch <0-125>`：设置 RF 信道
- `rate 250|1000|2000`：设置空口速率，单位 `kbps`
- `power -18|-12|-6|0`：设置发射功率
- `ack on|off`：打开或关闭自动应答
- `prefix <text>`：修改 `TX BURST` 的发送前缀

## ACK 说明

如果发送端日志出现：

```text
[nRF24] TX failed: ACK not received.
```

通常表示接收端没有返回自动应答。请重点检查：

- 两端是否在同一个 `channel`
- 两端是否使用相同的 `data rate`
- 两端是否使用相同的 `pipe address`
- 接收端当前是否处于 `RX` 模式
- 两端的 `Auto ACK` 设置是否一致

如果只是先验证单向接收，可以先把两端都设置成：

```text
ack off
```

## 典型测试流程

1. 准备两块设备，并都烧录本示例。
2. 让 A 设备保持在 `RX` 模式。
3. 让 B 设备切换到 `TX BURST`，或者在串口发送 `send hello`。
4. 观察 A 设备屏幕上的 `Last RX` 和串口日志。
5. 如果要验证 ACK，请保持 A 在 `RX` 模式，并确保两端都使用 `ack on`。

## 文件

主程序文件：

- [nrf24_send_recv_test.ino](./nrf24_send_recv_test.ino)
