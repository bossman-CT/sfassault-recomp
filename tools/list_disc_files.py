#!/usr/bin/env python3
"""List the files on a GameCube disc image, with their sizes.

Usage: list_disc_files.py [image.iso] [--unusual]

The image defaults to the first .iso in rom/. --unusual lists only files that might name
functions or hold leftover debug data: symbol and map files, executables, text and
anything with an extension the rest of the disc doesn't use much. It prints paths, sizes
and offsets only, never file contents.
"""
import argparse
import collections
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# Extensions that can name functions or come from a developer's machine.
SUSPECT_EXTENSIONS = {
    ".map", ".elf", ".sym", ".txt", ".dbg", ".pdb", ".o", ".a", ".lib", ".c", ".cpp",
    ".h", ".s", ".mcp", ".lcf", ".bak", ".log", ".ini", ".xml", ".csv", ".plf", ".dol",
    ".rel", ".str",
}
# An extension used by this few files or fewer counts as rare.
RARE_COUNT = 2


def find_image():
    images = sorted((ROOT / "rom").glob("*.iso"))
    if not images:
        sys.exit("error: no .iso in rom/; pass the image's path")
    return images[0]


def read_fst(f):
    """Yields (path, offset, size) for every file in the disc's file system table, whose
    offset and size are at 0x424 and 0x428 of the disc header. Each 12-byte entry holds a
    directory flag and a name offset into the string table that follows the entries;
    then a file's offset and size, or a directory's parent and the index after its last
    child."""
    f.seek(0x424)
    fst_off, fst_size = struct.unpack(">II", f.read(8))
    f.seek(fst_off)
    fst = f.read(fst_size)
    count = struct.unpack(">I", fst[8:12])[0]
    strings = fst[count * 12:]

    def name(i):
        off = struct.unpack(">I", fst[i * 12:i * 12 + 4])[0] & 0xFFFFFF
        return strings[off:strings.index(b"\0", off)].decode("shift_jis", "replace")

    dirs = [(count, "")]  # (index after the directory's last child, its path)
    for i in range(1, count):
        while i >= dirs[-1][0]:
            dirs.pop()
        is_dir = fst[i * 12] != 0
        a, b = struct.unpack(">II", fst[i * 12 + 4:i * 12 + 12])
        path = f"{dirs[-1][1]}/{name(i)}"
        if is_dir:
            dirs.append((b, path))
        else:
            yield path, a, b


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("image", nargs="?", type=Path)
    ap.add_argument("--unusual", action="store_true")
    args = ap.parse_args()
    with open(args.image or find_image(), "rb") as f:
        files = list(read_fst(f))

    ext_count = collections.Counter(Path(p).suffix.lower() for p, _, _ in files)
    if args.unusual:
        files = [(p, o, s) for p, o, s in files
                 if Path(p).suffix.lower() in SUSPECT_EXTENSIONS
                 or ext_count[Path(p).suffix.lower()] <= RARE_COUNT]
    for path, off, size in files:
        print(f"{size:>11,}  0x{off:08X}  {path}")
    total = sum(s for _, _, s in files)
    print(f"{len(files)} files, {total:,} bytes")
    if not args.unusual:
        print("by extension: " + ", ".join(f"{e or '(none)'} {n}" for e, n in ext_count.most_common()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
