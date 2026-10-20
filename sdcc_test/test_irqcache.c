/*
 * test_irqcache.c - the timer interrupt landing on stores and loads that
 * miss the second chip's data cache, at every phase.
 *
 * The main loop walks a pattern of stores and read-backs across the two
 * halves the cache splits at 0x4000 (data at 0x3ff0.., the stack above
 * 0x4000 with --stack-loc 0x7fff) and over more lines than the cache has
 * (0x80 apart: eight 16-byte lines, so every step evicts), while TIMER1
 * interrupts with a period that is coprime to the loop, so the interrupt
 * drifts through every instruction of it over the run - the handler
 * itself stores to the stack and to data.  Every iteration checks what it
 * wrote; the handler's count is checked at the end.  A core or cache that
 * drops a store, or stalls, when an interrupt hits a miss shows up as a
 * FAIL or a timeout.  REV2=1 (the chip testbench with CACHE=1 is where it
 * matters: lisa_sim has no cache).
 */
#include "harness.h"
#include <tt07.h>

volatile unsigned int ticks;
volatile unsigned char isr_pat;
__at(0x3ff0) volatile unsigned char edge[32];        /* across 0x4000 */
__at(0x2000) volatile unsigned char lines[0x400];    /* 0x80 apart: evictions */

void timer1_isr(void) __interrupt(LISA_INT_TIMER1)
{
  volatile unsigned char local[4];                   /* stores on the stack in the handler */
  unsigned char p = isr_pat;
  local[0] = p;
  local[3] = (unsigned char)~p;
  lines[0x3f8] = p;                                  /* a line of its own */
  ticks++;
  isr_pat = (unsigned char)(p + 1);
  if (local[0] != p || local[3] != (unsigned char)~p)
    isr_pat = 0xee;                                  /* never: a flag the end checks */
  INT_STATUS = INT_TIMER1;
}

static unsigned char step(unsigned char k)
{
  volatile unsigned char stk[16];                    /* on the stack above 0x4000 */
  unsigned int i;
  unsigned char bad = 0;

  for (i = 0; i < 32; i++)
    edge[i] = (unsigned char)(k + i);
  for (i = 0; i < 16; i++)
    stk[i] = (unsigned char)(k ^ i);
  for (i = 0; i < 8; i++)
    lines[i * 0x80] = (unsigned char)(k + 0x40 + i);
  for (i = 0; i < 32; i++)
    if (edge[i] != (unsigned char)(k + i))
      bad |= 1;
  for (i = 0; i < 16; i++)
    if (stk[i] != (unsigned char)(k ^ i))
      bad |= 2;
  for (i = 0; i < 8; i++)
    if (lines[i * 0x80] != (unsigned char)(k + 0x40 + i))
      bad |= 4;
  edge[31]++;                                        /* read-modify-write on the far side */
  lines[0x80]++;
  if (edge[31] != (unsigned char)(k + 32) || lines[0x80] != (unsigned char)(k + 0x42))
    bad |= 8;
  return bad;
}

int main(void)
{
  unsigned int n;
  unsigned char bad = 0;

  TIMER1_CTRL = 0;
  TIMER1_PREDIV_LO = 0x06;      /* 50 MHz / 7 */
  TIMER1_PREDIV_HI = 0x00;
  TIMER1_DIV_LO = 331 & 0xff;   /* a tick every 2317 clocks: a prime, so it drifts through the loop; the
                                   handler's own misses take a few hundred clocks on the chip bench */
  TIMER1_DIV_HI = 331 >> 8;
  INT_STATUS = 0xff;
  ticks = 0;
  isr_pat = 0;
  INT_ENABLE = INT_TIMER1;
  TIMER1_CTRL = TIMER_CTRL_ENABLE;
  lisa_ei();
  for (n = 0; n < 600; n++)
    {
      bad |= step((unsigned char)n);
      if (bad)
        break;
      if ((n & 63) == 63)
        putc('.');                /* the chip bench times out on silence */
    }
  lisa_di();
  putc('\n');
  CHECK(1, bad == 0);
  CHECK(2, ticks > 500);
  CHECK(3, isr_pat != 0xee);
  CHECK(4, n == 600);
  DONE();
  return 0;
}
