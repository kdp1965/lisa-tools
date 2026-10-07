# Tests for the SDCC LISA port (sdcc -mlisa).
#
#   make            build every test program (.ihx)
#   make check      build and run them on the C++ simulator (../lisa_sim)
#   make asm        keep the generated assembly (*.asm) for inspection
#
# Self-checking tests print "ok NNNN" / "FAIL NNNN" per case and end with
# "ALL PASSED"; output tests are compared with expected/<name>.txt.

SDCC   ?= ../sdcc-lisa/build/bin/sdcc
SIM    ?= ../lisa_sim/lisa_sim
CYCLES ?= 2000000
CFLAGS ?= -mlisa

SELFCHECK = test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_divl test_bf16 test_bf16f test_ptroff test_rot
# Probes of the TT07 interrupt hazards (built, not checked: they fail on the
# chip and on the simulator's TT07 model, pass with --fixed-irq)
PROBES    = test_aluprobe test_sltprobe test_irqhaz
OUTPUT    = hello test_printf test_owl
# The data cache on an RP2040 SPI RAM (hw_test.mjs spiram=): built, run on the
# chip only; test_cacherm's expectations are the TT07 silicon's swap n(sp)
CACHE     = test_cacherm test_cachermw test_cachepush test_cachewb test_cacheseq test_cacheloop
# Probes print the raw C flag after add/adc/sub/cmp (silicon vs. the ISA doc);
# they are built but not checked.
TESTS     = $(SELFCHECK) $(OUTPUT) $(PROBES) $(CACHE)

all: $(TESTS:%=%.ihx)

# a new compiler or library rebuilds everything (a stale .ihx once made
# 15 suites "fail")
DEPS = harness.h $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa/lisa.lib)

%.ihx: %.c $(DEPS)
	$(SDCC) $(CFLAGS) -o $@ $<

# simulator only: a frame bigger than the chip's RAM, stack at the top of 32K
test_bigframe.ihx: test_bigframe.c $(DEPS)
	$(SDCC) $(CFLAGS) --stack-loc 0x7fff -o $@ $<

# the C type float on the bfloat16 unit
test_bf16f.ihx: test_bf16f.c $(DEPS)
	$(SDCC) $(CFLAGS) --bf16-float -o $@ $<

# the cache tests use the 32K data space as the TT07 cache folds it: stack
# at the top, data below 0x3800 (the linker checks)
test_cacheseq.ihx test_cacheloop.ihx test_cachermw.ihx test_cachepush.ihx test_ptroff.ihx: %.ihx: %.c $(DEPS)
	$(SDCC) $(CFLAGS) --tt07-cache -o $@ $<

# the C interrupt handler test is only correct on a core with the respin's
# interrupt semantics (see README-lisa.md "Interrupts on TT07")
SIMFLAGS_test_irq = --fixed-irq
export SIMFLAGS_test_irq

check: all
	@pass=0; fail=0; \
	for t in $(SELFCHECK); do \
	  eval f=\$$SIMFLAGS_$$t; \
	  out=$$(SIM=$(SIM) SIMFLAGS="$$f" ./run_sim.sh $$t.ihx $(CYCLES)); \
	  n=$$(printf '%s\n' "$$out" | grep -c '^ok'); \
	  if printf '%s\n' "$$out" | grep -q '^ALL PASSED'; then \
	    echo "PASS $$t ($$n cases)"; pass=$$((pass+1)); \
	  else \
	    echo "FAIL $$t"; printf '%s\n' "$$out" | grep -v '^ok'; fail=$$((fail+1)); \
	  fi; \
	done; \
	for t in $(OUTPUT); do \
	  if SIM=$(SIM) ./run_sim.sh $$t.ihx $(CYCLES) | diff -u expected/$$t.txt - > $$t.diff; then \
	    echo "PASS $$t (output matches)"; pass=$$((pass+1)); rm -f $$t.diff; \
	  else \
	    echo "FAIL $$t (see $$t.diff)"; fail=$$((fail+1)); \
	  fi; \
	done; \
	echo "$$pass passed, $$fail failed"; test $$fail -eq 0

asm: $(TESTS:%=%.asm)

%.asm: %.c harness.h
	$(SDCC) $(CFLAGS) -S -o $@ $<

clean:
	rm -f *.ihx *.asm *.lst *.rel *.sym *.map *.lk *.cdb *.diff *.noi

.PHONY: all check asm clean
