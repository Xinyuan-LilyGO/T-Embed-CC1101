#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import configparser
import json
import shutil
import subprocess
import sys
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parent.parent
EXAMPLES_DIR = ROOT / "examples"
PLATFORMIO_INI = ROOT / "platformio.ini"
FIRMWARE_EXAMPLES_DIR = ROOT / "firmware" / "examples"
DEFAULT_ENV = "T_Embed_CC1101"
DEFAULT_CHIP = "esp32s3"
SOURCE_EXTENSIONS = (".ino", ".cpp", ".c", ".cc", ".cxx")


def discover_examples() -> list[str]:
    candidate_dirs: set[str] = set()
    for source_path in EXAMPLES_DIR.rglob("*"):
        if not source_path.is_file():
            continue
        if source_path.suffix.lower() not in SOURCE_EXTENSIONS:
            continue

        rel_dir = source_path.parent.relative_to(EXAMPLES_DIR).as_posix()
        if rel_dir:
            candidate_dirs.add(rel_dir)

    examples: list[str] = []
    for rel_dir in sorted(candidate_dirs, key=lambda item: (item.count("/"), item)):
        parent = Path(rel_dir).parent.as_posix()
        if parent != "." and parent in candidate_dirs:
            continue
        examples.append(rel_dir)

    return examples


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


def capture_command(command: list[str], cwd: Path) -> str:
    print(f"$ {' '.join(command)}", flush=True)
    completed = subprocess.run(
        command,
        cwd=cwd,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return completed.stdout


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


def _offset_as_int(offset: str) -> int:
    return int(str(offset).strip(), 0)


def extract_envdump_literal(text: str, key: str) -> str:
    marker = f"'{key}':"
    start = text.find(marker)
    if start < 0:
        raise RuntimeError(f"Could not find {key} in PlatformIO envdump output")

    pos = start + len(marker)
    while pos < len(text) and text[pos].isspace():
        pos += 1
    if pos >= len(text):
        raise RuntimeError(f"Missing value for {key} in PlatformIO envdump output")

    first = text[pos]
    if first in ("'", '"'):
        end = pos + 1
        while end < len(text):
            if text[end] == first and text[end - 1] != "\\":
                return text[pos : end + 1]
            end += 1
        raise RuntimeError(f"Unterminated string value for {key} in envdump output")

    if first not in "[{(":
        end = pos
        while end < len(text) and text[end] not in ",\r\n":
            end += 1
        return text[pos:end].strip()

    pairs = {"[": "]", "{": "}", "(": ")"}
    close_char = pairs[first]
    depth = 0
    end = pos
    while end < len(text):
        ch = text[end]
        if ch == first:
            depth += 1
        elif ch == close_char:
            depth -= 1
            if depth == 0:
                return text[pos : end + 1]
        end += 1

    raise RuntimeError(f"Unterminated container value for {key} in envdump output")


def load_build_flash_layout(env_name: str, project_conf: Path) -> list[tuple[str, Path]]:
    pio = shutil.which("pio") or "pio"
    envdump_output = capture_command(
        [pio, "run", "-e", env_name, "-t", "envdump", "--project-conf", str(project_conf)],
        ROOT,
    )

    flash_extra_images = ast.literal_eval(
        extract_envdump_literal(envdump_output, "FLASH_EXTRA_IMAGES")
    )
    app_offset = str(ast.literal_eval(
        extract_envdump_literal(envdump_output, "ESP32_APP_OFFSET")
    )).strip()

    if not flash_extra_images:
        raise RuntimeError("FLASH_EXTRA_IMAGES is empty in PlatformIO envdump output")
    if not app_offset:
        raise RuntimeError("ESP32_APP_OFFSET is empty in PlatformIO envdump output")

    build_dir = ROOT / ".pio" / "build" / env_name
    firmware_path = build_dir / "firmware.bin"
    if not firmware_path.exists():
        raise FileNotFoundError(f"Missing build artifact: {firmware_path}")

    layout: list[tuple[str, Path]] = []
    for offset, path_text in flash_extra_images:
        image_path = Path(str(path_text))
        if not image_path.exists():
            raise FileNotFoundError(f"Flash image not found: {image_path}")
        layout.append((str(offset).strip(), image_path))

    layout.append((app_offset, firmware_path))
    layout.sort(key=lambda item: _offset_as_int(item[0]))
    return layout


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


def remove_legacy_output(example_name: str) -> None:
    legacy_dir = (FIRMWARE_EXAMPLES_DIR / Path(example_name)).resolve()
    output_root = FIRMWARE_EXAMPLES_DIR.resolve()

    if output_root != legacy_dir and output_root not in legacy_dir.parents:
        raise RuntimeError(f"Refusing to remove unexpected output directory: {legacy_dir}")

    if legacy_dir.exists():
        shutil.rmtree(legacy_dir)

    parent = legacy_dir.parent
    while parent != output_root and parent.exists() and not any(parent.iterdir()):
        parent.rmdir()
        parent = parent.parent


def merge_artifacts(
    example_name: str,
    chip: str,
    board_settings: dict[str, str],
    flash_layout: list[tuple[str, Path]],
) -> Path:
    FIRMWARE_EXAMPLES_DIR.mkdir(parents=True, exist_ok=True)
    remove_legacy_output(example_name)

    merged_bin = FIRMWARE_EXAMPLES_DIR / make_merged_bin_name(example_name)
    command = [
        sys.executable,
        str(ROOT / "script" / "esptool.py"),
        "--chip",
        chip,
        "merge_bin",
        "-o",
        str(merged_bin),
        "--flash_size",
        board_settings["flash_size"],
        "--flash_freq",
        board_settings["flash_freq"],
    ]
    for offset, source_path in flash_layout:
        command.extend([offset, str(source_path)])

    run_command(command, ROOT)
    return merged_bin


def export_artifacts(
    example_name: str,
    chip: str,
    merge: bool,
    board_settings: dict[str, str],
    flash_layout: list[tuple[str, Path]],
) -> Path:
    if not merge:
        raise RuntimeError(
            "This script now only exports merged example bin files. Remove --no-merge."
        )

    return merge_artifacts(example_name, chip, board_settings, flash_layout)


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
        flash_layout = load_build_flash_layout(env_name, temp_conf)
        return export_artifacts(example_name, chip, merge, board_settings, flash_layout)
    finally:
        temp_conf.unlink(missing_ok=True)


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
            "Build one or more examples from examples/ and export only the merged "
            "example-named bin file to firmware/examples/"
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
        help=f"Chip name used when merging the output bin. Default: {DEFAULT_CHIP}",
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
        help="Deprecated. This script now always exports only the merged example bin.",
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
            output_path = build_example(
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

        successes.append((example_name, output_path))
        print()
        print(f"Build complete: {example_name}")
        print(f"Output file: {output_path}")

    print()
    print("Summary:")
    for example_name, output_path in successes:
        print(f"  [OK]   {example_name} -> {output_path}")
    for example_name, error_text in failures:
        print(f"  [FAIL] {example_name} -> {error_text}")

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
