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

SELFCHECK = test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f test_ptroff test_rot test_index test_vshift
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
# Probes print the raw C flag after add/adc/sub/cmp (silicon vs. the ISA doc);
# they are built but not checked.
TESTS     = $(SELFCHECK) $(OUTPUT) $(if $(REV2),$(REV2CACHE),$(PROBES) $(CACHE))

all: $(TESTS:%=%$(SFX).ihx)

# a new compiler or library rebuilds everything (a stale .ihx once made
# 15 suites "fail")
DEPS = harness.h $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa/lisa.lib) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-cache/lisa.lib) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-rev2/lisa.lib)

%$(SFX).ihx: %.c $(DEPS)
	$(SDCC) $(CFLAGS) -o $@ $<

# simulator only: a frame bigger than the chip's RAM, stack at the top of 32K
test_bigframe$(SFX).ihx: test_bigframe.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x7fff -o $@ $<

# its sp cases put a byte 0x80 above the stack, which must stay below 0x8000
test_cachermw8$(SFX).ihx: test_cachermw8.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x3fff -o $@ $<

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
