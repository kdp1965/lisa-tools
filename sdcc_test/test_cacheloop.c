/* test_cacheloop.c - test_cacheseq's fillsum at 0x4200, instrumented (hw_test.mjs spiram=) */
#include "harness.h"
static void fill(unsigned char *p, unsigned int n)
{
  unsigned int i;
  for (i = 0; i < n; i++) p[i] = (unsigned char)(i * 3);
}
int main(void)
{
  unsigned char *p = (unsigned char *)0x4200;
  unsigned int j, s = 0;
  fill(p, 64);
  for (j = 0; j < 64; j++) { puthex(p[j]); putc(' '); s += p[j]; }
  putc('\n'); puthex(s); putc('\n');
  CHECK(1, s == 0x17a0);
  DONE();
  return 0;
}
