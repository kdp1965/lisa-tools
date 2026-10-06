/* test_irqhaz.c - the two TT07 interrupt hazards, each in its own loop
 * with a 1 ms timer interrupt running (handler: ticks++, acknowledge).
 *
 *  A: `ldc #1; if nc; ldi #0xff` - the ldi must be skipped (C is set); an
 *     interrupt taken right after the `if` loses the predicate and the
 *     ldi runs.  A is checked for 0 afterwards.
 *  B: `ldx #_rtn; txa` - IX must hold the address of rtn; an interrupt
 *     taken after the first word loads IX with the vector word instead
 *     and later executes the literal as a jal (harmless here: rtn
 *     returns).  The low byte of IX is checked.
 *
 * Each loop runs ~20 ms; the counts of corrupted iterations are printed.
 * On the TT07 silicon both are expected to be nonzero. */
#include "harness.h"
#include <tt07.h>

volatile unsigned char ticks;
volatile unsigned int bad_a, bad_b;

void timer1_isr(void) __interrupt(LISA_INT_TIMER1)
{
  ticks++;
  INT_STATUS = INT_TIMER1;
}

void rtn(void) { }

static void start(void)
{
  TIMER1_CTRL = 0;
  TIMER1_PREDIV_LO = 0x4f; TIMER1_PREDIV_HI = 0xc3;   /* 1 ms */
  TIMER1_DIV_LO = 1; TIMER1_DIV_HI = 0;
  INT_STATUS = 0xff;
  ticks = 0;
  INT_ENABLE = INT_TIMER1;
  TIMER1_CTRL = TIMER_CTRL_ENABLE;
  lisa_ei();
}

static void stop(void)
{
  lisa_di();
  TIMER1_CTRL = 0;
  INT_ENABLE = 0;
}

int main(void)
{
  bad_a = 0; bad_b = 0;
  start();
  while (ticks != 20)
    {
      __asm
	ldi	#0x00
	ldc	#1
	if	nc
	ldi	#0xff
	cpi	#0x00
	bz	1$
	ldx	#_bad_a
	inx	0(ix)
	if	c
	inx	1(ix)
1$:
      __endasm;
    }
  stop();
  start();
  while (ticks != 20)
    {
      __asm
	ldx	#_rtn
	txa
	cpi	#<(_rtn + 0)
	bz	2$
	ldx	#_bad_b
	inx	0(ix)
	if	c
	inx	1(ix)
2$:
      __endasm;
    }
  stop();
  puts("A (if skipped): "); puthex(bad_a); putc('\n');
  puts("B (ldx): "); puthex(bad_b); putc('\n');
  CHECK(1, bad_a == 0);
  CHECK(2, bad_b == 0);
  DONE();
  return 0;
}
