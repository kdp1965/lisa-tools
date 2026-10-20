/* test_bankspeed.c - how fast are bytes in each region of the data space
 * (ttsky26d RESPIN 28; a probe, not a check): 4096 bytes written twice and
 * read once, through IX, with the loop's locals on the stack (bank 1 at the
 * top).  The host times the lines.  --rev2, the cache on. */
#include "harness.h"

__sfr __at(0x216) DATA_BANK;

static void region(const char *name, unsigned char *p, unsigned char bank)
{
  unsigned int i, sum = 0;
  unsigned char k;

  DATA_BANK = bank;
  puts(name);
  puts(" start\n");
  for (k = 0; k < 2; k++)
    for (i = 0; i < 4096; i++)
      p[i] = (unsigned char)(i + k);
  for (i = 0; i < 4096; i++)
    sum += p[i];
  DATA_BANK = 1;
  puts(name);
  puts(" done\n");
  (void)sum;
}

#ifndef REGION
#define REGION 99
#endif

void main(void)
{
  if (REGION == 0 || REGION == 99) region("common 0x1000", (unsigned char *)0x1000, 1);
  if (REGION == 1 || REGION == 99) region("window DB=1 (phys 8K)", (unsigned char *)0x2000, 1);
  if (REGION == 2 || REGION == 99) region("window DB=4 (phys 32K)", (unsigned char *)0x2000, 4);
  if (REGION == 3 || REGION == 99) region("window DB=8 (phys 64K)", (unsigned char *)0x2000, 8);
  if (REGION == 4 || REGION == 99) region("window DB=16 (phys 128K)", (unsigned char *)0x2000, 16);
  if (REGION == 5 || REGION == 99) region("window DB=30 (phys 240K)", (unsigned char *)0x2000, 30);
  if (REGION == 6 || REGION == 99) region("stack window 0x6000 SB=1 (phys 24K)", (unsigned char *)0x6000, 1);
  if (REGION == 7) { puts("nothing\n"); }
  DONE();
}
