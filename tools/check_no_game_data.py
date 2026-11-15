#!/usr/bin/env python3
"""Fail if any tracked file looks like game data.

Checks every file git tracks (or the paths given on the command line) for:
- disc images and files extracted from the disc, by extension and by the disc header;
- memory-card and save files, which hold game assets such as the save icon;
- frame dumps, and images, audio and video, which would be game imagery or sound;
- compiled binaries and archives, since a build of the game contains its code;
- generated (recompiled) code from build/, and pasted disassembly or decompiler output;
- anything under rom/, saves/ or a build directory;
- binaries over the size limit.

Run it before committing (the .githooks/pre-commit hook does); CI runs it on every pull
request, and on each commit in it. A file it flags must not be committed: this project is
only legal to host because it contains no part of the game.
"""
import re
import subprocess
import sys
from pathlib import Path

BANNED_EXTENSIONS = {
    # disc images
    ".iso", ".gcm", ".rvz", ".wia", ".gcz", ".ciso", ".wbfs", ".nfs", ".tgc",
    # executables and code modules
    ".dol", ".rel", ".elf", ".map",
    # common GameCube data formats: movies, streamed audio, banners, cheat lists, textures,
    # fonts, archives
    ".thp", ".adp", ".dsp", ".ast", ".bnr", ".gct", ".tpl", ".bti", ".bfn", ".szs", ".arc",
    # the formats on this game's disc
    ".pac", ".c3d", ".cbd", ".chd", ".nut", ".fpc", ".mot", ".sfd", ".efb",
    # memory-card and save files, and Dolphin save states and movies
    ".raw", ".gci", ".gcs", ".sav", ".dtm",
    ".s01", ".s02", ".s03", ".s04", ".s05", ".s06", ".s07", ".s08", ".s09", ".s10",
}
# A built game holds its recompiled code, so no compiled output may be committed.
BUILD_EXTENSIONS = {
    ".exe", ".dll", ".so", ".dylib", ".o", ".obj", ".a", ".lib", ".pdb", ".app", ".msi",
}
# Extensions too generic to ban outright: only the paths listed here may use them. Images,
# audio and video would be screenshots, dumps or the game's own artwork and sound; archives
# could hold anything; .s is disassembly.
RESTRICTED_EXTENSIONS = {
    ".bin", ".dat", ".s",
    ".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".tga", ".dds", ".tif", ".tiff",
    ".ico", ".icns", ".svg", ".psd",
    ".wav", ".ogg", ".mp3", ".flac", ".opus", ".aif", ".aiff",
    ".mp4", ".webm", ".mkv", ".avi", ".mov",
    ".zip", ".7z", ".rar", ".tar", ".gz", ".xz", ".zst", ".bz2",
}
# Original files a maintainer has checked, which may use a restricted extension.
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
# Lines a disassembler or decompiler writes for the game's code: dtk's
# "/* 80003100 00000100  7C 08 02 A6 */", a debugger's "80003100: 7c0802a6 mflr",
# and Ghidra's names. A few in a design note are fine; a pasted listing is not.
DISASM_PATTERNS = (
    re.compile(rb"/\* [0-9A-Fa-f]{8} [0-9A-Fa-f]{8}  (?:[0-9A-Fa-f]{2} ){3}[0-9A-Fa-f]{2} \*/"),
    re.compile(rb"\b8[01][0-9A-Fa-f]{6}:?[ \t]+(?:[0-9A-Fa-f]{8}|(?:[0-9A-Fa-f]{2} ){3}[0-9A-Fa-f]{2})[ \t]+[a-z]{1,8}\.?\b"),
    re.compile(rb"\b(?:FUN|DAT|LAB|PTR)_8[01][0-9A-Fa-f]{6}\b|\bundefined[1248]\b|\b[iuf]Var\d+\b"),
)
DISASM_LIMIT = 20


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
    if ext in BUILD_EXTENSIONS:
        found.append(f"{p.suffix} is compiled output, and a build of the game contains its code")
    if ext in RESTRICTED_EXTENSIONS and norm not in ALLOWED_PATHS:
        found.append(f"{p.suffix} files are allowed only at listed paths (ALLOWED_PATHS)")
    if "memcard" in name:
        found.append("looks like a memory card file")
    if name.startswith("frame_") and ext in (".png", ".jpg", ".webp", ".bmp"):
        found.append("looks like a frame dump")
    if norm.lower().startswith(BANNED_DIRS) and norm not in ALLOWED_PATHS:
        found.append("is in a folder that holds game data or build output")
    data = p.read_bytes()
    if data[0x1C:0x20] == GC_DISC_MAGIC:
        found.append("has a GameCube disc header")
    if is_binary(data):
        if len(data) > MAX_BINARY_BYTES and norm not in ALLOWED_PATHS:
            found.append(f"binary file of {len(data) // 1024} KiB (limit {MAX_BINARY_BYTES // 1024} KiB)")
    else:
        if sum(data.count(m) for m in GENERATED_MARKERS) > GENERATED_LIMIT:
            found.append("looks like generated (recompiled) code")
        if sum(len(r.findall(data)) for r in DISASM_PATTERNS) > DISASM_LIMIT:
            found.append("looks like disassembly or decompiler output")
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
