/* hw_psram.c - the data cache's PSRAM at a given SCLK (ttsky26d RESPIN 34,
 * the Arty's QSPI Pmod on CS1): a pattern written over 0x1000-0x3fff
 * through the cache, then read back twice.  Errors the same in both reads
 * were written wrong (or are read wrong the same way); errors that differ
 * between the reads are the read path.  The first few are listed: the
 * address, what was written, the two reads.  Build: sdcc -mlisa --rev3
 * (make hw_psram_rev3.ihx REV3=1); run with arty_run.py CACHE=1 SPI=0x.... */
#include "harness.h"

#define LO 0x1000u
#define HI 0x4000u

static unsigned char pat(unsigned int a, unsigned char seed)
{
  return (unsigned char)(a ^ (a >> 8) ^ seed);
}

static unsigned int bad1, bad2, shown;

static void put2(unsigned char v)
{
  const char *h = "0123456789abcdef";
  putc(h[v >> 4]);
  putc(h[v & 15]);
}

static void pass(unsigned char seed)
{
  volatile unsigned char *p;
  unsigned int a;
  unsigned char e, r1, r2;

  for (a = LO; a < HI; a++) {
    p = (volatile unsigned char *)a;
    *p = pat(a, seed);
  }
  bad1 = bad2 = shown = 0;
  for (a = LO; a < HI; a++) {
    p = (volatile unsigned char *)a;
    r1 = *p;
    if (r1 != pat(a, seed)) bad1++;
  }
  for (a = LO; a < HI; a++) {
    p = (volatile unsigned char *)a;
    e = pat(a, seed);
    r2 = *p;
    if (r2 != e) {
      bad2++;
      if (shown < 6) {
        shown++;
        puts("  "); puthex(a); putc(' '); put2(e); putc(' '); put2(r2); putc('\n');
      }
    }
  }
  puts("seed "); put2(seed);
  puts(" read1 bad "); puthex(bad1);
  puts(" read2 bad "); puthex(bad2);
  putc('\n');
}

void main(void)
{
  pass(0x5a);
  pass(0xa5);
  pass(0x00);
  CHECK(1, bad1 == 0 && bad2 == 0);
  DONE();
}
