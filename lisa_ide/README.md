# lisa_ide

A curses debugger for the LISA chip on the Tiny Tapeout demo board, on top
of Ken's TUI framework (`tui/`, shared with `merlin/` and `pico16/`): source
tabs, a command window with history and tab completion, a register watch
window, a terminal tab for LISA's UART.

```sh
make                       # ./lisa (macOS: the system ncurses, or /opt/homebrew's if installed)
./lisa
lisa> connect /dev/cu.usbmodem1101        # the board's REPL: selects tt_um_lisa, starts uartPass, finds the debugger
lisa> load ../sdcc_test/test_core.ihx     # program + verify the flash through LISA, then as debug (Tab completes)
lisa> debug ../sdcc_test/test_core.ihx    # the image and its .cdb (sdcc --debug), the sources in tabs
lisa> break main                          # or file:line, a line of the active tab, *addr
lisa> run
lisa> next / into / finish                # by breakpoints at the line's exits, not by stepping
lisa> print s   print arr[2]   print p->x   print *p   print &g
lisa> locals / bt / where
```

Commands (`help` lists them, `help <command>` explains one): `connect`, `open` (a `.S`, `.lst` listing, hex
object or C file in a tab), `attach` (a listing/object tab as the running
program), `break`/`delete`/`br`, `run`/`halt`/`stop`, `step` (one
instruction, the chip's step bit), `set reg=val`, `read`/`write` (debugger
registers), `reset`, `term`, `ls`, `sp`, `pc`, and the source level: `debug`,
`next`/`n`, `into`, `finish`, `print`/`p`, `locals`, `bt`, `where`, and
`setup` (the SETUP tab).

## Programming (`src/LisaFlash.cxx`)

`load <prog.ihx> [base]` programs the flash through LISA's debugger, as the
LISA Commander's "via LISA" does: the core halted, the 64 KB blocks the
image touches erased, then one word per exchange (`w20<word>` with the
flash status `r22` in the same round trip, ~2.6 ms through the RP2040's
pass-through: test_index's 1208 words take 6.6 s), a read-back verify, the
instruction cache invalidated and the core reset to PC 0; then it does a
`debug` of the image. The base defaults to the program fetch base (the
SETUP tab); another base is programmed but not run from. `verify
<prog.ihx> [base]` compares only. Ctrl-C stops a load (the flash is then
partly programmed). Tab after `load`, `debug` or `verify` completes
directories and `.ihx`/`.hex` files.

## The SETUP tab (`src/LisaSetup.cxx`)

`setup` opens a form over the debugger's configuration registers
(`debug_regs.v`), read from the chip when connected:

* program fetch (LISA1), the data cache (LISA2) and the TTLC: chip select
  and base address (bytes; the registers 0x12/0x13/0x1f hold them >> 8);
  the debugger's flash port: chip select (0x14–0x16)
* per chip select: flash or RAM, SPI or QSPI, 24- or 16-bit addresses,
  dummy read cycles (0x17, 0x18)
* the SPI mode, SCLK divider and CE delay (0x1e); the data cache on/off and
  its map (0x1d); the TTLC shift clock and the pin muxes (0x1c, 0x1b)

Up/Down/Tab move, Left/Right/Space change a choice, hex digits and
Backspace edit a number, Enter presses a button, Esc cancels. The bottom
shows the register values the form will write and anything inconsistent
(no flash for program fetch, the data cache on a flash, ...). **OK**
writes the registers, reads each back, and keeps the settings: `connect`
applies them from then on, and they are saved in `.tui_prefs`. **Cancel**
drops the edits; **Read from chip** and **Defaults** reload the form.
`setup apply` writes the kept settings again (after a project reset),
`setup defaults` goes back to the defaults.

**The data cache on the demo board** needs the RP2040's SPI RAM emulation
(the `lisa_spi_ram` MicroPython build; the LISA Commander offers to flash
it). `spiram on` sets the Commander's known-good settings (CE1 on
`uio[4]`, CS1 a 16-bit SPI RAM, SPI mode 3, CE delay 127, SCLK /1), starts
the emulation with a line of Python through `uartPass.py`'s sideband,
checks the RAM with a pattern through the debugger, and only then turns
the cache on; `spiram off` turns it off. The settings are kept, so
`connect` does the same again; the SETUP tab's cache on/off goes through
the same steps with its own settings. If the RAM fails the check the
cache stays off. With the cache on CS1, CE1 must be on `uio[4]` (uio mux
bits 1:0 = 3): otherwise the emulator sees its select active and drives
MISO, which the flash shares, and every flash read returns 0. Programs
for the cache are built with `sdcc --tt07-cache`.

