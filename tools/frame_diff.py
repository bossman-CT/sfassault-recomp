#!/usr/bin/env python3
"""Compare two directories of frame dumps (frame_NNNNN.png), as regress.ps1 makes them.

usage: frame_diff.py <baseline dir> <current dir> [<dir for side-by-sides>]

A frame counts as changed when more than CHANGED_FRACTION of its pixels differ by more than
PIXEL_THRESHOLD in brightness. Prints one summary line, then the most changed frames, and
writes baseline|current side-by-sides of up to three of them. Exit code: 0 when nothing
changed, 1 when something did, 2 when the dumps can't be compared. Needs Pillow
(pip install pillow).
"""
import sys
from pathlib import Path

try:
    from PIL import Image, ImageChops
except ImportError:
    sys.exit("frame_diff.py needs Pillow: pip install pillow")

PIXEL_THRESHOLD = 24
CHANGED_FRACTION = 0.005


def changed_fraction(a, b):
    diff = ImageChops.difference(a, b).convert('L').point(lambda v: 255 if v > PIXEL_THRESHOLD else 0)
    return diff.histogram()[255] / (diff.width * diff.height)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    base, cur = Path(sys.argv[1]), Path(sys.argv[2])
    pairs_dir = Path(sys.argv[3]) if len(sys.argv) > 3 else None
    names = sorted(p.name for p in base.glob('frame_*.png'))
    if not names:
        print(f'no baseline frames in {base}')
        return 2
    missing = [n for n in names if not (cur / n).exists()]
    results = []
    for n in names:
        if n in missing:
            continue
        a, b = Image.open(base / n).convert('RGB'), Image.open(cur / n).convert('RGB')
        results.append((changed_fraction(a, b) if a.size == b.size else 1.0, n))
    changed = sorted((r for r in results if r[0] > CHANGED_FRACTION), reverse=True)
    print(f'{len(results)} frames compared, {len(changed)} changed, {len(missing)} missing')
    for frac, n in changed[:8]:
        print(f'  {n}: {frac * 100:.1f}% of pixels changed')
    if missing:
        print(f'  missing from the current run: {missing[0]} .. {missing[-1]}')
    if pairs_dir and changed:
        pairs_dir.mkdir(parents=True, exist_ok=True)
        for _, n in changed[:3]:
            a, b = Image.open(base / n).convert('RGB'), Image.open(cur / n).convert('RGB')
            pair = Image.new('RGB', (a.width + b.width + 8, max(a.height, b.height)), (255, 0, 255))
            pair.paste(a, (0, 0))
            pair.paste(b, (a.width + 8, 0))
            pair.save(pairs_dir / f'pair_{n}')
        print(f'  side-by-sides (baseline | current) in {pairs_dir}')
    return 1 if changed or missing else 0


if __name__ == '__main__':
    sys.exit(main())
