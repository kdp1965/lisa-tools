#!/usr/bin/env python3
# icache_pin_study.py [budget words ...] - what a locked on-chip store of
# library helpers would buy next to the respin's streaming window (Ken,
# 2026-10-24: a second RAM32, 32 x 32 bits = 64 instruction words, loaded
# once with the helper functions and locked).  The rev-3 suite (make check
# REV3=1) on lisa_sim --rev3 --icache=0x8,window: profile the restarts per
# word (--icache-profile), charge each one's stall clocks to the library
# function holding it (the .map: a C: symbol with no module), rank the
# helpers by clocks saved per word, pin the best that fit the budget
# (--icache-pin, each program's own addresses for them) and run again.
import os, re, subprocess, sys, collections
SIM = '../lisa_sim/lisa_sim'
BUDGETS = [int(b) for b in sys.argv[1:]] or [64, 128]
PROGS = ('test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress '
         'test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f '
         'test_ptroff test_rot test_index test_vshift hello test_printf test_owl').split()
PROGS = [p for p in PROGS if os.path.exists(p + '_rev3.ihx')]
FLAGS = {'test_irq': ['--fixed-irq']}
TMP = '/tmp/icache_pin_study'
os.makedirs(TMP, exist_ok=True)

def run(p, extra):
    out = subprocess.run([SIM, '-b', '--rev3', '--icache=0x8,window', '-c', '2000000'] + FLAGS.get(p, []) + extra +
                         [p + '_rev3.ihx'], capture_output=True, text=True).stderr
    c = re.search(r'clocks (\d+): (\d+) executing \+ (\d+) stalled', out)
    i = re.search(r'instructions (\d+)', out)
    return int(c.group(1)), int(i.group(1))

LIB = '../sdcc-lisa/build/device/lib/build/lisa-rev3'


def functions(p):
    """the library modules' code: name -> (first word, last word, library?).
    The CODE area starts at s_CODE and holds the modules in link order (the
    map's Files Linked, then Libraries Linked), each its .rel's CODE size:
    a module's static functions have no symbol in the map, so a symbol's
    span to the next one is not its size."""
    t = open(p + '_rev3.map').read()
    a = int(re.search(r'C:\s+([0-9A-F]+)\s+s_CODE', t).group(1), 16)
    files = re.findall(r'\n(\S+\.rel)\s+\[ (\S+) \]', t.split('Files Linked')[1].split('Libraries Linked')[0])
    libs = re.findall(r'\[ (\S+) \]', t.split('Libraries Linked')[1].split('User Base')[0])
    out = {}
    for rel, lib in [(f, False) for f, m in files] + [(LIB + '/' + m + '.rel', True) for m in libs]:
        sz = 0
        for line in open(rel):
            m = re.match(r'A CODE size ([0-9A-F]+)', line)
            if m:
                sz = int(m.group(1), 16)
        if sz:
            out[os.path.basename(rel)[:-4]] = (a // 2, (a + sz) // 2 - 1, lib)
        a += sz
    return out


stall = collections.Counter()       # helper: stall clocks over the suite
size = {}
base = {}
fns = {}
for p in PROGS:
    prof = f'{TMP}/{p}.prof'
    base[p] = run(p, [f'--icache-profile={prof}'])
    fns[p] = functions(p)
    ranges = sorted((a, e, n) for n, (a, e, lib) in fns[p].items() if lib)
    for line in open(prof):
        pc, _, clocks = line.split()
        pc, clocks = int(pc, 16), int(clocks)
        for a, e, n in ranges:
            if a <= pc <= e:
                stall[n] += clocks
                size[n] = max(size.get(n, 0), e - a + 1)
                break

rank = sorted(stall, key=lambda n: -stall[n] / size[n])
print('library modules by stall clocks saved per word (the suite, the 8-word window):')
print('%-24s %6s %10s %8s' % ('module', 'words', 'clocks', 'per word'))
for n in rank[:20]:
    print('%-24s %6d %10d %8.0f' % (n, size[n], stall[n], stall[n] / size[n]))

tot0 = sum(c for c, _ in base.values()); ins = sum(i for _, i in base.values())
print('\nthe suite: %.2f clocks per instruction with the 8-word window alone' % (tot0 / ins))
for budget in BUDGETS:
    chosen, used = [], 0
    for n in rank:
        if used + size[n] <= budget:
            chosen.append(n); used += size[n]
    tot = 0
    rows = []
    for p in PROGS:
        pin = f'{TMP}/{p}.pin'
        with open(pin, 'w') as f:
            for n in chosen:
                if n in fns[p]:
                    a, e, _ = fns[p][n]
                    f.write('%x %x\n' % (a, e))
        c, i = run(p, [f'--icache-pin={pin}'])
        tot += c
        rows.append((p, base[p][0] / base[p][1], c / i))
    print('\n%d words (%d used): %s' % (budget, used, ', '.join(chosen)))
    print('  the suite %.2f -> %.2f clocks per instruction (%.2fx)' % (tot0 / ins, tot / ins, tot0 / tot))
    for p, a, b in sorted(rows, key=lambda r: r[2] / r[1])[:6]:
        print('  %-14s %5.1f -> %5.1f (%.2fx)' % (p, a, b, a / b))
