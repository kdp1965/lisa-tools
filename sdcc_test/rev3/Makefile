# The third chip's tests (ttsky26d REV3.md): assembly until sdcc --rev3 exists.
#   make          build test_rev3.ihx
#   make check    run it on lisa_sim --rev3 (expects ALL PASSED)
SDAS   ?= ../../sdcc-lisa/build/bin/sdaslisa
SDLD   ?= ../../sdcc-lisa/build/bin/sdldlisa
SIM    ?= ../../lisa_sim/lisa_sim
TESTS   = test_rev3

all: $(TESTS:%=%.ihx)

%.rel: %.s
	$(SDAS) -plosgff -o $@ $<

%.ihx: %.rel
	$(SDLD) -i $<

check: all
	@for t in $(TESTS); do printf "%-14s " $$t; $(SIM) --rev3 -b -c 300000 $$t.ihx 2>&1 | grep -E "ALL PASSED|SOME FAILED|FAIL" | tr '\n' ' '; echo; done

clean:
	rm -f *.rel *.ihx *.lst *.sym *.map *.rst *.hlr

.PHONY: all check clean
