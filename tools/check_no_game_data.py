#!/usr/bin/env python3
"""Fail if any tracked file looks like game data.

Checks every file git tracks (or the paths given on the command line) for:
- disc images and files extracted from the disc, by extension and by the disc header;
- memory-card and save files, which hold game assets such as the save icon;
- frame dumps and generated (recompiled) code from build/;
- anything under rom/, saves/ or a build directory;
- binaries over the size limit.

Run it before committing (the .githooks/pre-commit hook does); CI runs it on every pull
request. A file it flags must not be committed: this project is only legal to host
because it contains no part of the game.
"""
import subprocess
import sys
from pathlib import Path

BANNED_EXTENSIONS = {
    # disc images
    ".iso", ".gcm", ".rvz", ".wia", ".gcz", ".ciso", ".wbfs", ".nfs", ".tgc",
    # executables and code modules
    ".dol", ".rel", ".elf", ".map",
    # common GameCube data formats: movies, streamed audio, banners, cheat lists
    ".thp", ".adp", ".dsp", ".ast", ".bnr", ".gct",
    # the formats on this game's disc
    ".pac", ".c3d", ".cbd", ".chd", ".nut", ".fpc", ".mot", ".sfd", ".efb",
    # memory-card and save files
    ".raw", ".gci", ".gcs", ".sav",
}
# Extensions too generic to ban outright: only the paths listed here may use them.
RESTRICTED_EXTENSIONS = {".bin", ".dat"}
ALLOWED_PATHS = {"src/shader_keys.bin", "rom/README.md"}
# Where game data lives on a developer's machine; nothing from these may be committed.
BANNED_DIRS = ("rom/", "saves/", "build/", "build-", "orig/", "analysis/out/")
# The magic word at offset 0x1C of every GameCube disc image.
GC_DISC_MAGIC = bytes.fromhex("C2339F3D")
MAX_BINARY_BYTES = 512 * 1024
# Recompiled code reads the guest's registers as c->r[n] and c->f[n] thousands of times per
# file; hand-written code almost never does.
GENERATED_MARKERS = (b"c->r[", b"c->f[")
GENERATED_LIMIT = 200


def tracked_files():
    out = subprocess.run(["git", "ls-files", "-z"], capture_output=True, check=True).stdout
    return [p for p in out.decode().split("\0") if p]


def is_binary(data):
    return b"\0" in data[:8192]


def problems(path):
    p = Path(path)
    if not p.is_file():
        return []  # submodules and deleted files
    norm = path.replace("\\", "/")
    name, ext = p.name.lower(), p.suffix.lower()
    found = []
    if ext in BANNED_EXTENSIONS:
        found.append(f"extension {p.suffix} is game data")
    if ext in RESTRICTED_EXTENSIONS and norm not in ALLOWED_PATHS:
        found.append(f"{p.suffix} files are allowed only at listed paths (ALLOWED_PATHS)")
    if "memcard" in name:
        found.append("looks like a memory card file")
    if name.startswith("frame_") and ext in (".png", ".jpg", ".webp", ".bmp"):
        found.append("looks like a frame dump")
    if norm.startswith(BANNED_DIRS) and norm not in ALLOWED_PATHS:
        found.append("is in a folder that holds game data or build output")
    data = p.read_bytes()
    if data[0x1C:0x20] == GC_DISC_MAGIC:
        found.append("has a GameCube disc header")
    if is_binary(data):
        if len(data) > MAX_BINARY_BYTES and norm not in ALLOWED_PATHS:
            found.append(f"binary file of {len(data) // 1024} KiB (limit {MAX_BINARY_BYTES // 1024} KiB)")
    elif sum(data.count(m) for m in GENERATED_MARKERS) > GENERATED_LIMIT:
        found.append("looks like generated (recompiled) code")
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
