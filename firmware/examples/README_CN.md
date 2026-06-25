# 中文 | [English](./README.md)

# `firmware/examples/` 使用说明

`firmware/examples/` 目录用于存放从 `examples/` 构建出来、可直接烧录的示例固件。

现在每个示例只会导出一个合并好的 `.bin` 文件，并直接放到这个目录下面。常见结构如下：

```text
firmware/examples/
  README.md
  README_CN.md
  factory.bin
  test_ir_send_recv.bin
  test_cc1101_send_recv.bin
```

## 1. 推荐使用哪个文件

直接使用和示例同名的 `.bin` 文件即可，例如：

- `factory.bin`
- `test_ir_send_recv.bin`
- `test_cc1101_send_recv.bin`

这些文件都是已经合并好的完整固件，烧录时统一写入 `0x0` 地址。

## 2. 如何烧录

例如：

```powershell
python script\esptool.py --chip esp32s3 --baud 921600 write_flash -z 0x0 firmware\examples\test_ir_send_recv.bin
```

如果你已经安装了 `esptool`，也可以直接使用系统里的 `esptool.py` 或 `python -m esptool`。

## 3. 如何切换到另一个示例

如果想测试另一个示例，只需要重新烧录 `firmware/examples/` 下对应的 `.bin` 文件。

例如：

- 红外示例：`firmware/examples/test_ir_send_recv.bin`
- CC1101 示例：`firmware/examples/test_cc1101_send_recv.bin`
- 电池示例：`firmware/examples/test_battery_bq27220_bq25896.bin`

## 4. 注意事项

- 烧录前请确认设备处于可下载状态，串口和 USB 连接正常。
- 这些固件面向当前项目的硬件配置生成，不建议直接用于其他板型。
- 某些测试示例需要配合串口查看日志，例如红外、无线、电池或 NFC 类测试。
- 重新构建同一个示例时，会覆盖 `firmware/examples/<example>.bin`。
- 如果示例路径里带有子目录，输出文件名会把 `/` 替换成 `__`。
  例如 `extend_nrf2401/recv` 会生成 `extend_nrf2401__recv.bin`。

## 5. 如何重新生成这些文件

可以使用项目根目录下的构建脚本重新生成：

```powershell
python script\build_example_firmware.py --list
python script\build_example_firmware.py test_ir_send_recv
python script\build_example_firmware.py test_ir_send_recv test_cc1101_send_recv
python script\build_example_firmware.py --all
```

生成完成后，输出的 `.bin` 文件会放到 `firmware/examples/` 目录下。
