Import("env")

from datetime import datetime
from pathlib import Path
import subprocess


PROJECT_DIR = Path(env["PROJECT_DIR"])
FACTORY_DIR = PROJECT_DIR / "examples" / "factory"
GENERATED_CPP = FACTORY_DIR / "factory_build_info_autogen.cpp"
sw_version = env.GetProjectOption("custom_sw_version", "dev")


def run_git(*args):
    try:
        return subprocess.check_output(
            ["git", *args],
            cwd=PROJECT_DIR,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except Exception:
        return ""


def is_dirty():
    try:
        result = subprocess.run(
            ["git", "diff", "--quiet", "--ignore-submodules", "HEAD", "--"],
            cwd=PROJECT_DIR,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        return result.returncode != 0
    except Exception:
        return False


git_hash = run_git("rev-parse", "--short=8", "HEAD") or "unknown"
if git_hash != "unknown" and is_dirty():
    git_hash += "-dirty"

build_stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def c_string_literal(value):
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\r", "\\r")
        .replace("\n", "\\n")
    )


generated_cpp = """#include "factory_build_info.h"

const char kFactorySoftwareVersion[] = "{sw_version}";
const char kFactoryGitHash[] = "{git_hash}";
const char kFactoryBuildStamp[] = "{build_stamp}";
""".format(
    sw_version=c_string_literal(sw_version),
    git_hash=c_string_literal(git_hash),
    build_stamp=c_string_literal(build_stamp),
)

if not GENERATED_CPP.exists() or GENERATED_CPP.read_text(encoding="utf-8") != generated_cpp:
    GENERATED_CPP.write_text(generated_cpp, encoding="utf-8")
