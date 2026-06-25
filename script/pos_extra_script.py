import os
from platform import platform
import sys
import time
from SCons.Script import COMMAND_LINE_TARGETS
import esptool
import gen_esp32part
import csv
import spiffsgen

Import("env", "projenv")

def getSpiffsOffset(file):
    try:
        with open(file, 'r', newline='') as csvfile:
            cr = csv.DictReader(csvfile)
            for row in cr:
                if row.get(' SubType') is not None and row[' SubType'] == 'spiffs':
                    if row.get(' Size') is not None and row.get(' Offset') is not None:
                        return [row.get(' Offset'),row.get(' Size')]
    except:
        return None

def _parse_size(value):
    if isinstance(value, int):
        return value
    elif value.isdigit():
        return int(value)
    elif value.startswith("0x"):
        return int(value, 16)
    elif value[-1].upper() in ("K", "M"):
        base = 1024 if value[-1].upper() == "K" else 1024 * 1024
        return int(value[:-1]) * base
    return value

def _get_board_f_flash(env):
    frequency = env.subst("$BOARD_F_FLASH")
    frequency = str(frequency).replace("L", "")
    return str(int(int(frequency) / 1000000)) + "m"

def _get_board_flash_mode(env):
    memory_type = env.BoardConfig().get("build.arduino.memory_type", "qio_qspi")
    mode = env.subst("$BOARD_FLASH_MODE")
    if memory_type in ("opi_opi", "opi_qspi"):
        return "dout"
    if mode in ("qio", "qout"):
        return "dio"
    return mode

