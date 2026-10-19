/* test_dbank.c - data in the second chip's data banks (ttsky26d RESPIN 22,
 * the data map of RESPIN 28) through SDCC's named address spaces: an object
 * declared in a space made with __addressmod is reached through the data
 * window 0x2000-0x3fff after the space's switch function has set the data
 * bank [p:0x16] (a granule of 8K).  Each space's area (DBANK2, DBANK3) is
 * linked at 0x2000; the stack is above 0x4000, in the stack bank whatever
 * the data bank. --rev2 only. */
#include "harness.h"

__sfr __at(0x216) DATA_BANK;

static void dbank2(void) { DATA_BANK = 2; }
static void dbank3(void) { DATA_BANK = 3; }

__addressmod dbank2 DBANK2;
__addressmod dbank3 DBANK3;

DBANK2 unsigned char a2[100];
DBANK2 unsigned int  w2;
DBANK3 unsigned char a3[100];
DBANK3 unsigned int  w3;
unsigned char plain[16];             /* ordinary data, below 0x2000 */

static unsigned char sum2(void)
{
  unsigned char i, s = 0;
  for (i = 0; i < 100; i++) s += a2[i];
  return s;
}

void main(void)
{
  unsigned char i;

  for (i = 0; i < 100; i++) { a2[i] = i; a3[i] = 200 - i; plain[i & 15] = i; }
  w2 = 0x1234;
  w3 = 0xabcd;
  CHECK(1, (unsigned int) &a2[0] == (unsigned int) &a3[0]);   /* one window address, two banks */
  CHECK(2, a2[10] == 10 && a3[10] == 190);
  CHECK(3, w2 == 0x1234 && w3 == 0xabcd);
  CHECK(4, sum2() == (unsigned char) 4950);
  CHECK(5, plain[3] == 99);                /* ordinary data, untouched by the banks: the last i with i & 15 == 3 */
  a3[50] += a2[50];
  CHECK(6, a3[50] == 200);
  DONE();
}
