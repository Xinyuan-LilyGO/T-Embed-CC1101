#!/usr/bin/env python3
from __future__ import annotations

import argparse
import configparser
import json
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent.parent
EXAMPLES_DIR = ROOT / "examples"
PLATFORMIO_INI = ROOT / "platformio.ini"
FIRMWARE_EXAMPLES_DIR = ROOT / "firmware" / "examples"
DEFAULT_ENV = "T_Embed_CC1101"
DEFAULT_CHIP = "esp32s3"
MERGED_COMMAND_NAME = "esptool_merged_command.txt"
BUILD_OFFSETS = {
    "bootloader.bin": "0x0",
    "partitions.bin": "0x8000",
    "firmware.bin": "0x10000",
}
BUILD_FILES = (
    "bootloader.bin",
    "partitions.bin",
    "firmware.bin",
    "firmware.elf",
    "firmware.map",
)


def discover_examples() -> list[str]:
    examples: set[str] = set()
    for ino_path in EXAMPLES_DIR.rglob("*.ino"):
        rel_dir = ino_path.parent.relative_to(EXAMPLES_DIR).as_posix()
        if rel_dir:
            examples.add(rel_dir)
    return sorted(examples)


def normalize_example_name(raw_name: str, available: list[str]) -> str:
    normalized = raw_name.strip().replace("\\", "/").strip("/")
    if normalized.startswith("examples/"):
        normalized = normalized[len("examples/") :]

    if normalized in available:
        return normalized

    basename_matches = [item for item in available if Path(item).name == normalized]
    if len(basename_matches) == 1:
        return basename_matches[0]

    available_text = "\n".join(f"  - {item}" for item in available)
    raise SystemExit(
        f"Unknown example: {raw_name}\n\nAvailable examples:\n{available_text}"
    )


def replace_src_dir(config_text: str, example_name: str) -> str:
    target_src_dir = f"examples/{example_name}"
    lines = config_text.splitlines()
    output: list[str] = []
    in_platformio = False
    replaced = False

    for line in lines:
        stripped = line.lstrip()

        if stripped.startswith("[") and stripped.endswith("]"):
            if in_platformio and not replaced:
                output.append(f"src_dir = {target_src_dir}")
                replaced = True
            in_platformio = stripped == "[platformio]"
            output.append(line)
            continue

        if in_platformio and not stripped.startswith((";", "#")):
            if re.match(r"^src_dir\s*=", stripped):
                if not replaced:
                    indent = line[: len(line) - len(stripped)]
                    output.append(f"{indent}src_dir = {target_src_dir}")
                    replaced = True
                continue

        output.append(line)

    if in_platformio and not replaced:
        output.append(f"src_dir = {target_src_dir}")
        replaced = True

    if not replaced:
        raise RuntimeError("Could not find [platformio] section in platformio.ini")

    return "\n".join(output) + "\n"


def write_temp_project_conf(example_name: str) -> Path:
    config_text = PLATFORMIO_INI.read_text(encoding="utf-8")
    generated_text = replace_src_dir(config_text, example_name)
    safe_name = example_name.replace("/", "__")
    temp_conf = ROOT / f".platformio.generated.{safe_name}.ini"
    temp_conf.write_text(generated_text, encoding="utf-8")
    return temp_conf


