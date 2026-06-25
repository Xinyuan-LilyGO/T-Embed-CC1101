# [中文](./README_CN.md) | English

# `firmware/examples/` Usage Guide

The `firmware/examples/` directory stores flashable example firmware generated from `examples/`.

Each build now exports only one merged `.bin` file directly into this directory. A typical structure looks like this:

```text
firmware/examples/
  README.md
  README_CN.md
  factory.bin
  test_ir_send_recv.bin
  test_cc1101_send_recv.bin
```

## 1. Which file should I use

Use the example-named `.bin` file directly, for example:

- `factory.bin`
- `test_ir_send_recv.bin`
- `test_cc1101_send_recv.bin`

Each file is a fully merged firmware image and should be flashed at address `0x0`.

## 2. How to flash

Example:

```powershell
python script\esptool.py --chip esp32s3 --baud 921600 write_flash -z 0x0 firmware\examples\test_ir_send_recv.bin
```

If `esptool` is already installed on your system, you can also use `esptool.py` directly or `python -m esptool`.

## 3. How to switch to another example

To test another example, flash that example's `.bin` file from `firmware/examples/`.

Examples:

- IR example: `firmware/examples/test_ir_send_recv.bin`
- CC1101 example: `firmware/examples/test_cc1101_send_recv.bin`
- Battery example: `firmware/examples/test_battery_bq27220_bq25896.bin`

## 4. Notes

- Make sure the device is in a downloadable state and the USB/serial connection is working before flashing.
- These firmware files are generated for this project's hardware configuration and should not be assumed to work on other boards.
- Some test examples are intended to be used together with serial logs, such as IR, wireless, battery, or NFC-related tests.
- Rebuilding the same example overwrites `firmware/examples/<example>.bin`.
- If an example path contains subfolders, the output filename uses `__` in place of `/`.
  For example, `extend_nrf2401/recv` becomes `extend_nrf2401__recv.bin`.

## 5. How to regenerate these files

Use the build script from the project root:

```powershell
python script\build_example_firmware.py --list
python script\build_example_firmware.py test_ir_send_recv
python script\build_example_firmware.py test_ir_send_recv test_cc1101_send_recv
python script\build_example_firmware.py --all
```

After the build completes, the generated `.bin` files will be placed in `firmware/examples/`.
