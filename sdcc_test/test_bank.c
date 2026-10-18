/* test_bank.c - __banked functions in code banks 2 and 3 (ttsky26d RESPIN
 * 22, lisa_isa.md "Bank registers"): calls through ___sdcc_bcall from the
 * common area and from one bank into another, byte / int / long results,
 * parameters in A and on the stack, 3-byte banked function pointers,
 * recursion across banks.  Three files: this one (common), test_bank2.c
 * (#pragma bank 2) and test_bank3.c (#pragma bank 3), linked with
 * -Wl-bBANK2=0x10000 -Wl-bBANK3=0x18000 (byte n * 0x8000 = word 0x4000 of
 * bank n).  --rev2 only: the banks are the second chip's. */
#include "harness.h"
#include "test_bank.h"

__sfr __at(0x215) CODE_BANK;

static unsigned char common_add(unsigned char x) { return x + 10; }

void main(void)
{
  bfn_t f;

  CHECK(1, CODE_BANK == 1);
  CHECK(2, b2_byte(5) == 7);                        /* A in, A out */
  CHECK(3, CODE_BANK == 1);                         /* the bank restored */
  CHECK(4, b2_sum(1000, 2345) == 3345);             /* the stack in, IX out */
  CHECK(5, b2_mul(100000L, 3L) == 300000L);         /* the return slot; __mullong (common) from bank 2 */
  CHECK(6, b3_byte(1) == 6);                        /* bank 3 calls bank 2: 1 + 2 + 3 */
  CHECK(7, CODE_BANK == 1);
  f = b2_byte;
  CHECK(8, f(10) == 12);                            /* a 3-byte function pointer */
  f = b3_byte;
  CHECK(9, f(10) == 15);
  CHECK(10, b2_apply(b3_byte, 1) == 8);             /* a banked pointer called inside a bank: (1 + 2 + 3) + 2 */
  CHECK(11, b2_apply_common(common_add, 1) == 13);  /* a common function called from a bank */
  CHECK(12, b3_depth(6) == 6);                      /* b3 -> b2 -> b3 ... */
  CHECK(13, b3_sum3(1, 300, 2) == 303);
  CHECK(14, CODE_BANK == 1);
  DONE();
}
