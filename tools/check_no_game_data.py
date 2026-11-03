#!/usr/bin/env python3
"""Fail if any tracked file looks like game data.

Checks every file git tracks (or the paths given on the command line) for disc-image and
extracted-file extensions, the GameCube disc header magic, and binaries over the size
limit. Run it before committing; CI runs it on every pull request.
"""
import subprocess
import sys
from pathlib import Path

BANNED_EXTENSIONS = {
    ".iso", ".gcm", ".rvz", ".wia", ".gcz", ".ciso", ".wbfs", ".nfs", ".tgc",
    ".dol", ".rel", ".thp", ".adp", ".dsp", ".ast", ".gct",
}
# The magic word at offset 0x1C of every GameCube disc image.
GC_DISC_MAGIC = bytes.fromhex("C2339F3D")
MAX_BINARY_BYTES = 512 * 1024
# Paths allowed to hold larger binaries, such as shader-key tables gcn-recomp generates.
ALLOWED_LARGE = {"src/shader_keys.bin"}


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True).stdout
    return [p for p in out.decode().split("\0") if p]


def is_binary(data):
    return b"\0" in data[:8192]


def problems(path):
    p = Path(path)
    if not p.is_file():
        return []  # submodules and deleted files
    found = []
    if p.suffix.lower() in BANNED_EXTENSIONS:
        found.append(f"extension {p.suffix} is game data")
    with p.open("rb") as f:
        head = f.read(0x20)
        f.seek(0)
        sample = f.read(8192)
    if head[0x1C:0x20] == GC_DISC_MAGIC:
        found.append("has a GameCube disc header")
    size = p.stat().st_size
    if is_binary(sample) and size > MAX_BINARY_BYTES and path not in ALLOWED_LARGE:
        found.append(f"binary file of {size // 1024} KiB (limit {MAX_BINARY_BYTES // 1024} KiB)")
    if path.startswith("build") and "/" in path:
        found.append("is inside a build directory")
    return found


def main():
    paths = sys.argv[1:] or tracked_files()
    bad = {path: problems(path) for path in paths}
    bad = {path: why for path, why in bad.items() if why}
    for path, why in bad.items():
        print(f"{path}: {'; '.join(why)}")
    if bad:
        print("\nThese files look like game data and can't be committed. See CONTRIBUTING.md.")
        return 1
    print(f"OK: {len(paths)} files checked, no game data found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
