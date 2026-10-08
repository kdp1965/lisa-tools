/*
 * test_vshift.c - shifts by a variable count (gen.c genShift): a byte in A,
 * a word by shl16 / shr16, a long by whole bytes first and then bits;
 * counts of 0, 1, 3, 7, 8, 9, 15, 16, 20, 24 and 31, both directions,
 * signed and unsigned.
 */
#include "harness.h"

static volatile unsigned char n0 = 0, n1 = 1, n3 = 3, n7 = 7, n8 = 8, n9 = 9, n15 = 15, n16 = 16, n20 = 20, n24 = 24, n31 = 31;
static volatile unsigned char ub = 0xa5;
static volatile signed char sb = -100;              /* 0x9c */
static volatile unsigned int uw = 0xa5c3;
static volatile int sw = -12345;                    /* 0xcfc7 */
static volatile unsigned long ul = 0xa5c3f017UL;
static volatile long sl = -123456789L;              /* 0xf8a432eb */

int main(void)
{
  {
    unsigned char a = ub << n0, b = ub << n1, c = ub << n3, d = ub << n7;
    unsigned char e = ub >> n0, f = ub >> n1, g = ub >> n3, h = ub >> n7;
    CHECK(1, a == 0xa5 && b == 0x4a && c == 0x28 && d == 0x80);
    CHECK(2, e == 0xa5 && f == 0x52 && g == 0x14 && h == 0x01);
  }
  {
    signed char a = sb >> n0, b = sb >> n1, c = sb >> n3, d = sb >> n7, e = sb << n1;
    CHECK(3, a == -100 && b == -50 && c == -13 && d == -1 && (unsigned char)e == 0x38);
  }
  {
    unsigned int a = uw << n0, b = uw << n1, c = uw << n3, d = uw << n7, e = uw << n8, f = uw << n9, g = uw << n15;
    CHECK(4, a == 0xa5c3 && b == 0x4b86 && c == 0x2e18 && d == 0xe180);
    CHECK(5, e == 0xc300 && f == 0x8600 && g == 0x8000);
  }
  {
    unsigned int a = uw >> n0, b = uw >> n1, c = uw >> n3, d = uw >> n7, e = uw >> n8, f = uw >> n9, g = uw >> n15;
    CHECK(6, a == 0xa5c3 && b == 0x52e1 && c == 0x14b8 && d == 0x014b);
    CHECK(7, e == 0x00a5 && f == 0x0052 && g == 0x0001);
  }
  {
    int a = sw >> n0, b = sw >> n1, c = sw >> n3, d = sw >> n8, e = sw >> n15, f = sw << n1, g = sw << n3;
    CHECK(8, a == -12345 && (unsigned int)b == 0xe7e3 && (unsigned int)c == 0xf9f8 && (unsigned int)d == 0xffcf);
    CHECK(9, e == -1 && (unsigned int)f == 0x9f8e && (unsigned int)g == 0x7e38);
  }
  {
    unsigned long a = ul << n0, b = ul << n1, c = ul << n7, d = ul << n8, e = ul << n9, f = ul << n16;
    unsigned long g = ul << n20, h = ul << n24, i = ul << n31;
    CHECK(10, a == 0xa5c3f017UL && b == 0x4b87e02eUL && c == 0xe1f80b80UL && d == 0xc3f01700UL);
    CHECK(11, e == 0x87e02e00UL && f == 0xf0170000UL && g == 0x01700000UL && h == 0x17000000UL && i == 0x80000000UL);
  }
  {
    unsigned long a = ul >> n0, b = ul >> n1, c = ul >> n7, d = ul >> n8, e = ul >> n9, f = ul >> n16;
    unsigned long g = ul >> n20, h = ul >> n24, i = ul >> n31;
    CHECK(12, a == 0xa5c3f017UL && b == 0x52e1f80bUL && c == 0x014b87e0UL && d == 0x00a5c3f0UL);
    CHECK(13, e == 0x0052e1f8UL && f == 0x0000a5c3UL && g == 0x00000a5cUL && h == 0x000000a5UL && i == 1);
  }
  {
    long a = sl >> n0, b = sl >> n1, c = sl >> n8, d = sl >> n9, e = sl >> n16, f = sl >> n24, g = sl >> n31;
    long h = sl << n1, i = sl << n8, j = sl << n20;
    CHECK(14, a == -123456789L && (unsigned long)b == 0xfc521975UL && (unsigned long)c == 0xfff8a432UL && (unsigned long)d == 0xfffc5219UL);
    CHECK(15, (unsigned long)e == 0xfffff8a4UL && f == -8 && g == -1);
    CHECK(16, (unsigned long)h == 0xf14865d6UL && (unsigned long)i == 0xa432eb00UL && (unsigned long)j == 0x2eb00000UL);
  }
  DONE();
  return 0;
}
