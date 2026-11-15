"""Measure how long the game takes to show an input, by replay (docs/dev/frame-rate.md,
"Input latency").

A copy of a route gets one input forced at a moment of virtual time. The copy and the
original are both replayed by virtual time, dumping every frame around that moment. The
first frame that differs is when the game first showed the input. Replays are exact, so
the delay is exact.

  python -I tools/input_delay.py inject <route dir> <new route dir> <T> [--dur 0.5]
                                        [--field stick_y] [--value FF]
  python -I tools/input_delay.py cut <route dir> <new route dir> <T>
  python -I tools/input_delay.py diff <base run dir> <test run dir> <T>

- inject: copies the route with channel 0's field (buttons, stick_x, stick_y, cstick_x,
  cstick_y; hex, as the log has it) forced from T to T+dur seconds, then restored.
- cut: copies the route up to T, ending there, for a --fast-forward run that goes live
  at T (for GCN_LATENCYLOG).
- diff: reads two runs' dumps (frame_NNNNN.png) and SFASSAULT_FRAMELOG (frames.txt). It
  prints the first frame that differs and how long after T the game handed it over.

The route needs a v6 log (each pad line ends in its virtual time). Run each replay in its
own route folder: a replay writes a scratch memory card there. Needs Pillow for diff.
"""
import argparse
import shutil
import sys
from pathlib import Path

TB = 40500000  # time-base ticks per second
FIELDS = {'buttons': 4, 'stick_x': 5, 'stick_y': 6, 'cstick_x': 7, 'cstick_y': 8}


def read_log(route):
    raw = (Path(route) / 'inputs.txt').read_text(newline='')
    nl = '\r\n' if '\r\n' in raw else '\n'
    return raw.split(nl), nl


def pad(line):
    p = line.split()
    return p if len(p) == 12 and p[0].isdigit() else None


def new_route(src, dst, lines, nl):
    dst = Path(dst)
    dst.mkdir(parents=True, exist_ok=True)
    card = Path(src) / 'memcard_a.raw'
    if card.exists():
        shutil.copy(card, dst / 'memcard_a.raw')
    (dst / 'inputs.txt').write_text(nl.join(lines), newline='')


def inject(a):
    lines, nl = read_log(a.route)
    idx, t0, t1 = FIELDS[a.field], int(a.t * TB), int((a.t + a.dur) * TB)
    out, orig, phase = [], None, 0
    for line in lines:
        p = pad(line)
        if p:
            tick = int(p[11])
            if phase == 0 and tick >= t0 and orig:
                q = list(orig); q[idx] = a.value; q[11] = str(t0); out.append(' '.join(q)); phase = 1
            if phase == 1 and tick >= t1:
                q = list(orig); q[11] = str(t1); out.append(' '.join(q)); phase = 2
            if p[2] == '0':
                orig = list(p)
                if phase == 1:
                    p[idx] = a.value
                    line = ' '.join(p)
        out.append(line)
    if phase != 2:
        sys.exit('the log does not reach T + dur')
    new_route(a.route, a.new_route, out, nl)


def cut(a):
    lines, nl = read_log(a.route)
    t, out, last = int(a.t * TB), [], None
    for line in lines:
        p = pad(line)
        if p and int(p[11]) > t:
            break
        if line.startswith('# end'):
            continue
        last = p or last
        out.append(line)
    if not last:
        sys.exit('no pad line before T')
    out.append(f'# end {last[0]} {last[1]} {t}')
    new_route(a.route, a.new_route, out + [''], nl)


def diff(a):
    from PIL import Image, ImageChops
    base, test = Path(a.base), Path(a.test)
    times = {}
    for line in (test / 'frames.txt').read_text().splitlines():
        p = line.split()
        if len(p) == 2:
            times[int(p[0])] = int(p[1]) / TB
    dumps = lambda d: {int(f.stem.split('_')[1]): f for f in d.glob('frame_*.png')}
    fa, fb = dumps(base), dumps(test)
    for f in sorted(set(fa) & set(fb)):
        if ImageChops.difference(Image.open(fa[f]).convert('RGB'), Image.open(fb[f]).convert('RGB')).getbbox():
            t = times[f]
            first = min(g for g, tg in times.items() if tg >= a.t)
            print(f'first differs at frame {f}, {(t - a.t) * 1000:.1f} ms after T; '
                  f'{f - first} frame(s) after the first frame handed over after T')
            return
    print('no frame differs')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    i = sub.add_parser('inject')
    i.add_argument('route'); i.add_argument('new_route'); i.add_argument('t', type=float)
    i.add_argument('--dur', type=float, default=0.5)
    i.add_argument('--field', choices=FIELDS, default='stick_y')
    i.add_argument('--value', default='FF')
    c = sub.add_parser('cut')
    c.add_argument('route'); c.add_argument('new_route'); c.add_argument('t', type=float)
    d = sub.add_parser('diff')
    d.add_argument('base'); d.add_argument('test'); d.add_argument('t', type=float)
    a = ap.parse_args()
    {'inject': inject, 'cut': cut, 'diff': diff}[a.cmd](a)


if __name__ == '__main__':
    main()
