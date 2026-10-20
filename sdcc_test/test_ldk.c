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
  unsigned int start, bix;
  unsigned char v, v5, r[7], i;

  /* The harness's strings ("ok ", the hex digits) are packed constants too
   * (--pack-const): every CHECK and puts runs with KB = 0, the values read
   * under another KB are kept in variables (on the chip an unprogrammed
   * bank reads 0xFF for ever: puts never ends) */
  v = KB;
  CHECK(1, v == 0);                     /* reset: the lower half */
  KB = 5;
  v5 = KB;
  KB = 0;
  CHECK(2, v5 == 5);                    /* writable, 6 bits */

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
  bix = g_ix;
  v = KB;
  for (i = 0; i < 7; i++)
    r[i] = next();
  KB = 0;
  puts("bank "); puthex(v); putc(' '); puthex(bix); putc('\n');
  CHECK(14, v == 2);
  CHECK(15, r[0] == 0x56);
  CHECK(16, r[1] == 0x80);
  CHECK(17, r[2] == 0x78);
  CHECK(18, r[3] == 0x80);
  CHECK(19, r[4] == 0xCD);
  CHECK(20, r[5] == 0x80);
  CHECK(21, r[6] == 0x00);
  v = KB;
  CHECK(22, v == 0);
  DONE();
  return 0;
}
