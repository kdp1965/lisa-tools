/* test_ldirq.c - the ISR shadows through ldirq / stirq on the second chip:
 * a context switch carries them in a byte, {amode[2:0], signed_inv_live,
 * cflag_save, signed_inv, Z, C}, that ldirq reads and stirq writes back,
 * and rets restores the set from the shadows (RESPIN 6).  Until 2026-10-18
 * the RTL's ldirq loaded nothing (RESPIN 24): the byte was whatever A held,
 * stirq wrote it back, and rets came back with amode 0 - every shift of the
 * compiler's code lost its carry from then on (NuttX never reached the
 * shell).  The handler here does what NuttX's does, main checks the byte it
 * saw and the mode it came back to.  sdcc --rev2 only (ldamode). */
#include "harness.h"
#include <tt07.h>

volatile unsigned char ticks;
volatile unsigned char seen;          /* what ldirq read in the handler */

void timer1_isr(void) __interrupt(LISA_INT_TIMER1)
{
  ticks++;
  INT_STATUS = INT_TIMER1;
  __asm
	ldi	#0x5a			; a value ldirq must replace
	ldirq
	sta	_seen
	stirq				; the round trip a context switch restore makes
  __endasm;
}

static unsigned char mode(void) __naked
{
  __asm
	ldamode				; A = {ie, amode}
	ret
  __endasm;
}

static unsigned char shift_in(void) __naked
{
  __asm
	ldi	#0x40
	ldc	#1
	shl				; amode 1: C comes in at bit 0
	ret
  __endasm;
}

int main(void)
{
  unsigned char t, m;
  TIMER1_CTRL = 0;
  TIMER1_PREDIV_LO = 0x4f;      /* 50 MHz / 50000 = 1 ms */
  TIMER1_PREDIV_HI = 0xc3;
  TIMER1_DIV_LO = 2;            /* 2 ms */
  TIMER1_DIV_HI = 0;
  INT_STATUS = 0xff;
  ticks = 0;
  seen = 0xff;
  INT_ENABLE = INT_TIMER1;
  TIMER1_CTRL = TIMER_CTRL_ENABLE;
  CHECK(1, mode() == 1);        /* ie 0, amode 1 */
  lisa_ei();
  CHECK(2, mode() == 9);        /* ie 1 */
  while (ticks != 2)            /* cpi / bnz only */
    ;
  lisa_di();
  t = ticks;
  TIMER1_CTRL = 0;
  INT_ENABLE = 0;
  puthex(t); putc(' '); puthex(seen); putc(' '); m = mode(); puthex(m); putc('\n');
  CHECK(3, t == 2);
  CHECK(4, (seen & 0xe0) == 0x20);     /* the handler saw amode 1 in the shadow */
  CHECK(5, (seen & 0x10) == 0);        /* no signed inversion live in the loop */
  CHECK(6, m == 1);                    /* amode 1 came back through stirq / rets */
  CHECK(7, shift_in() == 0x81);        /* and shifts still rotate through C */
  DONE();
  return 0;
}
