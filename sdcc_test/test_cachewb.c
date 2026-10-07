/*
 * test_cachewb.c - the data cache's write-back on the SPI RAM (hw_test.mjs
 * spiram=).  data_cache8.v: 8 lines of 16 bytes, line = {a[14], a[5:4]},
 * so 0x100, 0x140, 0x180, 0x1c0 all share line 0 (tags differ).
 */
#include "harness.h"
#define A ((volatile unsigned char *)0x100)
#define B ((volatile unsigned char *)0x140)
#define C ((volatile unsigned char *)0x180)
#define D ((volatile unsigned char *)0x1c0)
static void show(const char *t, unsigned char v) { puts(t); puthex(v); putc('\n'); }
int main(void)
{
  unsigned char v;
  A[0] = 0x11; B[0] = 0x22; C[0] = 0x33;             /* write misses, each evicting the last */
  v = A[0]; show("A ", v); CHECK(1, v == 0x11);
  v = B[0]; show("B ", v); CHECK(2, v == 0x22);
  v = C[0]; show("C ", v); CHECK(3, v == 0x33);
  A[1] = 0x44;                                       /* dirty A, then a read miss evicts it */
  v = B[1]; v = A[1]; show("A1 ", v); CHECK(4, v == 0x44);
  A[2] = 0x55; A[2]++;                               /* RMW on a hit, then eviction by a write miss */
  B[2] = 0x66; v = A[2]; show("A2 ", v); CHECK(5, v == 0x56);
  A[3] = 0x77; v = B[3]; v = C[3]; v = D[3]; v = A[3]; show("A3 ", v); CHECK(6, v == 0x77);
  A[4] = 1; B[4] = 2; A[4]++; B[4]++; A[4]++; B[4]++;
  v = A[4]; show("A4 ", v); CHECK(7, v == 3);
  v = B[4]; show("B4 ", v); CHECK(8, v == 4);
  A[5] = 0x80; v = A[5]; B[5] = 0x90; A[5]++; v = A[5]; show("A5 ", v); CHECK(9, v == 0x81);
  v = B[5]; show("B5 ", v); CHECK(10, v == 0x90);
  DONE();
  return 0;
}
