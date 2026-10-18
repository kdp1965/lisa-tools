/* test_huge.c - the huge model (sdcc -mlisa --rev2 --model-huge): every
 * function is a banked call through ___sdcc_bcall unless __nonbanked, and
 * a source goes to code bank n with --codeseg BANKn (no #pragma bank, no
 * __banked in the sources - the way an OS is built: README-lisa.md "Code
 * banks").  Three files: this one (common), test_huge5.c (bank 5) and
 * test_huge6.c (bank 6), linked with -Wl-bBANK5=0x28000 -Wl-bBANK6=0x30000.
 * --rev2 only: the banks are the second chip's. */
#include "harness.h"
#include "test_huge.h"

__sfr __at(0x215) CODE_BANK;

unsigned char common_add(unsigned char x) { return x + 10; }
unsigned char common_add_nb(unsigned char x) __nonbanked { return x + 20; }
static unsigned char local_add(unsigned char x) { return x + 30; }

void main(void)
{
  fn_t f;
  nbfn_t g;
  volatile unsigned char s3 = sizeof(fn_t), s2 = sizeof(nbfn_t);

  CHECK(1, s3 == 3 && s2 == 2);
  CHECK(2, CODE_BANK == 1);
  CHECK(3, h5_byte(1) == 6);                        /* A in, A out */
  CHECK(4, CODE_BANK == 1);                         /* the bank restored */
  CHECK(5, h5_sum(1000, 2345) == 3345);             /* the stack in, IX out */
  CHECK(6, h5_mul(100000L, 3L) == 300000L);         /* the return slot; __mullong (the small-model library) from bank 5 */
  CHECK(7, h6_byte(1) == 12);                       /* bank 6 calls bank 5 */
  CHECK(8, local_add(1) == 31);                     /* a common static: banked too (b = 1) */
  CHECK(9, common_add_nb(1) == 21);                 /* a plain call */
  f = h5_byte;
  CHECK(10, f(10) == 15);                           /* a 3-byte pointer to a bank */
  f = common_add;
  CHECK(11, f(10) == 20);                           /* ... and to common code */
  f = local_add;
  CHECK(12, f(10) == 40);
  g = common_add_nb;
  CHECK(13, g(10) == 30);                           /* a 2-byte pointer */
  CHECK(14, h5_apply(h6_byte, 1) == 17);            /* a pointer called inside a bank: 12 + 5 */
  CHECK(15, h5_apply(common_add, 1) == 16);
  CHECK(16, h5_apply_nb(common_add_nb, 1) == 26);
  CHECK(17, h6_depth(6) == 6);                      /* h6 -> h5 -> h6 ... */
  CHECK(18, h6_sum3(1, 300, 2) == 333);             /* 11 + 300 + 22 */
  CHECK(19, CODE_BANK == 1);
  DONE();
}
