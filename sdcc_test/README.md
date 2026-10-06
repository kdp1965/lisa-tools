# SDCC LISA port tests

Test programs for `sdcc -mlisa` (the SDCC port in `../sdcc-lisa`,
see its `README-lisa.md` and `src/lisa/PLAN.md`).

    make check          # build and run everything on ../lisa_sim/lisa_sim
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
* `test_signed.c`, `test_strs.c` — signed compares and negation, the
  string library; `test_regress.c` — the bugs SDCC's own regression suite
  found in the port (struct arguments, bit fields, setjmp, narrow-operand
  subtraction, shift-count slot sharing, literal pointers, ...);
  `test_pfu.c` — printf/sprintf `%u`; `test_loop.c` — the sdcc_hello main
  loop; `test_aluprobe.c`, `test_sltprobe.c` — print the raw C flag after
  add/adc/sub/cmp (built, not checked).
* `test_bigframe.c` — a 900-byte local array (bytes beyond the 511-byte
  `n(sp)` reach go through an IX window); more than the chip's 128 bytes of
  RAM, so simulator only (built with `--stack-loc 0x7fff`).
* `hello.c`, `test_printf.c`, `test_owl.c` — output compared with
  `expected/*.txt` (`test_printf` exercises the library printf: %d %u %x
  %ld %c %s, widths).
* `run_sim.sh prog.ihx [cycles]` — prints only the UART output of a batch
  simulator run.

The same `.ihx` files load on the TT07 board with LISA Commander (Intel
HEX, byte address = 2 x PC); `lisa-test/test/hw_test.mjs connect
ihx=<file> disconnect` runs one on the chip and waits for the verdict.
Everything except `test_bigframe` fits the 128-byte RAM; the big suite
(`sdcc-lisa/build/support/regression`, `make test-lisa`) runs on the
simulator with 32K.
