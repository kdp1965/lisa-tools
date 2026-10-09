# lisa_monitor - an interactive monitor on LISA's UART1, built with the
# SDCC port for the TT07 chip's data cache (32K SRAM emulated by the
# demo board's RP2040, see sdcc-lisa/README-lisa.md).
#
#   make            monitor.ihx, and monitor_small.ihx for the 128-byte RAM
#   make board      program the board, set the cache up, start it and leave
#                   the console attached: then  socat -,raw,echo=0 TCP:localhost:5555
#   make board-small  the same with monitor_small.ihx (no SPI RAM, no cache)
#   make sim        the monitor on lisa_sim with a pty: connect to the other pty
#   make commander  copy the image into the LISA Commander web app's firmware
#                   list (lisa-test/firmware/lisa_monitor.ihx, assets.js rebuilt)

SDCC   ?= ../sdcc-lisa/build/bin/sdcc
SIM    ?= ../lisa_sim/lisa_sim
NODE   ?= node
TEST   ?= ../../tt07-um-lisa-ttlc/lisa-test/test

all: monitor.ihx monitor_small.ihx

monitor.ihx: monitor.c $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa-cache/lisa.lib)
	$(SDCC) -mlisa --tt07-cache -o $@ $<

# the 128-byte RAM, nothing special on the board: calc in bf16 only
monitor_small.ihx: monitor.c $(wildcard $(SDCC)) $(wildcard $(dir $(SDCC))../device/lib/build/lisa/lisa.lib)
	$(SDCC) -mlisa -DSMALL -o $@ $<

board: monitor.ihx
	cd $(TEST) && $(NODE) hw_test.mjs connect monitor=$(CURDIR)/monitor.ihx

board-small: monitor_small.ihx
	cd $(TEST) && $(NODE) hw_test.mjs connect run=$(CURDIR)/monitor_small.ihx

sim: monitor.ihx
	@echo "run:  socat -d -d pty,raw,echo=0 pty,raw,echo=0   then  $(SIM) -p <ptyA> monitor.ihx"
	@echo "and   socat -,raw,echo=0 <ptyB>   in another terminal; press Enter for the banner"

commander: monitor.ihx monitor_small.ihx
	cp monitor.ihx $(TEST)/../firmware/lisa_monitor.ihx
	cp monitor_small.ihx $(TEST)/../firmware/lisa_monitor_small.ihx
	python3 $(TEST)/../web/build_assets.py

clean:
	rm -f *.ihx *.asm *.lst *.rel *.sym *.map *.lk *.rst *.cdb *.noi
