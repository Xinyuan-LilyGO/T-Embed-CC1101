[English](./README.md) | [简体中文](./README_CN.md)

# nRF24 Send / Recv Test

`nrf24_send_recv_test` is an nRF24L01 send/receive test example for the `LilyGo T-Embed PN532`.

This example provides:

- An on-screen UI for channel, mode, last received text, and TX/RX counters
- Two working modes: `RX` and `TX BURST`
- Serial commands for channel, data rate, output power, ACK, and TX prefix
- WS2812 status indication
  - `RX` = green
  - `TX` = blue

## Hardware

The example uses the board definitions in `examples/utilities.h`.

### nRF24L01 pin mapping

| Signal | GPIO |
| --- | --- |
| `CS` | `44` |
| `CE` | `43` |
| `SCK` | `11` |
| `MOSI` | `9` |
| `MISO` | `10` |
| `IRQ` | not connected |

Notes:

- This example uses polling and does not require the nRF24 `IRQ` pin.
- The nRF24 module shares the SPI bus with the display. Bus switching is handled in the example.

## Default Settings

| Item | Value |
| --- | --- |
| Mode | `RX` |
| Channel | `76` |
| Frequency | `2476 MHz` |
| Data rate | `1000 kbps` |
| Output power | `0 dBm` |
| Auto ACK | `ON` |
| Address width | `5 bytes` |
| Pipe address | `01 23 45 67 89` |
| TX prefix | `T-Embed nRF24` |
| TX interval | `1000 ms` |

Frequency and channel relationship:

```text
Frequency(MHz) = 2400 + Channel
```

## Build And Flash

The project [platformio.ini](../../platformio.ini) is already configured to build this example:

```ini
src_dir = examples/nrf24_send_recv_test
```

Common commands:

```bash
pio run -e T_Embed_CC1101
pio run -e T_Embed_CC1101 -t upload
pio device monitor -b 115200
```

## On-Screen UI

The display shows:

- Current channel
- Current mode: `RX` or `TX BURST`
- Last received text
- RX length and RX count
- Current data rate / output power / ACK state

## Controls

### Buttons

- `USR`: cycle through preset channels `0 -> 40 -> 76 -> 125`
- `ENC`: switch `RX <-> TX BURST`

### Serial Commands

Serial baud rate: `115200`

Available commands:

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

Command summary:

- `help`: show help text
- `status`: print current radio settings
- `rx`: switch to receive mode
- `tx`: switch to periodic burst transmit mode
- `send <text>`: send one packet immediately
- `ch <0-125>`: set RF channel
- `rate 250|1000|2000`: set air data rate in `kbps`
- `power -18|-12|-6|0`: set TX output power
- `ack on|off`: enable or disable auto ACK
- `prefix <text>`: update the `TX BURST` text prefix

## ACK Notes

If the sender prints:

```text
[nRF24] TX failed: ACK not received.
```

usually the receiver did not return an auto-acknowledgement. Check:

- Both devices are on the same `channel`
- Both devices use the same `data rate`
- Both devices use the same `pipe address`
- The receiver is currently in `RX` mode
- Both sides use matching `Auto ACK` settings

If you only want to verify one-way reception first, set both devices to:

```text
ack off
```

## Typical Test Flow

1. Prepare two devices and flash this example to both of them.
2. Keep device A in `RX`.
3. Switch device B to `TX BURST`, or send `send hello` from the serial console.
4. Observe `Last RX` on device A and check the serial log.
5. To verify auto ACK, keep device A in `RX` and make sure both devices use `ack on`.

## File

Main sketch:

- [nrf24_send_recv_test.ino](./nrf24_send_recv_test.ino)
