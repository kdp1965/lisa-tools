# lisa_ide

A curses debugger for the LISA chip on the Tiny Tapeout demo board, on top
of Ken's TUI framework (`tui/`, shared with `merlin/` and `pico16/`): source
tabs, a command window with history and tab completion, a register watch
window, a terminal tab for LISA's UART.

```sh
make                       # ./lisa (macOS: the system ncurses, or /opt/homebrew's if installed)
./lisa
lisa> connect /dev/cu.usbmodem1101        # the board's REPL: selects tt_um_lisa, starts uartPass, finds the debugger
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
`setup defaults` goes back to the defaults. Turning the data cache on
needs a RAM on its chip select - on the demo board the RP2040's SPI RAM
emulation, which the LISA Commander starts.

## Source level (`src/LisaCdb.cxx`)

`debug prog.ihx` loads the image (the code words are then read from it,
not over the UART) and `prog.cdb` beside it, opens each source the `.cdb`
names in a tab (the arrow marks the PC's line, `*` a breakpoint's), and
warns if the flash does not hold the image.  The reader is
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
