# Wi-Fi 信号诊断示例

[English](./README.md) | [简体中文](./README_CN.md)

这个示例是给外置天线版 T-Embed CC1101 Plus 使用的 Wi-Fi 专用诊断程序。这里故意关闭了 BLE，只做 Wi-Fi 信号测试，同时避开 Wi-Fi / BLE 共存导致的重启问题。

![](./run.jpg)

## 功能

- 启动一个 2.4 GHz Wi-Fi AP，名称是 `T-Embed-RF-Test`，密码是 `12345678`，方便手机直接测设备发射出来的 Wi-Fi 信号。
- 将 Wi-Fi 最大发射功率设置为 `19.5 dBm`，并关闭 Wi-Fi 省电。
- 每 10 秒扫描一次周围路由器，并在屏幕和串口输出 `dBm` 单位的 RSSI。
- 屏幕上会显示附近最强的几个 Wi-Fi、当前信道和 AP 连接客户端数量。

## PlatformIO 使用步骤

1. 先装好外置天线，再给设备上电。
2. 用 USB 连接设备和电脑。
3. 打开仓库根目录下的 `platformio.ini`。
4. 确认下面这行是启用状态：

   ```ini
   src_dir = examples/wifi_ble_signal_test
   ```

5. 保存 `platformio.ini`。
6. 在 VSCode / PlatformIO 中选择正确的串口。
7. 点击 PlatformIO 的 Upload 按钮烧录。
8. 打开串口监视器，波特率设为 `115200`。

## 测试步骤

1. 在手机上安装一个 Wi-Fi 分析类 App。
2. 烧录后，设备屏幕应显示 `WiFi RF Signal Test`。
3. 用手机搜索 `T-Embed-RF-Test`。
4. 分别记录 `0.5 m`、`1.5 m`、`3 m` 距离下的 RSSI。
5. 把设备放到路由器旁边，记录屏幕或串口里显示的路由器 RSSI。
6. 把截图和串口日志发回。

## RSSI 参考

- `-30 ~ -50 dBm`：很强
- `-50 ~ -67 dBm`：正常
- `-67 ~ -75 dBm`：偏弱，但通常还能使用
- `< -75 dBm`：信号较差

如果用了这个示例后，设备在近距离下 Wi-Fi 依然明显偏弱，优先检查外置天线、IPEX / U.FL 座子、射频线、天线焊接和整机装配路径。
