/* test_divc.c - the second chip's divider gives both results and chains
 * (ttsky26d RESPIN 27): div leaves the quotient in RA (its low byte in A) and
 * the remainder in IX, rem the other way round (TT07 and rc10 cleared IX);
 * divc divides {IX, A} by the divisor of the last division - a byte of
 * quotient to A and RA, the remainder to IX - so a long division of any
 * width by a 16-bit divisor is one divc per byte, the remainder carried in
 * IX.  Signed mode (amode[1]) as C: the remainder takes the dividend's sign.
 * The chains are checked against the C library's 64-bit division (the
 * bit-serial loop, independent of the divider) and then, against constants,
 * the library itself on the step (divull.s).  sdcc --rev2 only. */
#include "harness.h"

volatile unsigned int g_dd, g_dv;         /* the dividend and divisor in */
volatile unsigned int g_ra, g_ix;         /* RA and IX out, all 16 bits */
volatile unsigned char g_a;               /* A out */
volatile unsigned char g_digits[8];       /* the chain's dividend, most significant byte first */
volatile unsigned char g_q[8];            /* ... and its quotient bytes */

/* g_dd / g_dv with lddiv and the two-word div: RA, IX, A to the globals */
static void do_div(void) __naked
{
  __asm
	sra
	lda	_g_dd+1
	push	a			; the dividend high byte at 1(sp) after the next push
	lda	_g_dv+1
	push	a			; the divisor high byte at 0(sp)
	lda	_g_dd
	lddiv	1(sp)			; RA[7:0] <= 1(sp), IX[7:0] <= A
	lda	_g_dv
	div	0, 0(sp)		; {0(sp), A} the divisor
	sta	_g_a
	txa
	sta	_g_ix
	txau
	sta	_g_ix+1			; {ix_cond, ix[14:8]}: the whole high byte
	xchg	ra
	txa
	sta	_g_ra
	txau
	sta	_g_ra+1
	ads	#2
	lra
	ret
  __endasm;
}

/* the same with rem */
static void do_rem(void) __naked
{
  __asm
	sra
	lda	_g_dd+1
	push	a
	lda	_g_dv+1
	push	a
	lda	_g_dd
	lddiv	1(sp)
	lda	_g_dv
	rem	0, 0(sp)
	sta	_g_a
	txa
	sta	_g_ix
	txau
	sta	_g_ix+1
	xchg	ra
	txa
	sta	_g_ra
	txau
	sta	_g_ra+1
	ads	#2
	lra
	ret
  __endasm;
}

/* signed: amode[1] set around do_div / do_rem (amode 1, shifts through C, is
 * what compiled code runs with - back to it, not to 0) */
static void do_sdiv(void)
{
  __asm
	amode	3
  __endasm;
  do_div();
  __asm
	amode	1
  __endasm;
}

static void do_srem(void)
{
  __asm
	amode	3
  __endasm;
  do_rem();
  __asm
	amode	1
  __endasm;
}

/* g_digits[0..7] / g_dv by bytes: a div of 0 by g_dv sets the divisor (and
 * IX = 0, the remainder so far), then one divc per byte; the quotient bytes
 * to g_q, the remainder to g_ix */
static void do_chain(void) __naked
{
  __asm
	sra
	lda	_g_dv+1
	push	a
	ldi	#0
	tax
	taxu
	lda	_g_dv
	div	2, 0(sp)		; 0 / {0(sp), A}: the divisor latched, IX = 0
	lda	_g_digits+0
	divc
	sta	_g_q+0
	lda	_g_digits+1
	divc
	sta	_g_q+1
	lda	_g_digits+2
	divc
	sta	_g_q+2
	lda	_g_digits+3
	divc
	sta	_g_q+3
	lda	_g_digits+4
	divc
	sta	_g_q+4
	lda	_g_digits+5
	divc
	sta	_g_q+5
	lda	_g_digits+6
	divc
	sta	_g_q+6
	lda	_g_digits+7
	divc
	sta	_g_q+7
	txa
	sta	_g_ix
	txau
	sta	_g_ix+1
	ads	#1
	lra
	ret
  __endasm;
}

/* one divc on its own: IX = g_ix, A = g_a, the divisor as latched */
static void do_step(void) __naked
{
  __asm
	sra
	lda	_g_ix
	tax
	lda	_g_ix+1
	taxu
	lda	_g_a
	divc
	sta	_g_a
	txa
	sta	_g_ix
	txau
	sta	_g_ix+1
	xchg	ra
	txa
	sta	_g_ra
	txau
	sta	_g_ra+1
	lra
	ret
  __endasm;
}

static volatile unsigned long long g_v;
static volatile long long g_sv;

/* the chain of g_v / d against the C library */
static unsigned char chain_ok(unsigned long long v, unsigned int d)
{
  unsigned long long q;
  unsigned char i;

  for (i = 0; i < 8; i++)
    g_digits[i] = (unsigned char)(v >> (8 * (7 - i)));
  g_dv = d;
  do_chain();
  q = 0;
  for (i = 0; i < 8; i++)
    q = (q << 8) | g_q[i];
  return q == v / d && g_ix == (unsigned int)(v % d);
}

