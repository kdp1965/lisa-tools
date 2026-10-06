# lisa-tools — software for the LISA 8-bit core

Tools for the LISA (Little ISA) microcontroller on the TT07 Tiny Tapeout
chip (`tt07-um-lisa-ttlc`).

| Directory | What it is |
|---|---|
| `lisa_as`, `lisa_ld`, `lisa_ar` | the original assembler, linker and archiver (`make` in each) |
| `lisa_cc` | the original hand-written C compiler |
| `lisa_bringup` | the bring-up firmware (`?` prints a banner) built with the tools above |
| `lisa_pydb` | Python debugger / flash programmer for the TT07 demo board |
| `lisa_sim` | C++ instruction-set simulator; models the TT07 silicon exactly, including its ALU flag corner cases |
| `sdcc_test` | self-checking C programs for the SDCC port (`make check` runs them on `lisa_sim`) |
| `sdcc-lisa` | **not in this repo** — the SDCC port (`sdcc -mlisa`, `sdaslisa`, `sdldlisa`), see below |

## The SDCC port

`sdcc-lisa/` is ignored here because it is a fork of SDCC with its own
history. Clone it into place:

```bash
git clone -b lisa git@github.com:kdp1965/sdcc-lisa.git sdcc-lisa
```

Then follow `sdcc-lisa/README-lisa.md` for the build (it needs a few
macOS-specific configure settings) and `sdcc-lisa/src/lisa/PLAN.md` for the
ABI and code-generator design. `sdcc_test/Makefile` expects the compiler at
`sdcc-lisa/build/bin/sdcc`.

## Running C on the chip

`sdcc -mlisa -o prog.ihx prog.c` writes Intel HEX, which LISA Commander
(`tt07-um-lisa-ttlc/lisa-test`) loads directly; its hardware test runs a
suite on the board with `node hw_test.mjs connect ihx=../../lisa-tools/sdcc_test/test_core.ihx disconnect`.
