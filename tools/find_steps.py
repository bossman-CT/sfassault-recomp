"""List candidate per-frame steps in the recompiled game (issue #12).

A per-frame step is a value loaded from memory, changed by an arithmetic instruction that
gcn-recomp can scale (recomp/ppc.py, translate_step) and stored back to the same address:
x += t, x -= t, x *= k, x = x*k + b, an integer x += 1. This reads the generated C in
build/gen (each statement carries its instruction word), follows each store back through
its basic block to the instruction that computed the stored register, and reports it when
one of that instruction's operands was loaded from the stored address.

  python tools/find_steps.py [build/gen] > candidates.txt

Each line is `<arith pc> <operand> <store pc> <kind>`: the first two columns are a
steps.txt line, the store pc is what GCN_STORE_HIST counts. Nothing from the game is
written into the repository; the list is regenerated from a local build.
"""
import glob
import os
import re
import sys

LINE = re.compile(r'/\* ([0-9A-F]{8}) ([0-9A-F]{8}) \*/(.*)')
GPR_W = re.compile(r'c->r\[(\d+)\] = ')
FPR_W = re.compile(r'c->f\[(\d+)\]\.d = |c->ps1\[(\d+)\] = ')

LOADS = {32: ('g', 4, 'lwz'), 33: ('g', 4, 'lwzu'), 34: ('g', 1, 'lbz'), 40: ('g', 2, 'lhz'),
         42: ('g', 2, 'lha'), 48: ('f', 4, 'lfs'), 49: ('f', 4, 'lfsu'), 50: ('f', 8, 'lfd'),
         51: ('f', 8, 'lfdu'), 56: ('f', 8, 'psq_l')}
STORES = {36: ('g', 4), 37: ('g', 4), 38: ('g', 1), 44: ('g', 2), 52: ('f', 4), 53: ('f', 4),
          54: ('f', 8), 55: ('f', 8), 60: ('f', 8)}
FLOAT_A = {18: 'fdiv', 20: 'fsub', 21: 'fadd', 25: 'fmul', 28: 'fmsub', 29: 'fmadd',
           30: 'fnmsub', 31: 'fnmadd'}
# Operand letters translate_step accepts per form (ppc.py), state operand first.
FLOAT_OK = {21: 'AB', 20: 'A', 25: 'AC', 18: 'A', 29: 'BAC', 28: 'A'}


def fields(w):
    return w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31


def disp(w, op):
    if op in (56, 60):
        d = w & 0xFFF
        return d - 0x1000 if d & 0x800 else d
    d = w & 0xFFFF
    return d - 0x10000 if d & 0x8000 else d


def arith(w):
    """(kind, dest, {letter: (file, reg)}) for a scalable arithmetic instruction, else None."""
    op, d, a, b, c = fields(w)
    if op == 14 and a != 0:
        return ('addi', ('g', d), {'A': ('g', a)}, disp(w, op))
    if op == 31:
        xo9 = (w >> 1) & 0x1FF
        if xo9 == 266:
            return ('add', ('g', d), {'A': ('g', a), 'B': ('g', b)}, None)
        if xo9 == 40:
            return ('subf', ('g', d), {'B': ('g', b)}, None)
    if op in (59, 63, 4):
        xo5 = (w >> 1) & 31
        if xo5 in FLOAT_OK and (op != 4 or xo5 in (18, 20, 21, 25, 28, 29)):
            regs = {'A': ('f', a), 'B': ('f', b), 'C': ('f', c)}
            name = ('ps_' if op == 4 else '') + FLOAT_A[xo5] + ('s' if op == 59 else '')
            return (name, ('f', d), {k: regs[k] for k in FLOAT_OK[xo5]}, xo5)
    return None


def writes(text):
    return ({('g', int(m)) for m in GPR_W.findall(text)} |
            {('f', int(a or b)) for a, b in FPR_W.findall(text)})


def scan(path, out):
    block = []  # (pc, word, text) since the last label or call
    for raw in open(path, encoding='utf-8', errors='replace'):
        s = raw.strip()
        if s.endswith(':') and s.startswith('L_') or s.startswith('void ') or s == '}':
            block = []
            continue
        m = LINE.search(raw)
        if not m:
            continue
        pc, w, text = int(m.group(1), 16), int(m.group(2), 16), m.group(3)
        op, sreg, base, _, _ = fields(w)
        if op in STORES and base != 0:
            check(block, pc, w, out)
        if 'c->lr = ' in text or 'call_indirect' in text:
            block = []
            continue
        block.append((pc, w, text))


def check(block, spc, sw, out):
    sop, sreg, base, _, _ = fields(sw)
    sfile = STORES[sop][0]
    soff = disp(sw, sop)
    # The instruction that last wrote the stored register, with base unchanged since.
    for i in range(len(block) - 1, -1, -1):
        pc, w, text = block[i]
        wr = writes(text)
        if ('g', base) in wr:
            return
        if (sfile, sreg) not in wr:
            continue
        ar = arith(w)
        if not ar:
            return
        kind, dest, ops, extra = ar
        if dest != (sfile, sreg):
            return
        if kind == 'addi' and extra not in (1, -1):
            return
        for letter, reg in ops.items():
            if find_load(block[:i], reg, base, soff, sop):
                out.append(f'{pc:08X} {letter} {spc:08X} {kind}')
                return
        return


def find_load(block, reg, base, off, sop):
    for j in range(len(block) - 1, -1, -1):
        pc, w, text = block[j]
        wr = writes(text)
        op, d, a, _, _ = fields(w)
        if op in LOADS and (LOADS[op][0], d) == reg:
            return a == base and disp(w, op) == off and LOADS[op][1] == STORES[sop][1]
        if reg in wr or ('g', base) in wr:
            return False
    return False


def main():
    gen = sys.argv[1] if len(sys.argv) > 1 else os.path.join('build', 'gen')
    out = []
    for path in sorted(glob.glob(os.path.join(gen, '*.c'))):
        scan(path, out)
    for line in out:
        print(line)
    print(f'{len(out)} candidates', file=sys.stderr)


if __name__ == '__main__':
    main()