def run_command(command: list[str], cwd: Path) -> None:
    print(f"$ {' '.join(command)}", flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def load_env_board_settings(env_name: str) -> dict[str, str]:
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    parser.read(PLATFORMIO_INI, encoding="utf-8")

    env_section = f"env:{env_name}"
    if not parser.has_section(env_section):
        raise RuntimeError(f"PlatformIO environment not found: {env_name}")

    board_name = parser.get(env_section, "board", fallback="").strip()
    if not board_name:
        raise RuntimeError(f"Missing 'board' setting in [{env_section}]")

    boards_dir = parser.get("platformio", "boards_dir", fallback="boards").strip()
    board_path = ROOT / boards_dir / f"{board_name}.json"
    if not board_path.exists():
        raise FileNotFoundError(f"Board definition not found: {board_path}")

    board_data = json.loads(board_path.read_text(encoding="utf-8"))
    build_data = board_data.get("build", {})
    upload_data = board_data.get("upload", {})

    flash_mode = str(build_data.get("flash_mode", "dio")).strip()
    flash_freq = normalize_flash_freq(build_data.get("f_flash", "40000000L"))
    flash_size = str(upload_data.get("flash_size", "4MB")).strip()

    return {
        "board_name": board_name,
        "flash_mode": flash_mode,
        "flash_freq": flash_freq,
        "flash_size": flash_size,
    }


def normalize_flash_freq(value: object) -> str:
    raw = str(value).strip()
    lowered = raw.lower().rstrip("l")

    if lowered.endswith("m"):
        return lowered

    digits = "".join(ch for ch in lowered if ch.isdigit())
    if not digits:
        raise RuntimeError(f"Unsupported flash frequency value: {raw}")

    number = int(digits)
    if number >= 1_000_000:
        number //= 1_000_000

    return f"{number}m"


def remove_build_dir(env_name: str) -> None:
    build_root = (ROOT / ".pio" / "build").resolve()
    target_dir = (build_root / env_name).resolve()

    if build_root not in target_dir.parents:
        raise RuntimeError(f"Refusing to remove unexpected build directory: {target_dir}")

    if target_dir.exists():
        shutil.rmtree(target_dir)


def make_merged_bin_name(example_name: str) -> str:
    safe_name = example_name.strip().replace("\\", "/").strip("/").replace("/", "__")
    return f"{safe_name}.bin"


def merge_artifacts(
    example_name: str,
    output_dir: Path,
    chip: str,
    board_settings: dict[str, str],
) -> list[str]:
    merged_bin_name = make_merged_bin_name(example_name)
    merged_bin = output_dir / merged_bin_name
    command = [
        sys.executable,
        str(ROOT / "script" / "esptool.py"),
        "--chip",
        chip,
        "merge_bin",
        "-o",
        merged_bin_name,
        "--flash_mode",
        board_settings["flash_mode"],
        "--flash_freq",
        board_settings["flash_freq"],
        "--flash_size",
        board_settings["flash_size"],
    ]
    for filename, offset in BUILD_OFFSETS.items():
        command.extend([offset, filename])

    run_command(command, output_dir)

    merged_cmd = output_dir / MERGED_COMMAND_NAME
    merged_cmd.write_text(
        (
            f"python script/esptool.py --chip {chip} --baud 921600 write_flash -z "
            f"0x0 {merged_bin_name}\n"
        ),
        encoding="utf-8",
    )
    return [merged_bin.name, MERGED_COMMAND_NAME]


def export_artifacts(
    example_name: str,
    env_name: str,
    chip: str,
    merge: bool,
    board_settings: dict[str, str],
) -> Path:
    build_dir = ROOT / ".pio" / "build" / env_name
    if not build_dir.exists():
        raise FileNotFoundError(f"Build directory not found: {build_dir}")

    output_dir = FIRMWARE_EXAMPLES_DIR / Path(example_name) / "bin"
    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    copied_files: list[str] = []
    for filename in BUILD_FILES:
        source = build_dir / filename
        if not source.exists():
            if filename.endswith((".elf", ".map")):
                continue
            raise FileNotFoundError(f"Missing build artifact: {source}")
        shutil.copy2(source, output_dir / filename)
        copied_files.append(filename)

    flash_args = output_dir / "flash_args.txt"
    flash_args.write_text(
        "\n".join(f"{offset} {name}" for name, offset in BUILD_OFFSETS.items()) + "\n",
        encoding="utf-8",
    )

    esptool_cmd = output_dir / "esptool_command.txt"
    esptool_cmd.write_text(
        (
            f"python script/esptool.py --chip {chip} --baud 921600 write_flash -z "
            "0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin\n"
        ),
        encoding="utf-8",
    )

    extra_files = ["flash_args.txt", "esptool_command.txt"]
    if merge:
        extra_files.extend(
            merge_artifacts(example_name, output_dir, chip, board_settings)
        )

    build_info = {
        "example": example_name,
        "env": env_name,
        "chip": chip,
        "board": board_settings["board_name"],
        "generated_at": datetime.now().isoformat(timespec="seconds"),
        "source_dir": f"examples/{example_name}",
        "files": copied_files + extra_files,
        "offsets": BUILD_OFFSETS,
        "flash_mode": board_settings["flash_mode"],
        "flash_freq": board_settings["flash_freq"],
        "flash_size": board_settings["flash_size"],
        "merged_bin": make_merged_bin_name(example_name) if merge else None,
    }
    (output_dir / "build_info.json").write_text(
        json.dumps(build_info, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    return output_dir


def build_example(
    example_name: str,
    env_name: str,
    chip: str,
    clean: bool,
    merge: bool,
) -> Path:
    temp_conf = write_temp_project_conf(example_name)
    pio = shutil.which("pio") or "pio"
    board_settings = load_env_board_settings(env_name)

    try:
        if clean:
            remove_build_dir(env_name)

        run_command(
            [pio, "run", "-e", env_name, "--project-conf", str(temp_conf)],
            ROOT,
        )
    finally:
        temp_conf.unlink(missing_ok=True)

    return export_artifacts(example_name, env_name, chip, merge, board_settings)


def unique_in_order(items: list[str]) -> list[str]:
    seen: set[str] = set()
    result: list[str] = []
    for item in items:
        if item in seen:
            continue
        seen.add(item)
        result.append(item)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Build one or more examples from examples/ and export flashable bin files to "
            "firmware/examples/<example>/bin/"
        )
    )
    parser.add_argument(
        "examples",
        nargs="*",
        help="Example name(s), e.g. test_ir_send_recv or extend_nrf2401/recv",
    )
    parser.add_argument(
        "--env",
        default=DEFAULT_ENV,
        help=f"PlatformIO environment name. Default: {DEFAULT_ENV}",
    )
    parser.add_argument(
        "--chip",
        default=DEFAULT_CHIP,
        help=f"Chip name used in esptool_command.txt. Default: {DEFAULT_CHIP}",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List buildable examples under examples/",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Build all discovered examples under examples/",
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Skip the clean step before building",
    )
    parser.add_argument(
        "--no-merge",
        action="store_true",
        help="Skip generating the example-named merged bin",
    )
    parser.add_argument(
        "--continue-on-error",
        action="store_true",
        help="Keep building remaining examples if one example fails",
    )
    args = parser.parse_args()

    available = discover_examples()
    if args.list:
        print("Available examples:")
        for item in available:
            print(f"  - {item}")
        return 0

    if not args.examples and not args.all:
        parser.print_help()
        print("\nTip: use --list to see available examples.")
        return 1

    requested_examples = available if args.all else [
        normalize_example_name(item, available) for item in args.examples
    ]
    requested_examples = unique_in_order(requested_examples)

    successes: list[tuple[str, Path]] = []
    failures: list[tuple[str, str]] = []

    for index, example_name in enumerate(requested_examples, start=1):
        print()
        print("=" * 80)
        print(f"[{index}/{len(requested_examples)}] Building {example_name}")
        print("=" * 80)

        try:
            output_dir = build_example(
                example_name=example_name,
                env_name=args.env,
                chip=args.chip,
                clean=not args.no_clean,
                merge=not args.no_merge,
            )
        except (subprocess.CalledProcessError, FileNotFoundError, RuntimeError) as exc:
            failures.append((example_name, str(exc)))
            print(f"[FAIL] {example_name}: {exc}", file=sys.stderr)
            if not args.continue_on_error:
                break
            continue

        successes.append((example_name, output_dir))
        print()
        print(f"Build complete: {example_name}")
        print(f"Output folder: {output_dir}")
        print("Files:")
        for path in sorted(output_dir.iterdir()):
            if path.is_file():
                print(f"  - {path.name}")

    print()
    print("Summary:")
    for example_name, output_dir in successes:
        print(f"  [OK]   {example_name} -> {output_dir}")
    for example_name, error_text in failures:
        print(f"  [FAIL] {example_name} -> {error_text}")

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