def merge_bin_files(env):
    upload_cmd = env['UPLOADERFLAGS']
    offset = env['ESP32_APP_OFFSET']
    platform = env['HOST_OS']

    boot_app0_path = ''
    bootloader_path = ''
    partitions_path = ''
    firmware_path_prefix = ''
    firmware_path = ''

    chip_type = upload_cmd[upload_cmd.index('--chip') + 1]
    flash_size = upload_cmd[upload_cmd.index('--flash_size') + 1]
    flash_mode = upload_cmd[upload_cmd.index('--flash_mode') + 1]

    if chip_type == 'esp32':
        boot_app0_path = upload_cmd[upload_cmd.index('0xe000') + 1]
        bootloader_path = upload_cmd[upload_cmd.index('0x1000') + 1]  #esp32
        partitions_path = upload_cmd[upload_cmd.index('0x8000') + 1]
        firmware_path_prefix = partitions_path.rfind('\\') + 1
        firmware_path = partitions_path[:firmware_path_prefix] + 'firmware.bin'

    elif chip_type == 'esp32s3' or chip_type == 'esp32c3':
        boot_app0_path = upload_cmd[upload_cmd.index('0xe000') + 1]
        bootloader_path = upload_cmd[upload_cmd.index('0x0000') + 1]  #esp32s3
        partitions_path = upload_cmd[upload_cmd.index('0x8000') + 1]
        firmware_path_prefix = partitions_path.rfind('\\') + 1
        firmware_path = partitions_path[:firmware_path_prefix] + 'firmware.bin'

    boot_app0_sys_perfix = boot_app0_path[0].lower()
    bootloader_sys_perfix = bootloader_path[0].lower()
    partitions_sys_perfix = partitions_path[0].lower()
    firmware_sys_perfix = firmware_path[0].lower()

    if platform == 'linux':
        boot_app0_path = '\\mnt\\' + boot_app0_sys_perfix + \
            boot_app0_path[boot_app0_path.find('\\'):]
        bootloader_path = '\\mnt\\' + bootloader_sys_perfix + \
            bootloader_path[bootloader_path.find('\\'):]
        partitions_path = '\\mnt\\' + partitions_sys_perfix + \
            partitions_path[partitions_path.find('\\'):]
        firmware_path = '\\mnt\\' + firmware_sys_perfix + \
            firmware_path[firmware_path.find('\\'):]

        boot_app0_path = boot_app0_path.replace('\\', '/')
        bootloader_path = bootloader_path.replace('\\', '/')
        partitions_path = partitions_path.replace('\\', '/')
        firmware_path = firmware_path.replace('\\', '/')

    elif platform == 'win32':
        pass
    elif platform == 'darwin':
        firmware_path_prefix = partitions_path.rfind('/') + 1
        firmware_path = partitions_path[:firmware_path_prefix] + 'firmware.bin'
        pass

    bootloader_path = os.path.normpath(bootloader_path)
    partitions_path = os.path.normpath(partitions_path)
    boot_app0_path = os.path.normpath(boot_app0_path)
    firmware_path = os.path.normpath(firmware_path)

 

    print('FIRMWARE PATH:')
    print('bootloader_path:', end='  ')
    if chip_type == 'esp32':
        print('0x1000:', end='  ')
    elif chip_type == 'esp32s3' or chip_type == 'esp32c3':
        print('0x0000:', end='  ')
    print(bootloader_path)

    print('partitions_path:', end='  ')
    print('0x8000:', end='  ')
    print(partitions_path)

    print('boot_app0_path:', end='  ')
    print('0xe000:', end='  ')
    print(boot_app0_path)

    print('firmware_path:', end='  ')
    print(offset, end='')
    print(':  ', end='')
    print(firmware_path)

    outpuDir =  os.path.abspath('.')+ os.altsep + 'build'

    if not os.path.exists(outpuDir):
        os.mkdir(outpuDir)

    outputFilename =  outpuDir + os.altsep + "firmware_{0}_{1}.bin".format(
        env['PIOENV'], time.strftime("%Y%m%d", time.localtime()))


    has_data = False
    data_offset = 0
    spiffs_data = []
    dataDir =  os.path.abspath('.')+ os.altsep + 'data'
    dataOutputBinPath = outpuDir + os.altsep +'spiffs.bin' 

    if os.path.exists(dataDir):
        has_data = True

    partitions_csv_path = outpuDir + os.altsep+ 'partitions.csv'
    gen_esp32part_cmd = []
    gen_esp32part_cmd.append('--quiet')
    gen_esp32part_cmd.append('--input')
    gen_esp32part_cmd.append(partitions_path)
    gen_esp32part_cmd.append('--output')
    gen_esp32part_cmd.append(partitions_csv_path)
 
    # print('gen_esp32part_cmd')
    # print(gen_esp32part_cmd)

    if has_data:
        data_size = 0
        gen_esp32part.main(gen_esp32part_cmd)
        spiffs_data = getSpiffsOffset(partitions_csv_path)
        if spiffs_data is not None:
            data_size = spiffs_data[1]
            data_offset = spiffs_data[0]
            if data_size != 0:
                data_size = _parse_size(data_size)
                print(data_size)
                spiffsgen_commands = []
                spiffsgen_commands.append(str(data_size))
                spiffsgen_commands.append(dataDir)
                spiffsgen_commands.append(dataOutputBinPath)
                # print('spiffsgen_commands')
                # print(spiffsgen_commands)
                spiffsgen.main(spiffsgen_commands)
        else:
            has_data = False


    flash_freq = _get_board_f_flash(env)
    flash_mode = _get_board_flash_mode(env)

    commands = []
    commands.append('--chip')
    commands.append(chip_type)
    # commands.append(' --flash_mode ')
    # commands.append(flash_mode)
    commands.append('merge_bin')
    commands.append('-o')
    commands.append(outputFilename)
    commands.append('--flash_size')
    commands.append(flash_size)
    commands.append('--flash_freq')
    commands.append(flash_freq)

    if chip_type == 'esp32':
        commands.append('0x1000')
        commands.append(bootloader_path)
    elif chip_type == 'esp32s3' or chip_type == 'esp32c3':
        commands.append('0x0000')
        commands.append(bootloader_path)        
    
    commands.append('0xe000')
    commands.append(boot_app0_path)
    commands.append(' 0x8000 ')
    commands.append(partitions_path)
    commands.append('0x10000')
    commands.append(firmware_path)

    if has_data:
        commands.append(str(data_offset))
        commands.append(dataOutputBinPath)

    print(commands)
    esptool.main(commands)



def before_upload(source, target, env):
    merge_bin_files(env)

def after_buildprog(source, target, env):
    merge_bin_files(env)

env.AddPostAction("buildprog", after_buildprog)
env.AddPreAction("upload", before_upload)

