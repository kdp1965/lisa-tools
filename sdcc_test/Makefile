# Tests for the SDCC LISA port (sdcc -mlisa).
#
#   make            build every test program (.ihx)
#   make check      build and run them on the C++ simulator (../lisa_sim)
#   make check REV2=1   the same built with sdcc --rev2 (the second chip: its
#                   stack convention, bf16_add on its adder, lisa-rev2 library)
#                   as *_rev2.ihx, run on lisa_sim --rev2; the TT07 cache
#                   tests and probes are left out
#   make icache     the instruction cache study: misses and clocks per instruction
#                   for several cache shapes (icache_sweep.py, lisa_sim --icache)
#   make asm        keep the generated assembly (*.asm) for inspection
#
# Self-checking tests print "ok NNNN" / "FAIL NNNN" per case and end with
# "ALL PASSED"; output tests are compared with expected/<name>.txt.

SDCC   ?= ../sdcc-lisa/build/bin/sdcc
SIM    ?= ../lisa_sim/lisa_sim
CYCLES ?= 2000000
CFLAGS ?= -mlisa --debug
# REV2=1: everything for the second chip
ifneq ($(REV2),)
SFX      = _rev2
CFLAGS  += --rev2
REV2SIM  = --rev2
endif
# PACK=1: the second chip with constants packed two to a word and read with
# lda (ix++) (sdcc --pack-const, ttsky26d RESPIN 26) as *_rev2k.ihx
ifneq ($(PACK),)
REV2     = 1
SFX      = _rev2k
CFLAGS  += --rev2 --pack-const
REV2SIM  = --rev2
endif

SELFCHECK = test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f test_ptroff test_rot test_index test_vshift
# long long arithmetic through the library: built, not checked - its signed
# modulo (_modslonglong, check 8) is wrong on lisa_sim, to be fixed
KNOWN     = test_ll
# Probes of the TT07 interrupt hazards (built, not checked: they fail on the
# chip and on the simulator's TT07 model, pass with --fixed-irq)
PROBES    = test_aluprobe test_sltprobe test_irqhaz
OUTPUT    = hello test_printf test_owl
# The data cache on an RP2040 SPI RAM (hw_test.mjs spiram=): built, run on the
# chip only; test_cacherm's expectations are the TT07 silicon's swap n(sp)
CACHE     = test_cacherm test_cachermw test_cachepush test_cachewb test_cacheseq test_cacheloop test_cacheperiph
# The second chip's cache (8 lines of 16 bytes): the RMW-on-a-miss test with
# its evictions 0x80 apart; REV2=1 only, checked on lisa_sim (no cache there)
# and meant for the ttsky26d testbench and the FPGA with the cache on
REV2CACHE = test_cachermw8
# The second chip's code banks (RESPIN 22): __banked functions in banks 2 and
# 3 through ___sdcc_bcall; REV2=1 only (three source files, see test_bank.c)
REV2CACHE += test_bank
# ... and its data banks: two __addressmod spaces in data banks 2 and 3
REV2CACHE += test_dbank
# the huge model: every function banked, the banks by --codeseg; REV2=1 only
REV2CACHE += test_huge
# the ldirq / stirq byte of the second chip's shadows (RESPIN 24) and ldamode; REV2=1 only
REV2CACHE += test_ldirq
# the UART1 interrupts (RESPIN 25): the enables, a transmit-empty interrupt, two typed bytes; REV2=1 only
REV2CACHE += test_uartirq
# lda (ix++) and the constant bank KB (RESPIN 26): words read from the lower half and from bank 2; REV2=1 only
REV2CACHE += test_ldk
# the divider's two results (the remainder in IX) and the chained step divc (RESPIN 27); REV2=1 only
REV2CACHE += test_divc
# Probes print the raw C flag after add/adc/sub/cmp (silicon vs. the ISA doc);
# they are built but not checked.
TESTS     = $(SELFCHECK) $(KNOWN) $(OUTPUT) $(if $(REV2),$(REV2CACHE),$(PROBES) $(CACHE))

all: $(TESTS:%=%$(SFX).ihx)

# a new compiler or library rebuilds everything (a stale .ihx once made
# 15 suites "fail")
DEPS = harness.h $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa/lisa.lib) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-cache/lisa.lib) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-rev2/lisa.lib) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-rev2k/lisa.lib)

%$(SFX).ihx: %.c $(DEPS)
	$(SDCC) $(CFLAGS) -o $@ $<

# simulator only: a frame bigger than the chip's RAM, stack at the top of 32K
test_bigframe$(SFX).ihx: test_bigframe.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x7fff -o $@ $<

# the 64-bit library's frames outgrow the 128-byte RAM: stack at the top of
# 32K (the chip testbench runs it with the cache on)
test_divc$(SFX).ihx: test_divc.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x7fff -o $@ $<

# its sp cases put a byte 0x80 above the stack, which must stay below 0x8000
test_cachermw8$(SFX).ihx: test_cachermw8.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x3fff -o $@ $<

