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

SELFCHECK = test_core test_negate test_union test_varargs test_digits test_signed test_strs test_regress test_pfu test_loop test_bigframe test_irq test_irq2 test_divsh test_bf16 test_bf16f
# Probes of the TT07 interrupt hazards (built, not checked: they fail on the
# chip and on the simulator's TT07 model, pass with --fixed-irq)
PROBES    = test_aluprobe test_sltprobe test_irqhaz
OUTPUT    = hello test_printf test_owl
# Probes print the raw C flag after add/adc/sub/cmp (silicon vs. the ISA doc);
# they are built but not checked.
TESTS     = $(SELFCHECK) $(OUTPUT) $(PROBES)

all: $(TESTS:%=%.ihx)

%.ihx: %.c harness.h
	$(SDCC) $(CFLAGS) -o $@ $<

# simulator only: a frame bigger than the chip's RAM, stack at the top of 32K
test_bigframe.ihx: test_bigframe.c harness.h
	$(SDCC) $(CFLAGS) --stack-loc 0x7fff -o $@ $<

# the C type float on the bfloat16 unit
test_bf16f.ihx: test_bf16f.c harness.h
	$(SDCC) $(CFLAGS) --bf16-float -o $@ $<

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
