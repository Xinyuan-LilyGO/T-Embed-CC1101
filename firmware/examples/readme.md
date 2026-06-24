# [中文](./readme.md) | English

# `firmware/examples/` Usage Guide

The `firmware/examples/` directory stores example firmware generated from `examples/`.

Each example has its own output folder. A typical structure looks like this:

```text
firmware/examples/
  readme.md
  readme_en.md
  test_ir_send_recv/
    bin/
      test_ir_send_recv.bin
      bootloader.bin
      partitions.bin
      firmware.bin
      flash_args.txt
      esptool_command.txt
      esptool_merged_command.txt
      build_info.json
```

## 1. Which file should I use

In most cases, you should use the example-named merged `.bin` file inside the `bin/` folder, for example:

- `test_ir_send_recv.bin`
- `test_cc1101_send_recv.bin`
- `test_battery_bq27220_bq25896.bin`

This is the fully merged firmware image. You only need to flash it at address `0x0`.

## 2. What each file in `bin/` is for

- `<example>.bin`
  Fully merged firmware image. This is the recommended file to flash.

- `bootloader.bin`
  Bootloader image.

- `partitions.bin`
  Partition table image.

- `firmware.bin`
  Main application firmware for the example.

- `flash_args.txt`
  Lists the flash offsets used when flashing separate files.

- `esptool_command.txt`
  Reference command for flashing `bootloader.bin + partitions.bin + firmware.bin` separately.

- `esptool_merged_command.txt`
  Reference command for flashing the merged `<example>.bin` image.

- `build_info.json`
  Build metadata such as example name, target environment, chip, and flash settings.

- `firmware.elf`
  Debug file used for crash analysis or symbol lookup. It is not flashed directly.

- `firmware.map`
  Linker map file used for code size and memory layout analysis. It is not flashed directly.

## 3. How to flash

### Method 1: Flash the merged image, recommended

Open the example `bin/` folder and run the command recorded in `esptool_merged_command.txt`.

Example:

```powershell
cd firmware\examples\test_ir_send_recv\bin
python ..\..\..\..\script\esptool.py --chip esp32s3 --baud 921600 write_flash -z 0x0 test_ir_send_recv.bin
```

If `esptool` is already installed on your system, you can also use `esptool.py` directly or `python -m esptool`.

### Method 2: Flash separate files

If you do not want to use the merged image, you can flash the files individually with fixed addresses:

```powershell
cd firmware\examples\test_ir_send_recv\bin
python ..\..\..\..\script\esptool.py --chip esp32s3 --baud 921600 write_flash -z 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

The same offsets are also listed in `flash_args.txt` and `esptool_command.txt`.

## 4. How to switch to another example

To test another example, simply flash the `.bin` file from that example's folder.

For example:

- To test the IR example, flash `firmware/examples/test_ir_send_recv/bin/test_ir_send_recv.bin`
- To test the CC1101 example, flash `firmware/examples/test_cc1101_send_recv/bin/test_cc1101_send_recv.bin`
- To test the battery example, flash `firmware/examples/test_battery_bq27220_bq25896/bin/test_battery_bq27220_bq25896.bin`

## 5. Notes

- Make sure the device is in a downloadable state and the USB/serial connection is working before flashing.
- These firmware files are generated for this project's hardware configuration and should not be assumed to work on other boards.
- Some test examples are intended to be used together with serial logs, such as IR, wireless, battery, or NFC-related tests.
- If you rebuild an example, the files under `firmware/examples/<example>/bin/` will be overwritten by the new output.

## 6. How to regenerate these files

Use the build script from the project root:

```powershell
python script\build_example_firmware.py --list
python script\build_example_firmware.py test_ir_send_recv
python script\build_example_firmware.py test_ir_send_recv test_cc1101_send_recv
python script\build_example_firmware.py --all
```

After the build completes, the generated files will be placed in `firmware/examples/<example>/bin/`.
