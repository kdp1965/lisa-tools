/* test_bigframe.c - a 900-byte local array: bytes beyond the 511-byte n(sp)
 * reach go through an IX window.  Needs more than the 128 bytes of the
 * chip: simulator only (built with --stack-loc 0x7fff by the Makefile). */
#include "harness.h"
#include <string.h>
volatile unsigned int vk = 700;
unsigned char fill(unsigned char *p, unsigned int n) { unsigned int i; for (i = 0; i < n; i++) p[i] = (unsigned char)(i * 3); return p[n - 1]; }
/* the byte parameter arrives in A and is homed in the frame: with a frame
   past 511 bytes its slot must be in the far layout too (it once landed on
   RA's low byte: SDCC's bigstack) */
static unsigned char bigf(unsigned char par1, unsigned int par2) { unsigned char arr[780]; fill(arr, 780); return par1 + (par2 == 42) + (arr[779] == (unsigned char)(779 * 3)); }
int main(void)
{
  unsigned char buf[900];
  unsigned int k = vk, i, sum = 0;
  unsigned long acc = 1;
  unsigned char tail;
  fill(buf, sizeof buf);
  for (i = 0; i < sizeof buf; i++) sum += buf[i];
  CHECK(1, sum == 0x1b5d2 % 65536);
  CHECK(2, buf[k] == (unsigned char)(700 * 3));
  buf[k] = 0x5a; tail = buf[899];
  CHECK(3, buf[700] == 0x5a && tail == (unsigned char)(899 * 3));
  acc += buf[k]; acc <<= 4;
  CHECK(4, acc == ((1UL + 0x5a) << 4));
  memset(buf, 7, sizeof buf);
  CHECK(5, buf[0] == 7 && buf[450] == 7 && buf[899] == 7);
  { char *p = (char *)&buf[k]; *p = 9; CHECK(6, buf[700] == 9); }
  CHECK(7, bigf(23, 42) == 25 && bigf(0xa5, 0x5a5a) == 0xa6);
  DONE();
  return 0;
}
