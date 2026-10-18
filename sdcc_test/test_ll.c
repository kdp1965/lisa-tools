/* test_ll.c - long long arithmetic through the library's support routines
 * (_mullonglong, _divulonglong, _divslonglong, _modulonglong,
 * _modslonglong, the 64-bit shifts): NuttX's clock arithmetic uses them */
#include "harness.h"

volatile unsigned long long a = 123456789012ULL, b = 1000ULL, c = 7ULL;
volatile long long sa = -123456789012LL, sb = 1000LL, sc = -7LL;
volatile unsigned long long z = 0;

void main(void)
{
  CHECK(1, a / b == 123456789ULL);
  CHECK(2, a % b == 12ULL);
  CHECK(3, z / c == 0ULL);
  CHECK(4, (a >> 3) == 15432098626ULL);
  CHECK(5, (b << 40) == 1099511627776000ULL);
  CHECK(6, a * c == 864197523084ULL);
  CHECK(7, sa / sb == -123456789LL);
  CHECK(8, sa % sb == -12LL);
  CHECK(9, (unsigned long)(a / 10000ULL) == 12345678UL);
  CHECK(10, sa / sc == 17636684144LL);
  CHECK(11, sa % sc == -4LL);
  CHECK(12, (-sa) % sb == 12LL);
  CHECK(13, (sa >> 4) == -7716049314LL);
  CHECK(14, (a + sa) == 0ULL);
  CHECK(15, a > b && sa < sb && z == 0);
  DONE();
}
