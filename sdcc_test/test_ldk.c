/* test_ldk.c - lda (ix++) and the constant bank KB (0x21F) of the second chip
 * (ttsky26d RESPIN 26): A takes byte IX[0] of the code word at {KB, IX[14:1]},
 * Z from it, IX + 1.  Constants can be packed two to a word in any bank.
 * The words read are instructions (ldi #k = 0x80kk) of a function in the
 * lower half (KB 0) and of one in bank 2 (test_ldk2.c, --codeseg BANK2).
 * sdcc --rev2 only. */
#include "harness.h"
#include <tt07.h>

__sfr __at(0x21f) KB;

volatile unsigned int g_ix;             /* IX in, IX out */
volatile unsigned char g_a, g_z;

extern void pattern2(void) __banked;    /* in bank 2: ldi #0x56, ldi #0x78, ldi #0xCD, ret */

static void pattern(void) __naked
{
  __asm
	ldi	#0x12
	ldi	#0x34
	ldi	#0xAB
	ret
  __endasm;
}

/* KB = the bank of pattern2 (its window address is in C: pattern2 itself) */
static void kb_of_pattern2(void) __naked
{
  __asm
	ldi	#b_pattern2
	sta	0x21f
	ret
  __endasm;
}

/* one lda (ix++): IX from g_ix, the byte to g_a, Z to g_z, IX back to g_ix */
static void ldk_step(void) __naked
{
  __asm
	lda	_g_ix
	tax
	lda	_g_ix+1
	taxu
	ldi	#0
	sta	_g_z
	lda	(ix++)
	sta	_g_a
	bnz	1$
	ldi	#1
	sta	_g_z
1$:
	txa
	sta	_g_ix
	txau
	andi	#0x7f
	sta	_g_ix+1
	ret
  __endasm;
}

static unsigned char next(void)
{
  ldk_step();
  return g_a;
}

int main(void)
{
  unsigned int start;
  unsigned char v;

  v = KB;
  CHECK(1, v == 0);                     /* reset: the lower half */
  KB = 5;
  v = KB;
  CHECK(2, v == 5);                     /* writable, 6 bits */
  KB = 0;

  g_ix = (unsigned int)pattern << 1;    /* the byte offset of its first word in the bank */
  start = g_ix;
  puts("lower "); puthex(start); putc(' ');
  CHECK(3, next() == 0x12);             /* ldi #0x12: the low byte first */
  CHECK(4, next() == 0x80);             /* then the high byte of the word */
  CHECK(5, g_z == 0);
  CHECK(6, next() == 0x34);
  CHECK(7, next() == 0x80);
  CHECK(8, next() == 0xAB);
  CHECK(9, next() == 0x80);
  CHECK(10, next() == 0x00);            /* ret = 0x8A00 */
  CHECK(11, g_z == 1);                  /* Z from the byte */
  CHECK(12, next() == 0x8A);
  CHECK(13, g_ix == start + 8);         /* IX + 1 per byte */

  kb_of_pattern2();                     /* KB = bank 2 */
  g_ix = (unsigned int)pattern2 << 1;   /* 0x4000 << 1 = 0x8000: offset 0 of the bank (bit 15 is not an address) */
  v = KB;
  puts("bank "); puthex(v); putc(' '); puthex(g_ix); putc('\n');
  CHECK(14, v == 2);
  CHECK(15, next() == 0x56);
  CHECK(16, next() == 0x80);
  CHECK(17, next() == 0x78);
  CHECK(18, next() == 0x80);
  CHECK(19, next() == 0xCD);
  CHECK(20, next() == 0x80);
  CHECK(21, next() == 0x00);
  KB = 0;
  v = KB;
  CHECK(22, v == 0);
  DONE();
  return 0;
}
