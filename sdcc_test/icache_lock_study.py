#!/usr/bin/env python3
# icache_lock_study.py - a locked instruction cache next to the respin's
# streaming window (Ken, 2026-10-24: a second RAM32 loaded once and locked).
# A RAM32 is 32 x 32 bits = 64 instruction words: lines of W words with an
# address tag each.  Per program (each firmware locks its own hot code): run
# the rev-3 suite on lisa_sim --rev3 --icache=0x8,window with
# --icache-profile, sum the stall clocks per aligned line, lock the hottest
# lines that fit (--icache-pin: fetches there cost nothing and leave the
# window alone), and run again; a second pass re-profiles with the first
# choice pinned and refills (the stall moves when a line is locked).
import os, re, subprocess, collections
SIM = '../lisa_sim/lisa_sim'
PROGS = ('test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress '
         'test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f '
         'test_ptroff test_rot test_index test_vshift hello test_printf test_owl').split()
PROGS = [p for p in PROGS if os.path.exists(p + '_rev3.ihx')]
FLAGS = {'test_irq': ['--fixed-irq']}
TMP = '/tmp/icache_lock_study'
os.makedirs(TMP, exist_ok=True)
CONFIGS = [(64, 8), (64, 4), (64, 16), (128, 8)]       # words, line size


def run(p, extra):
    out = subprocess.run([SIM, '-b', '--rev3', '--icache=0x8,window', '-c', '2000000'] + FLAGS.get(p, []) + extra +
                         [p + '_rev3.ihx'], capture_output=True, text=True).stderr
    c = re.search(r'clocks (\d+): (\d+) executing \+ (\d+) stalled', out)
    i = re.search(r'instructions (\d+)', out)
    return int(c.group(1)), int(i.group(1))


def profile(p, pins):
    pin = f'{TMP}/{p}.pin'
    write_pins(pin, pins)
    prof = f'{TMP}/{p}.prof'
    run(p, [f'--icache-pin={pin}', f'--icache-profile={prof}'])
    return [(int(a, 16), int(c)) for a, _, c in (l.split() for l in open(prof))]


def write_pins(path, lines_w):
    with open(path, 'w') as f:
        for base, w in lines_w:
            f.write('%x %x\n' % (base, base + w - 1))


def choose(p, words, w):
    n = words // w
    chosen = []
    for _ in range(2):                    # pass 1, then refill after re-profiling
        cost = collections.Counter()
        for pc, clocks in profile(p, [(b, w) for b in chosen]):
            cost[pc - pc % w] += clocks
        for base, _ in cost.most_common():
            if len(chosen) >= n:
                break
            if base not in chosen:
                chosen.append(base)
        if len(chosen) >= n:
            break
    return chosen[:n]


base = {p: run(p, []) for p in PROGS}
tot0 = sum(c for c, _ in base.values())
ins = sum(i for _, i in base.values())
print('the 8-word window alone: %.2f clocks per instruction over the suite' % (tot0 / ins))
for words, w in CONFIGS:
    tot = 0
    rows = []
    for p in PROGS:
        lines = choose(p, words, w)
        pin = f'{TMP}/{p}.pin'
        write_pins(pin, [(b, w) for b in lines])
        c, i = run(p, [f'--icache-pin={pin}'])
        tot += c
        rows.append((p, base[p][0] / base[p][1], c / i))
    print('\n%d words locked as %d lines of %d: the suite %.2f -> %.2f clocks per instruction (%.2fx)'
          % (words, words // w, w, tot0 / ins, tot / ins, tot0 / tot))
    for p, a, b in sorted(rows, key=lambda r: r[2] / r[1])[:8]:
        print('  %-14s %5.1f -> %5.1f (%.2fx)' % (p, a, b, a / b))