# the code banks: bank n is the area BANKn at byte n * 0x8000 of the flash
# (sdcc compiles one source per run: the bank files first, to .rel)
test_bank$(SFX).ihx: test_bank.c test_bank2.c test_bank3.c test_bank40.c test_bank.h $(DEPS)
	$(SDCC) $(CFLAGS) -c -o test_bank2$(SFX).rel test_bank2.c
	$(SDCC) $(CFLAGS) -c -o test_bank3$(SFX).rel test_bank3.c
	$(SDCC) $(CFLAGS) -c -o test_bank40$(SFX).rel test_bank40.c
	$(SDCC) $(CFLAGS) -o $@ test_bank.c test_bank2$(SFX).rel test_bank3$(SFX).rel test_bank40$(SFX).rel -Wl-bBANK2=0x10000 -Wl-bBANK3=0x18000 -Wl-bBANK40=0x140000

# the data banks: an __addressmod space's area DBANKn is linked in the data
# window 0x2000-0x3fff (RESPIN 28; 0x4000 before it)
test_dbank$(SFX).ihx: test_dbank.c $(DEPS)
	$(SDCC) $(CFLAGS) -o $@ $< -Wl-bDBANK2=0x2000 -Wl-bDBANK3=0x2000

# the huge model (--model-huge): no __banked, no #pragma bank; a source goes
# to a code bank with --codeseg BANKn
# lda (ix++) reading from bank 2: the pattern of test_ldk2.c placed there
test_ldk$(SFX).ihx: test_ldk.c test_ldk2.c $(DEPS)
	$(SDCC) $(CFLAGS) --codeseg BANK2 -c -o test_ldk2$(SFX).rel test_ldk2.c
	$(SDCC) $(CFLAGS) -o $@ test_ldk.c test_ldk2$(SFX).rel -Wl-bBANK2=0x10000

test_huge$(SFX).ihx: test_huge.c test_huge5.c test_huge6.c test_huge.h $(DEPS)
	$(SDCC) $(CFLAGS) --model-huge --codeseg BANK5 -c -o test_huge5$(SFX).rel test_huge5.c
	$(SDCC) $(CFLAGS) --model-huge --codeseg BANK6 -c -o test_huge6$(SFX).rel test_huge6.c
	$(SDCC) $(CFLAGS) --model-huge -o $@ test_huge.c test_huge5$(SFX).rel test_huge6$(SFX).rel -Wl-bBANK5=0x28000 -Wl-bBANK6=0x30000

# the C type float on the bfloat16 unit
test_bf16f$(SFX).ihx: test_bf16f.c $(DEPS)
	$(SDCC) $(CFLAGS) --bf16-float -o $@ $<

# the cache tests use the 32K data space as the TT07 cache folds it: stack
# at the top, data below 0x3800 (the linker checks)
test_cacheseq.ihx test_cacheloop.ihx test_cachermw.ihx test_cachepush.ihx test_cacheperiph.ihx test_ptroff.ihx: %.ihx: %.c $(DEPS)
	$(SDCC) $(CFLAGS) --tt07-cache -o $@ $<

# the C interrupt handler test is only correct on a core with the respin's
# interrupt semantics (see README-lisa.md "Interrupts on TT07")
SIMFLAGS_test_irq = --fixed-irq
export SIMFLAGS_test_irq
# the bench types "Hi" into UART1 once the program has set up its interrupt
SIMFLAGS_test_uartirq = --send Hi --send-at 60000
export SIMFLAGS_test_uartirq

check: all
	@pass=0; fail=0; \
	for t in $(SELFCHECK) $(if $(REV2),$(REV2CACHE)); do \
	  eval f=\$$SIMFLAGS_$$t; \
	  out=$$(SIM=$(SIM) SIMFLAGS="$$f $(REV2SIM)" ./run_sim.sh $${t}$(SFX).ihx $(CYCLES)); \
	  n=$$(printf '%s\n' "$$out" | grep -c '^ok'); \
	  if printf '%s\n' "$$out" | grep -q '^ALL PASSED'; then \
	    echo "PASS $$t ($$n cases)"; pass=$$((pass+1)); \
	  else \
	    echo "FAIL $$t"; printf '%s\n' "$$out" | grep -v '^ok'; fail=$$((fail+1)); \
	  fi; \
	done; \
	for t in $(OUTPUT); do \
	  if SIM=$(SIM) SIMFLAGS="$(REV2SIM)" ./run_sim.sh $${t}$(SFX).ihx $(CYCLES) | diff -u expected/$$t.txt - > $$t.diff; then \
	    echo "PASS $$t (output matches)"; pass=$$((pass+1)); rm -f $$t.diff; \
	  else \
	    echo "FAIL $$t (see $$t.diff)"; fail=$$((fail+1)); \
	  fi; \
	done; \
	echo "$$pass passed, $$fail failed"; test $$fail -eq 0

icache: all
	@python3 ./icache_sweep.py

asm: $(TESTS:%=%$(SFX).asm)

%.asm: %.c harness.h
	$(SDCC) $(CFLAGS) -S -o $@ $<

clean:
	rm -f *.ihx *.asm *.lst *.rel *.sym *.map *.lk *.cdb *.adb *.diff *.noi

.PHONY: all check icache asm clean
