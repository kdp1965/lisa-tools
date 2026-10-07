/*
 * test_divl.c - unsigned long / and % on the hardware divider (divul.s:
 * a byte at a time below 256, Knuth D in base 256 above), then signed
 * long on top of it.  Generated expectations; the last block cross-checks
 * an LCG stream against a bit-serial reference.
 */
#include "harness.h"

static const unsigned long tab[][4] = {
  { 0x075bcd15UL, 0x000003e8UL, 0x0001e240UL, 0x00000315UL },
  { 0x075bcd15UL, 0x00000007UL, 0x010d1d4cUL, 0x00000001UL },
  { 0xb2d05e00UL, 0x00011170UL, 0x0000a769UL, 0x00002710UL },
  { 0xffffffffUL, 0x00000001UL, 0xffffffffUL, 0x00000000UL },
  { 0xffffffffUL, 0xffffffffUL, 0x00000001UL, 0x00000000UL },
  { 0xffffffffUL, 0x00000002UL, 0x7fffffffUL, 0x00000001UL },
  { 0xffffffffUL, 0x000000ffUL, 0x01010101UL, 0x00000000UL },
  { 0xffffffffUL, 0x00000100UL, 0x00ffffffUL, 0x000000ffUL },
  { 0xffffffffUL, 0x00008000UL, 0x0001ffffUL, 0x00007fffUL },
  { 0xffffffffUL, 0x0000ffffUL, 0x00010001UL, 0x00000000UL },
  { 0xffffffffUL, 0x00010000UL, 0x0000ffffUL, 0x0000ffffUL },
  { 0xffffffffUL, 0x80000000UL, 0x00000001UL, 0x7fffffffUL },
  { 0x80000000UL, 0x80000000UL, 0x00000001UL, 0x00000000UL },
  { 0x80000000UL, 0x7fffffffUL, 0x00000001UL, 0x00000001UL },
  { 0x12345678UL, 0x00009abcUL, 0x00001e1eUL, 0x00002c70UL },
  { 0x12345678UL, 0x0000def0UL, 0x000014e7UL, 0x00006be8UL },
  { 0x12345678UL, 0x00012345UL, 0x00001000UL, 0x00000678UL },
  { 0x12345678UL, 0x12345678UL, 0x00000001UL, 0x00000000UL },
  { 0x12345678UL, 0x12345679UL, 0x00000000UL, 0x12345678UL },
  { 0x00000001UL, 0xffffffffUL, 0x00000000UL, 0x00000001UL },
  { 0x00000000UL, 0x00003039UL, 0x00000000UL, 0x00000000UL },
  { 0x00ff00ffUL, 0x00000101UL, 0x0000fe02UL, 0x000000fdUL },
  { 0xffff0000UL, 0x0000ffffUL, 0x00010000UL, 0x00000000UL },
  { 0xfffe0001UL, 0x0000ffffUL, 0x0000ffffUL, 0x00000000UL },
  { 0x7fff8000UL, 0x00008001UL, 0x0000fffdUL, 0x00000003UL },
  { 0x40000000UL, 0x00004001UL, 0x0000fffcUL, 0x00000004UL },
  { 0x00010000UL, 0x00000100UL, 0x00000100UL, 0x00000000UL },
  { 0x00ffffffUL, 0x0000007fUL, 0x00020408UL, 0x00000007UL },
  { 0x000f4240UL, 0x000003e8UL, 0x000003e8UL, 0x00000000UL },
  { 0x3b9ac9ffUL, 0x0001869fUL, 0x00002710UL, 0x0000270fUL },
  { 0xa5a5a5a5UL, 0x00005a5aUL, 0x0001d557UL, 0x00000f0fUL },
  { 0xa5a5a5a5UL, 0x005a5a5aUL, 0x000001d5UL, 0x001e1ec3UL },
  { 0xdeadbeefUL, 0x0000beefUL, 0x00012a90UL, 0x0000227fUL },
  { 0xdeadbeefUL, 0xdead0000UL, 0x00000001UL, 0x0000beefUL },
  { 0x00010000UL, 0x0000ffffUL, 0x00000001UL, 0x00000001UL },
  { 0x0000ffffUL, 0x00010000UL, 0x00000000UL, 0x0000ffffUL },
};

static unsigned long refdiv(unsigned long n, unsigned long d, unsigned long *rem)
{
  unsigned long q = 0, r = 0;
  unsigned char i;
  for (i = 0; i < 32; i++) {
    r = (r << 1) | (n >> 31);
    n <<= 1;
    q <<= 1;
    if (r >= d) { r -= d; q |= 1; }
  }
  *rem = r;
  return q;
}

volatile unsigned long va, vb;
volatile long sa = -123456789L, sb = 1000L, sc = -7L, sd = 70000L;
volatile long se = -2147483647L - 1, sf = 65536L, sg = -65536L, sh = 3L, sj = 2000000000L, sk = -70000L;

int main(void)
{
  unsigned char i, bad = 0;
  unsigned long x = 0x12345678UL;

  for (i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
    va = tab[i][0]; vb = tab[i][1];
    CHECK(i + 1, va / vb == tab[i][2] && va % vb == tab[i][3]);
  }
  /* signed, on top of the unsigned routines */
  CHECK(50, sa / sb == -123456L && sa % sb == -789L);
  CHECK(51, sa / sc == 17636684L && sa % sc == -1L);
  CHECK(52, sb / sc == -142L && sb % sc == 6L);
  CHECK(53, sd / sb == 70L && sd % sb == 0L && -sd / sb == -70L);
  /* __divslong / __modslong in divul.s: LONG_MIN, divisors of 16 bits and more of both signs */
  CHECK(55, se / sb == -2147483L && se % sb == -648L);
  CHECK(56, se / sf == -32768L && se % sf == 0L && se / sh == -715827882L && se % sh == -2L);
  CHECK(57, sa / sg == 1883L && sa % sg == -52501L);
  CHECK(58, sj / sk == -28571L && sj % sk == 30000L);
  CHECK(59, sc / sb == 0L && sc % sb == -7L && sb / sa == 0L && sb % sa == 1000L);
  CHECK(60, se / sg == 32768L && se % sg == 0L && se / sb * sb + se % sb == se);
  /* an LCG stream, divisors of every size, against the bit-serial reference */
  for (i = 0; i < 200; i++) {
    unsigned long a, b, r, q;
    x = x * 1103515245UL + 12345UL;
    a = x;
    x = x * 1103515245UL + 12345UL;
    b = x >> ((i & 3) * 8);
    if (b == 0) b = 1;
    q = refdiv(a, b, &r);
    if (a / b != q || a % b != r) {
      bad++;
      puts("bad "); puthex((unsigned int)(a >> 16)); puthex((unsigned int)a); putc('/'); puthex((unsigned int)(b >> 16)); puthex((unsigned int)b); putc('\n');
    }
  }
  CHECK(54, bad == 0);
  DONE();
  return 0;
}
