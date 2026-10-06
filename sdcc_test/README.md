# SDCC LISA port tests

Test programs for `sdcc -mlisa` (the SDCC port in `~/projects/lisa/sdcc-lisa`,
see its `README-lisa.md` and `src/lisa/PLAN.md`).

    make check          # build and run everything on ../../simulator/lisa_sim
    make                # just build the .ihx files
    make asm            # generated assembly for inspection
    SDCC=... SIM=... make check     # other compiler / simulator binaries

* `harness.h` — UART1 output (`0x210`/`0x211`), `CHECK(n, cond)` prints
  `ok NNNN` / `FAIL NNNN`, `DONE()` prints `ALL PASSED` / `SOME FAILED`.
* `test_core.c` — 25 cases: 8/16/32-bit add/sub, shifts, signed and
  unsigned compares, mul, div/mod (library), arrays in RAM and in code
  space, pointers, generic pointers, function pointers, switch, globals
  with initializers, compound assignment.
* `test_negate.c`, `test_union.c`, `test_varargs.c`, `test_digits.c` —
  regression cases from printf debugging (unary minus, unions, va_arg,
  shift/subtract digit extraction).
* `hello.c`, `test_printf.c` — output compared with `expected/*.txt`
  (`test_printf` exercises the library printf: %d %u %x %ld %c %s, widths).
* `run_sim.sh prog.ihx [cycles]` — prints only the UART output of a batch
  simulator run.

The same `.ihx` files load on the TT07 board with LISA Commander once it
reads Intel HEX (byte address = 2 x PC).
