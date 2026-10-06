/*
 * test_divsh.c - the hardware divider (lddiv / div / rem, unsigned, the
 * signed helpers on top of it) and the 16-bit shifts (shl16 / shr16).
 * Volatile operands keep the compiler from folding anything.
 */
#include "harness.h"

volatile unsigned int ua = 50000, ub = 7, uc = 300, u1 = 1, u0 = 0, ud = 0x1234, ue = 0x9000, uf = 0xa000, ug = 0xfffe, uh = 0xffff;
volatile unsigned char ca = 200, cb = 7, cc = 13, cb1 = 1;
volatile int sa = -1234, sb = 100, sc = -7;
volatile signed char xa = -100, xb = 7, xc = -3;
volatile unsigned char n1 = 1, n3 = 3, n5 = 5, n9 = 9, n12 = 12, n0 = 0, n15 = 15;
volatile unsigned long la = 123456789UL, lb = 1000UL;

static unsigned int rsh(unsigned int v, unsigned char n) { return v >> n; }
static unsigned int lsh(unsigned int v, unsigned char n) { return v << n; }
static int srsh(int v, unsigned char n) { return v >> n; }

int main(void)
{
  unsigned int r;
  unsigned char c;
  int s;

  /* 16 / 16 */
  CHECK(1, ua / ub == 7142);
  CHECK(2, ua % ub == 6);
  CHECK(3, uc / ub == 42 && uc % ub == 6);
  CHECK(4, ua / u1 == 50000 && ua % u1 == 0);
  CHECK(5, ub / uc == 0 && ub % uc == 7);
  /* by a literal: 10 is not a power of two, 256 is handled as a shift */
  CHECK(6, ua / 10 == 5000 && ua % 10 == 0 && uc / 10 == 30 && uc % 10 == 0);
  CHECK(7, ua / 256 == 195 && ua % 256 == 80);
  /* 8 / 8, 16 / 8, 8 / 16 */
  CHECK(8, ca / cb == 28 && ca % cb == 4);
  CHECK(9, ca / cc == 15 && ca % cc == 5);
  CHECK(10, ua / cb == 7142 && ua % cb == 6);
  CHECK(11, ca / uc == 0 && ca % uc == 200);
  c = ca / cb;
  CHECK(12, c == 28);
  r = ca / cb;
  CHECK(13, r == 28);
  c = ua / ub;
  CHECK(14, c == (7142 & 0xff));
  r = ca % 3;
  CHECK(15, r == 2);
  /* signed: the library helpers on magnitudes */
  CHECK(16, sa / sb == -12 && sa % sb == -34);
  CHECK(17, sa / sc == 176 && sa % sc == -2);
  CHECK(18, sb / sc == -14 && sb % sc == 2);
  CHECK(19, xa / xb == -14 && xa % xb == -2);
  CHECK(20, xa / xc == 33 && xa % xc == -1);
  s = sa / sb;
  CHECK(21, s == -12);
  /* long: software, still */
  CHECK(22, la / lb == 123456UL && la % lb == 789UL);
  /* a chain: dividend from the previous quotient */
  r = ua / ub / cb;
  CHECK(23, r == 1020);
  r = (ua % uc) / cb;
  CHECK(24, r == 28);
  /* a 16-bit remainder, and the divider's divisor-1 case (its RA says 0
     for the quotient's high byte): variables, a literal, a chain */
  CHECK(49, ua % 0x1234 == 0xd48 && ua % ud == 0xd48);
  CHECK(50, ua / u1 == 50000 && uc / u1 == 300 && ca / u1 == 200);
  CHECK(51, ua / 1 == 50000 && ua % 1 == 0 && (ua / u1) / u1 == 50000);
  CHECK(52, (unsigned char)(ua / u1) == 0x50 && ua / cb1 == 50000);
  /* remainders of 0x8000 and more (RA holds 15 bits): big divisors */
  CHECK(53, ue % uf == 0x9000 && uf % ue == 0x1000 && ug % uh == 0xfffe);
  CHECK(54, ua % 0x9000 == 0x3350 && ug % 0x9000 == 0x6ffe && ue % 0x8001 == 0x0fff);
  CHECK(55, ue / uf == 0 && uf / ue == 1 && ug / uh == 0 && ug / 0x9000 == 1);

  /* 16-bit shifts by literals */
  CHECK(25, ua << 1 == 34464 && ua >> 1 == 25000);
  CHECK(26, ua << 3 == 6784 && ua >> 3 == 6250);
  CHECK(27, ua << 5 == 27136 && ua >> 5 == 1562);
  CHECK(28, ua << 7 == 43008 && ua >> 7 == 390);
  CHECK(29, ua << 8 == 20480 && ua >> 8 == 195);
  CHECK(30, ua << 9 == 40960 && ua >> 9 == 97);
  CHECK(31, ua << 12 == 0 && ua >> 12 == 12 && uc << 12 == 49152 && uc >> 12 == 0);
  CHECK(32, ua << 15 == 0 && ua >> 15 == 1);
  /* signed right shifts: the sign comes in */
  CHECK(33, sa >> 1 == -617 && sa >> 2 == -309 && sa >> 5 == -39);
  CHECK(34, sa >> 8 == -5 && sa >> 10 == -2 && sa >> 15 == -1);
  CHECK(35, sb >> 3 == 12 && sb >> 9 == 0 && sc >> 1 == -4 && sc >> 12 == -1);
  CHECK(36, sa << 2 == -4936 && sa << 9 == 23552);
  /* in place at the top of the frame */
  r = ua;
  r <<= 3;
  CHECK(37, r == 6784);
  r = ua;
  r >>= 5;
  CHECK(38, r == 1562);
  s = sa;
  s >>= 3;
  CHECK(39, s == -155);
  /* variable counts */
  CHECK(40, lsh(ua, n1) == 34464 && rsh(ua, n1) == 25000);
  CHECK(41, lsh(ua, n3) == 6784 && rsh(ua, n3) == 6250);
  CHECK(42, lsh(ua, n9) == 40960 && rsh(ua, n9) == 97);
  CHECK(43, lsh(ua, n0) == 50000 && rsh(ua, n0) == 50000);
  CHECK(44, lsh(ua, n15) == 0 && rsh(ua, n15) == 1);
  CHECK(45, srsh(sa, n1) == -617 && srsh(sa, n5) == -39 && srsh(sa, n12) == -1);
  CHECK(46, srsh(sb, n3) == 12 && srsh(sc, n0) == -7);
  CHECK(47, ua << n5 == 27136 && ua >> n5 == 1562);
  CHECK(48, sa >> n9 == -3);

  DONE();
  return 0;
}
