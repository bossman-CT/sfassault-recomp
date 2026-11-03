#!/usr/bin/env python3
"""Extract main.dol from a GameCube disc image and print its SHA-1.

Usage: extract_dol.py [image.iso] [-o OUT] [--verify-disc]

The image defaults to the first .iso in rom/, and the DOL is written to
analysis/out/main.dol, which git ignores. --verify-disc also hashes the whole image and
compares it with Redump's entry for GF7E01, which takes a while.

The build does this itself with gcn-recomp's recomp/extract_dol.py once the submodule is
checked out. This script only exists so the analysis can run without it.
"""
import argparse
import hashlib
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DISC_ID = b"GF7E01"
# Redump disc 1857, Star Fox: Assault (USA), revision 0.
REDUMP_SHA1 = "065341b8575adea00fd0a75e389f135d32840423"


def find_image():
    images = sorted((ROOT / "rom").glob("*.iso"))
    if not images:
        sys.exit("error: no .iso in rom/; pass the image's path")
    return images[0]


def expected_dol_sha1():
    m = re.search(r'set\(SFASSAULT_DOL_SHA1 "([0-9a-f]*)"\)',
                  (ROOT / "CMakeLists.txt").read_text())
    return m.group(1) if m else ""


def read_dol(f):
    """The DOL's offset at 0x420 of the disc header; its size is the end of its furthest
    section, from the 7 text and 11 data section offsets and sizes in its own header."""
    f.seek(0x420)
    dol_off = struct.unpack(">I", f.read(4))[0]
    f.seek(dol_off)
    dh = f.read(0x100)
    offs = struct.unpack(">18I", dh[0x00:0x48])
    sizes = struct.unpack(">18I", dh[0x90:0xD8])
    dol_size = max(o + s for o, s in zip(offs, sizes) if s)
    f.seek(dol_off)
    return dol_off, f.read(dol_size)


def sha1_file(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        while chunk := f.read(1 << 24):
            h.update(chunk)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("image", nargs="?", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=ROOT / "analysis" / "out" / "main.dol")
    ap.add_argument("--verify-disc", action="store_true")
    args = ap.parse_args()
    image = args.image or find_image()

    with open(image, "rb") as f:
        hdr = f.read(0x20)
        if hdr[:6] != DISC_ID:
            sys.exit(f"error: {image} is disc {hdr[:6]!r}, not {DISC_ID!r} (only .iso is supported)")
        dol_off, dol = read_dol(f)
    print(f"disc:     {hdr[:6].decode()}, revision {hdr[7]}")
    print(f"main.dol: offset 0x{dol_off:X}, 0x{len(dol):X} bytes")

    sha1 = hashlib.sha1(dol).hexdigest()
    print(f"main.dol: SHA-1 {sha1}")
    expected = expected_dol_sha1()
    if expected and sha1 != expected:
        sys.exit(f"error: this isn't the supported build (CMakeLists.txt expects {expected})")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(dol)
    print(f"wrote {args.out}")

    if args.verify_disc:
        disc_sha1 = sha1_file(image)
        ok = "matches Redump" if disc_sha1 == REDUMP_SHA1 else "does NOT match Redump"
        print(f"disc:     SHA-1 {disc_sha1} ({ok})")
        if disc_sha1 != REDUMP_SHA1:
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
