# Tests for the SDCC LISA port (sdcc -mlisa).
#
#   make            build every test program (.ihx)
#   make check      build and run them on the C++ simulator
#   make asm        keep the generated assembly (*.asm) for inspection
#
# Self-checking tests print "ok NNNN" / "FAIL NNNN" per case and end with
# "ALL PASSED"; output tests are compared with expected/<name>.txt.

SDCC   ?= ../../sdcc-lisa/build/bin/sdcc
SIM    ?= ../../simulator/lisa_sim
CYCLES ?= 2000000
CFLAGS ?= -mlisa

SELFCHECK = test_core test_negate test_union test_varargs test_digits
OUTPUT    = hello test_printf
TESTS     = $(SELFCHECK) $(OUTPUT)

all: $(TESTS:%=%.ihx)

%.ihx: %.c harness.h
	$(SDCC) $(CFLAGS) -o $@ $<

check: all
	@pass=0; fail=0; \
	for t in $(SELFCHECK); do \
	  out=$$(SIM=$(SIM) ./run_sim.sh $$t.ihx $(CYCLES)); \
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
