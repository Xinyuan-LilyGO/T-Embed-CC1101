Import("env")

from datetime import datetime
from pathlib import Path
import subprocess


PROJECT_DIR = Path(env["PROJECT_DIR"])
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

env.Append(
    CPPDEFINES=[
        ("T_EMBED_CC1101_SF_VER", '\\"{}\\"'.format(sw_version)),
        ("FACTORY_GIT_HASH", '\\"{}\\"'.format(git_hash)),
        ("FACTORY_BUILD_STAMP", '\\"{}\\"'.format(build_stamp)),
    ]
)
