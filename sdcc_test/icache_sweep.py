#!/usr/bin/env python3
# icache_sweep.py [config name ...] - run the suite on lisa_sim with several
# instruction cache configurations (--icache, lisa_sim/icache.h) and tabulate
# misses per 1000 instructions and clocks per instruction against the chip's
# single 4-word line.  make icache.  REV3=1: the *_rev3.ihx images (make
# check REV3=1) on lisa_sim --rev3, the programs that exist for it.
import subprocess, re, sys, os
SIM = '../lisa_sim/lisa_sim'
PROGS = ('test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress '
         'test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f '
         'test_ptroff test_rot test_index test_vshift hello test_printf test_owl').split()
FLAGS = {'test_irq': ['--fixed-irq']}
CONFIGS = [('chip 1x4', '1x4'), ('4x4 LRU', '4x4'), ('16x4 direct', '16x4,direct'), ('4x16 LRU', '4x16'),
           ('stream', '0,stream'), ('stream+1x4', '1x4,stream'), ('stream+4x4', '4x4,stream'),
           ('stream+4x16', '4x16,stream'), ('stream+16x4 dir', '16x4,direct,stream'),
           ('stream+4x16 r40', '4x16,40,stream'),
           ('window 8', '0x8,window'), ('window 12', '0x12,window'), ('window 16', '0x16,window'),
           ('window 32', '0x32,window')]
REV3 = os.environ.get('REV3') == '1'
if REV3:
    PROGS = [p for p in PROGS if os.path.exists(p + '_rev3.ihx')]
if len(sys.argv) > 1: CONFIGS = [c for c in CONFIGS if c[0] in sys.argv[1:]] or CONFIGS
res = {}
for p in PROGS:
    for name, cfg in CONFIGS:
        out = subprocess.run([SIM, '-b', f'--icache={cfg}', '-c', '2000000'] + (['--rev3'] if REV3 else []) +
                             FLAGS.get(p, []) + [p + ('_rev3.ihx' if REV3 else '.ihx')],
                             capture_output=True, text=True).stderr
        m = re.search(r'instructions (\d+)\s+fetches (\d+)\s+hits ([\d.]+)%\s+misses (\d+) \(([\d.]+) per 1000 instructions: (\d+) streaming', out)
        c = re.search(r'clocks (\d+): (\d+) executing \+ (\d+) stalled \((\d+)%\), ([\d.]+) per instruction', out)
        if not m or not c:
            print('no report for', p, cfg, out[-200:]); continue
        res[(p, name)] = dict(insts=int(m.group(1)), misses=int(m.group(4)), mpk=float(m.group(5)), seq=int(m.group(6)),
                              clocks=int(c.group(1)), exe=int(c.group(2)), stall=int(c.group(4)), cpi=float(c.group(5)))
names = [n for n, _ in CONFIGS]
print('misses per 1000 instructions / clocks per instruction (speedup over the chip\'s cache)')
print('%-14s %9s' % ('program', 'instrs') + ''.join('%22s' % n for n in names))
print('%-14s %9s' % ('', '') + ''.join('%22s' % 'miss/1k  cpi' for n in names))
tot = {n: [0, 0, 0, 0] for n in names}
for p in PROGS:
    if (p, names[0]) not in res: continue
    r0 = res[(p, names[0])]
    row = '%-14s %9d' % (p, r0['insts'])
    for n in names:
        r = res[(p, n)]
        tot[n][0] += r['clocks']; tot[n][1] += r['insts']; tot[n][2] += r['misses']; tot[n][3] += r['seq']
        row += '%8.1f %5.1f (%4.2fx)' % (r['mpk'], r['cpi'], r0['cpi'] / r['cpi'])
    print(row)
print('%-14s %9s' % ('all', '') + ''.join('%8.1f %5.1f (%4.2fx)' % (1000.0 * tot[n][2] / tot[n][1], tot[n][0] / tot[n][1], (tot[names[0]][0] / tot[names[0]][1]) / (tot[n][0] / tot[n][1])) for n in names))
print('streaming share of the misses over all programs: ' + '  '.join('%s %.0f%%' % (n, 100.0 * tot[n][3] / max(1, tot[n][2])) for n in names))