## Source level (`src/LisaCdb.cxx`)

`debug prog.ihx` loads the image (the code words are then read from it,
not over the UART) and `prog.cdb` beside it, opens each source the `.cdb`
names in a tab (the arrow marks the PC's line, `*` a breakpoint's), opens
`prog.rst` - the linked assembly listing - as a tab with the arrow too, and
warns if the flash does not hold the image.

SDCC's listings use byte addresses (its linker works in bytes; a LISA word
is two bytes), so PC = listing address / 2, and the bytes are printed in
memory order (opcode 0xF610 appears as `10 F6`). `prog.rst` has the
linked addresses; `prog.lst` is the assembler's output before linking,
whose addresses are offsets within each area of the module (the map says
where the linker put the area: the monitor's CODE at 0xEC, so its `.lst`
offset 0x70 is PC 0xAE). `open` understands both, and says so for a `.lst`.  The reader is
`../lisa_cdb` (`cdb.h`, `image.h`), shared with `lisa_sim`.

The chip has four hardware breakpoints and no shadow call stack, and a
debugger round trip costs ~30 ms, so:

* `next`/`into` do not step: the exits of the current line are computed
  from the image (`Cdb::line_exits`: the line starts reachable without
  crossing another, calls stepped over - a callee's first line for `into` -
  the function's return), the breakpoints are planted there and the core
  runs.  A stop at a return address is mid-statement in the caller, so it
  runs on to the caller's next line; a stop one recursion deeper runs on.
  More than four exits (rare): instructions are stepped, by breakpoints on
  every address the instruction can continue at, as the Commander does.
* Frames come from the registers and the stack (`Cdb::unwind`): a
  function's prologue (`sra`, `ads #-n`) gives its SP displacement, so at a
  statement boundary entry SP = SP + displacement; the return address is RA
  until saved, then the two bytes at the entry SP; the caller's SP is the
  entry SP plus the callee's arguments and the slot for a result wider than
  two bytes or a struct.  Exact where `next`/`into`/`finish`/breakpoints
  stop (statement boundaries); after `halt` mid-statement the caller
  frames may be off by the arguments being pushed.
* `print` reads data through IX (16 bytes per request), code-space
  constants from the image; peripheral registers are not readable this way.
* `run` blocks like gdb's `continue`: the program gets the UART (its output
  appears as `[LISA] ...`), the core is checked between, and it ends at a
  breakpoint, a `brk` or Ctrl-C.
* **TT07 breakpoint hazard.** A store's RAM write completes in the cycle
  after it; a hardware breakpoint on the next instruction stops the core in
  that cycle, when the data address is IX and the data the debugger's last
  byte. The store is lost and RAM[IX] overwritten (measured on the chip).
  So no stop is ever planted right after a store (`Cdb::is_store`):
  `break`, `next`/`into`/`finish` and instruction steps plant the first
  safe address after it (`Cdb::safe_stop`) and report the stop as the line
  meant, with SP corrected for anything pushed on the way. An asynchronous
  `halt`/Ctrl-C can still land there; nothing to be done about that one.
* `connect` only takes a character device.

`lisa_sim` has the same commands and checks this machinery against its
shadow call stack (`bt chip`, `exits`, `nexthw`/`intohw`/`finishhw`).
