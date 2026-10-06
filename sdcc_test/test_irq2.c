/* test_irq2.c - the minimum interrupt round trip on TT07: a slow timer 1
 * interrupt, counted by the handler, while main waits in a loop that uses
 * no predication and no ldx (both are unsafe with interrupts on the TT07
 * silicon, see README-lisa.md).  Everything else runs with interrupts off. */
#include "harness.h"
#include <tt07.h>

volatile unsigned char ticks;

void timer1_isr(void) __interrupt(LISA_INT_TIMER1)
{
  ticks++;
  INT_STATUS = INT_TIMER1;
}

int main(void)
{
  unsigned char t;
  TIMER1_CTRL = 0;
  TIMER1_PREDIV_LO = 0x4f;      /* 50 MHz / 50000 = 1 ms */
  TIMER1_PREDIV_HI = 0xc3;
  TIMER1_DIV_LO = 5;            /* 5 ms */
  TIMER1_DIV_HI = 0;
  INT_STATUS = 0xff;
  ticks = 0;
  INT_ENABLE = INT_TIMER1;
  TIMER1_CTRL = TIMER_CTRL_ENABLE;
  lisa_ei();
  while (ticks != 4)            /* cpi / bnz only */
    ;
  lisa_di();
  t = ticks;
  TIMER1_CTRL = 0;
  INT_ENABLE = 0;
  puthex(t); putc('\n');
  CHECK(1, t == 4);
  DONE();
  return 0;
}
