/*
 * test_cacheseq.c - sequential pointer writes and reads over a few hundred
 * bytes through the data cache (hw_test.mjs spiram=), in the low half of
 * the address space (cache lines 0..3) and the high half (lines 4..7).
 */
#include "harness.h"
static unsigned int fillsum(unsigned char *p, unsigned int n)
{
  unsigned int i, s = 0;
  for (i = 0; i < n; i++) p[i] = (unsigned char)(i * 3);
  puts("w"); putc('\n');
  for (i = 0; i < n; i++) s += p[i];
  puts("r"); putc('\n');
  return s;
}
int main(void)
{
  unsigned int s;
  s = fillsum((unsigned char *)0x0200, 64);  puthex(s); putc('\n'); CHECK(1, s == 6048);
  s = fillsum((unsigned char *)0x0200, 300); puthex(s); putc('\n'); CHECK(2, s == 0x8a96);
  s = fillsum((unsigned char *)0x4200, 64);  puthex(s); putc('\n'); CHECK(3, s == 6048);
  s = fillsum((unsigned char *)0x4200, 300); puthex(s); putc('\n'); CHECK(4, s == 0x8a96);
  s = fillsum((unsigned char *)0x7c70, 300); puthex(s); putc('\n'); CHECK(5, s == 0x8a96);
  s = fillsum((unsigned char *)0x7c70, 900); puthex(s); putc('\n'); CHECK(6, s == 0x1b5d2 % 65536);
  DONE();
  return 0;
}
