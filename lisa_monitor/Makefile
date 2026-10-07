# lisa_monitor - an interactive monitor on LISA's UART1, built with the
# SDCC port for the TT07 chip's data cache (32K SRAM emulated by the
# demo board's RP2040, see sdcc-lisa/README-lisa.md).
#
#   make            monitor.ihx
#   make board      program the board, set the cache up, start it and leave
#                   the console attached: then  socat -,raw,echo=0 TCP:localhost:5555
#   make sim        the monitor on lisa_sim with a pty: connect to the other pty

SDCC   ?= ../sdcc-lisa/build/bin/sdcc
SIM    ?= ../lisa_sim/lisa_sim
NODE   ?= node
TEST   ?= ../../tt07-um-lisa-ttlc/lisa-test/test

all: monitor.ihx

monitor.ihx: monitor.c $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa/lisa.lib)
	$(SDCC) -mlisa --tt07-cache -o $@ $<

board: monitor.ihx
	cd $(TEST) && $(NODE) hw_test.mjs connect monitor=$(CURDIR)/monitor.ihx

sim: monitor.ihx
	@echo "run:  socat -d -d pty,raw,echo=0 pty,raw,echo=0   then  $(SIM) -p <ptyA> monitor.ihx"
	@echo "and   socat -,raw,echo=0 <ptyB>   in another terminal; press Enter for the banner"

clean:
	rm -f *.ihx *.asm *.lst *.rel *.sym *.map *.lk *.rst *.cdb *.noi
