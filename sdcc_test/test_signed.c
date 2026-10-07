/* signed compares in every shape the code generator emits; the chip and
   the simulator must agree (values printed as hex so a failure shows what
   the chip computed) */
#include "harness.h"

unsigned char lt16(int a, int b) { return a < b; }
unsigned char gt16(int a, int b) { return a > b; }
unsigned char lt8(signed char a, signed char b) { return a < b; }
unsigned char lt8b(signed char a, signed char b) { if (a < b) return 1; return 0; }
unsigned char neg8(signed char x) { return x < 0; }
unsigned char neg16(int x) { return x < 0; }
unsigned char lt16b(int a, int b) { if (a < b) return 1; return 0; }
unsigned char lt32(long a, long b) { return a < b; }
int g16 = -2461;

/* __divsint / __modsint in divu.s */
volatile int m16 = -32767 - 1, p16 = 300, n16 = -300, s7 = 7, s7n = -7, one = 1;

/* the byte helpers in divu.s: a char result keeps the operands bytes, with
   int's rules for a mixed pair (__divuschar: a signed, __divsuchar: b) */
static signed char sdiv(signed char a, signed char b) { return a / b; }
static signed char smod(signed char a, signed char b) { return a % b; }
static signed char sudiv(signed char a, unsigned char b) { return a / b; }
static signed char sumod(signed char a, unsigned char b) { return a % b; }
static signed char usdiv(unsigned char a, signed char b) { return a / b; }
static signed char usmod(unsigned char a, signed char b) { return a % b; }
static unsigned char udiv(unsigned char a, unsigned char b) { return a / b; }
static signed char sdiv7(signed char a) { return a / 7; }
static signed char smod7(signed char a) { return a % 7; }
/* a literal that fits a byte of the other signedness: the mixed helpers
   (lisaNarrowByteDiv in SDCCopt.c) */
static signed char sd200(signed char a) { return a / 200; }
static signed char sm200(signed char a) { return a % 200; }
static signed char udn3(unsigned char a) { return a / -3; }
static signed char umn3(unsigned char a) { return a % -3; }
static signed char udn1(unsigned char a) { return a / -1; }
/* the quotient of two chars is an int: 128 and -255 happen */
volatile signed char c128 = -128, cm1 = -1;
volatile unsigned char u255 = 255;

int main(void)
{
  CHECK(1, lt16(-300, 200) == 1);
  CHECK(2, lt16(200, -300) == 0);
  CHECK(3, lt16(-300, -301) == 0);
  CHECK(4, lt16(-301, -300) == 1);
  CHECK(5, lt16(5, 300) == 1);
  CHECK(6, lt16(300, 5) == 0);
  CHECK(7, gt16(200, -300) == 1);
  CHECK(8, lt8(-3, 2) == 1 && lt8(2, -3) == 0);
  CHECK(9, lt8b(-3, 2) == 1 && lt8b(2, -3) == 0 && lt8b(-3, -3) == 0);
  CHECK(10, neg8(-1) == 1 && neg8(1) == 0 && neg8(0) == 0);
  CHECK(11, neg16(-1) == 1 && neg16(1) == 0 && neg16(-256) == 1 && neg16(255) == 0);
  CHECK(12, lt16b(-300, 200) == 1 && lt16b(200, -300) == 0);
  CHECK(13, lt32(-100000L, 1L) == 1 && lt32(1L, -100000L) == 0);
  CHECK(14, (g16 / 10) == -246);
  CHECK(15, (g16 % 10) == -1);
  CHECK(16, m16 / one == -32767 - 1 && m16 % one == 0 && m16 / 2 == -16384);
  CHECK(17, m16 % s7 == -1 && m16 / s7 == -4681);
  CHECK(18, n16 / s7n == 42 && n16 % s7n == -6 && p16 / s7n == -42 && p16 % s7n == 6);
  CHECK(19, one / m16 == 0 && one % m16 == 1 && n16 % s7 == -6 && n16 / s7 == -42);
  CHECK(20, 65535u / (unsigned)s7 == 9362 && 65535u % (unsigned)s7 == 1 && (unsigned)n16 / (unsigned)one == 65236u);
  CHECK(21, sdiv(100, -4) == -25 && smod(100, -4) == 0 && sdiv(-100, 3) == -33 && smod(-100, 3) == -1);
  CHECK(22, sdiv(-128, -1) == -128 && smod(-128, -1) == 0 && sdiv(-128, 1) == -128 && sdiv(127, -128) == 0 && smod(127, -128) == 127);
  CHECK(23, sdiv(-128, 7) == -18 && smod(-128, 7) == -2 && sdiv(7, -128) == 0 && smod(-7, 100) == -7);
  CHECK(24, sudiv(-100, 200) == 0 && sumod(-100, 200) == -100 && sudiv(-100, 3) == -33 && sumod(-100, 3) == -1 && sudiv(100, 200) == 0);
  CHECK(25, usdiv(200, -4) == -50 && usmod(200, -4) == 0 && usdiv(255, -1) == 1 && usmod(255, -1) == 0 && usdiv(200, 3) == 66 && usmod(200, -3) == 2);
  CHECK(26, udiv(255, 1) == 255 && udiv(200, 7) == 28 && sdiv7(-100) == -14 && smod7(-100) == -2 && sdiv7(127) == 18 && smod7(-128) == -2);
  CHECK(27, c128 / cm1 == 128 && c128 % cm1 == 0 && u255 / cm1 == -255 && u255 % cm1 == 0 && cm1 / u255 == 0 && cm1 % u255 == -1);
  CHECK(28, sd200(-100) == 0 && sm200(-100) == -100 && sd200(127) == 0 && sm200(127) == 127);
  CHECK(29, udn3(200) == -66 && umn3(200) == 2 && udn3(255) == -85 && umn3(255) == 0 && udn1(255) == 1 && udn1(100) == -100);
  puts("lt16 "); puthex(lt16(-300, 200)); puthex(lt16(200, -300)); puthex(lt16(-300, -301)); putc('\n');
  puts("neg16 "); puthex(neg16(-1)); puthex(neg16(1)); putc('\n');
  puts("div "); puthex((unsigned)(g16 / 10)); puthex((unsigned)(g16 % 10)); putc('\n');
  DONE();
  return 0;
}
