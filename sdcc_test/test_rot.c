/*
 * test_rot.c - the ROT and GETWORD iCodes the port claims (hasExtBitOp,
 * genRot / genGetWord): (x << s) | (x >> (bits - s)) in both orders, a
 * byte by every count, a word by 1, 8 and 15, a long by 16, and a right
 * shift of a long by 8 or 16 taken as a word, including in place.
 */
#include "harness.h"

volatile unsigned char vb = 0x96;          /* 1001 0110 */
volatile unsigned int vw = 0x8421;
volatile unsigned long vl = 0x89abcdefUL;
volatile long sl = -123456789L;            /* 0xf8a432eb */

static unsigned char rol8(unsigned char x, unsigned char n)
{
  while (n--)
    x = (x << 1) | (x >> 7);
  return x;
}

static unsigned char l1(unsigned char x) { return (x << 1) | (x >> 7); }
static unsigned char l2(unsigned char x) { return (x << 2) | (x >> 6); }
static unsigned char l3(unsigned char x) { return (x << 3) | (x >> 5); }
static unsigned char l4(unsigned char x) { return (x << 4) | (x >> 4); }
static unsigned char l5(unsigned char x) { return (x >> 3) | (x << 5); }
static unsigned char l6(unsigned char x) { return (x >> 2) | (x << 6); }
static unsigned char l7(unsigned char x) { return (x >> 1) | (x << 7); }
static unsigned int w1(unsigned int x) { return (x << 1) | (x >> 15); }
static unsigned int w8(unsigned int x) { return (x << 8) | (x >> 8); }
static unsigned int w15(unsigned int x) { return (x >> 1) | (x << 15); }
static unsigned long d16(unsigned long x) { return (x << 16) | (x >> 16); }
static unsigned int gw8(unsigned long x) { return x >> 8; }
static unsigned int gw16(unsigned long x) { return x >> 16; }
static unsigned int gw16m(unsigned long x) { return (x >> 16) & 0xffff; }
static int sgw16(long x) { return x >> 16; }
static unsigned char gb24(unsigned long x) { return x >> 24; }

int main(void)
{
  unsigned char b;
  unsigned int w;
  unsigned long l;

  CHECK(1, l1(vb) == 0x2d && l1(0x80) == 0x01 && l1(0x7f) == 0xfe);
  CHECK(2, l2(vb) == 0x5a && l3(vb) == 0xb4 && l4(vb) == 0x69);
  CHECK(3, l5(vb) == 0xd2 && l6(vb) == 0xa5 && l7(vb) == 0x4b && l7(0x01) == 0x80);
  CHECK(4, l1(vb) == rol8(vb, 1) && l3(vb) == rol8(vb, 3) && l6(vb) == rol8(vb, 6));
  b = vb;
  b = (b << 1) | (b >> 7);
  b = (b >> 2) | (b << 6);
  CHECK(5, b == 0x4b);
  CHECK(6, w1(vw) == 0x0843 && w1(0x8000) == 0x0001 && w15(vw) == 0xc210 && w15(0x0001) == 0x8000);
  CHECK(7, w8(vw) == 0x2184 && w8(0x00ff) == 0xff00);
  w = vw;
  w = (w << 8) | (w >> 8);
  w = (w << 1) | (w >> 15);
  w = (w >> 1) | (w << 15);
  CHECK(8, w == 0x2184);
  CHECK(9, d16(vl) == 0xcdef89abUL);
  l = vl;
  l = (l << 16) | (l >> 16);
  CHECK(10, l == 0xcdef89abUL);
  CHECK(11, gw8(vl) == 0xabcd && gw16(vl) == 0x89ab && gw16m(vl) == 0x89ab && gb24(vl) == 0x89);
  CHECK(12, sgw16(sl) == (int)0xf8a4 && (unsigned int)(sl >> 8) == 0xa432);
  l = vl;
  w = l >> 16;
  CHECK(13, w == 0x89ab);
  DONE();
  return 0;
}