int main(void)
{
  /* both results of a division */
  g_dd = 50000; g_dv = 7; do_div();
  CHECK(1, g_a == (7142 & 0xff) && g_ra == 7142 && g_ix == 6);
  g_dd = 50000; g_dv = 7; do_rem();
  CHECK(2, g_a == 6 && g_ra == 6 && g_ix == 7142);
  g_dd = 0xffff; g_dv = 1; do_div();              /* bit 15 of a quotient, by one */
  CHECK(3, g_ra == 0xffff && g_ix == 0 && g_a == 0xff);
  g_dd = 0xfffe; g_dv = 0xffff; do_div();         /* bit 15 of a remainder */
  CHECK(4, g_ra == 0 && g_ix == 0xfffe);
  g_dd = 1234; g_dv = 0; do_div();                /* by zero: all ones, the dividend */
  CHECK(5, g_ra == 0xffff && g_ix == 1234);
  g_dd = 1234; g_dv = 0; do_rem();
  CHECK(6, g_ra == 1234 && g_ix == 0xffff);
  g_dd = 300; g_dv = 300; do_div();
  CHECK(7, g_ra == 1 && g_ix == 0);
  g_dd = 299; g_dv = 300; do_div();
  CHECK(8, g_ra == 0 && g_ix == 299);

  /* signed */
  g_dd = (unsigned int)-7; g_dv = 2; do_sdiv();   /* -7 / 2 = -3 r -1 */
  CHECK(9, g_ra == (unsigned int)-3 && g_ix == (unsigned int)-1);
  g_dd = 7; g_dv = (unsigned int)-2; do_sdiv();   /* 7 / -2 = -3 r 1 */
  CHECK(10, g_ra == (unsigned int)-3 && g_ix == 1);
  g_dd = (unsigned int)-7; g_dv = (unsigned int)-2; do_srem();   /* -7 % -2 = -1, q 3 */
  CHECK(11, g_ra == (unsigned int)-1 && g_ix == 3);
  g_dd = 0x8000; g_dv = (unsigned int)-1; do_sdiv();            /* -32768 / -1 */
  CHECK(12, g_ra == 0x8000 && g_ix == 0);
  g_dd = (unsigned int)-7; g_dv = 0; do_sdiv();                 /* by zero, signed: all ones, the dividend */
  CHECK(13, g_ra == 0xffff && g_ix == (unsigned int)-7);

  /* the chained step: 0x12345678 / 1000 = 0x4a90b r 896, by bytes */
  g_digits[0] = 0; g_digits[1] = 0; g_digits[2] = 0; g_digits[3] = 0;
  g_digits[4] = 0x12; g_digits[5] = 0x34; g_digits[6] = 0x56; g_digits[7] = 0x78;
  g_dv = 1000; do_chain();
  CHECK(14, g_q[4] == 0x00 && g_q[5] == 0x04 && g_q[6] == 0xa9 && g_q[7] == 0x0b);
  CHECK(15, g_q[0] == 0 && g_q[1] == 0 && g_q[2] == 0 && g_q[3] == 0 && g_ix == 896);

  /* one step with a remainder carried in: {0x294, 0x56} / 1000 = 169 r 46 */
  g_dd = 0; g_dv = 1000; do_div();                /* the divisor latched */
  g_ix = 0x294; g_a = 0x56; do_step();
  CHECK(16, g_a == 169 && g_ra == 169 && g_ix == 46);
  /* ... after a rem (the divisor is kept from it too), the dividend low byte zero */
  g_dd = 5; g_dv = 3; do_rem();
  g_ix = 2; g_a = 0; do_step();                   /* 0x200 / 3 = 170 r 2 */
  CHECK(17, g_a == 170 && g_ix == 2);

  /* 64-bit chains against the library */
  g_v = 1234567890123456789ULL;
  CHECK(18, chain_ok(g_v, 10));
  CHECK(19, chain_ok(g_v, 1000));
  CHECK(20, chain_ok(g_v, 65535));
  CHECK(21, chain_ok(g_v, 1));
  CHECK(22, chain_ok(g_v, 0x8000));
  CHECK(23, chain_ok(0xffffffffffffffffULL, 0xffff));
  CHECK(24, chain_ok(0xffffffffffffffffULL, 3));
  CHECK(25, chain_ok(0x00000000ffffffffULL, 7));
  CHECK(26, chain_ok(0x8000000000000000ULL, 0x8001));

  /* the library's 64-bit division and remainder (divull.s: the chain for a
   * divisor below 65536, the bit-serial loop above) against constants */
  CHECK(27, g_v / 10 == 0x01b69b4ba630f34eULL && g_v % 10 == 9);
  CHECK(28, g_v / 1000 == 0x000462d53c8abac0ULL && g_v % 1000 == 0x315);
  CHECK(29, g_v / 65535 == 0x000011222216a000ULL && g_v % 65535 == 0x2115);
  CHECK(30, g_v / 0x8000 == 0x0000224421e8fbd3ULL && g_v % 0x8000 == 0x115);
  CHECK(31, g_v / 65536 == 0x0000112210f47de9ULL && g_v % 65536 == 0x8115);
  CHECK(32, g_v / 1000000 == 0x0000011f71fb04cbULL && g_v % 1000000 == 0x6f855);
  g_sv = -(long long)g_v;                         /* signed: _divslonglong / _modslonglong on it */
  CHECK(33, g_sv / 10 == -(long long)0x01b69b4ba630f34eLL);
  CHECK(34, g_sv % 10 == -9);

  DONE();
  return 0;
}
