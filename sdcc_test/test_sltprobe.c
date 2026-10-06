/* probe the silicon: cmp a,b then if slt / sgt / lt / c, for several sign
   combinations.  Prints one line per pair: a b slt sgt lt c */
#include "harness.h"

/* a at 3(sp), b at 4(sp): the inline asm makes the function save RA */
unsigned char p_slt(unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    ifte slt
    ldi.p #1
    ldi.p #0
__endasm; }
unsigned char p_sgt(unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    ifte sgt
    ldi.p #1
    ldi.p #0
__endasm; }
unsigned char p_lt (unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    ifte lt
    ldi.p #1
    ldi.p #0
__endasm; }
unsigned char p_c  (unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    ldac c
__endasm; }
/* same, but with a branch instead of ifte */
unsigned char p_sltbr(unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    if slt
    br.p 1$
    ldi #0
    br 2$
1$:
    ldi #1
2$:
__endasm; }
/* same, but with a nop between cmp and if */
unsigned char p_sltnop(unsigned char a, unsigned char b) { __asm
    ldax 3(sp)
    cmp 4(sp)
    nop
    ifte slt
    ldi.p #1
    ldi.p #0
__endasm; }
/* cpi instead of cmp (no signed inversion capture in the RTL) */
unsigned char p_sltcpi(unsigned char a) { __asm
    ldax 3(sp)
    cpi #0x00
    ifte slt
    ldi.p #1
    ldi.p #0
__endasm; }

static const unsigned char pairs[][2] = { {0xfe,0x00},{0x00,0xfe},{0x01,0x00},{0x00,0x01},{0x80,0x7f},{0x7f,0x80},{0xfe,0xfe},{0xfe,0xfd},{0x40,0xc0},{0xc0,0x40} };

int main(void)
{
  unsigned char i;
  puts("a  b  slt sgt lt c sltbr sltnop sltcpi(a)\n");
  for (i = 0; i < 10; i++)
    {
      unsigned char a = pairs[i][0], b = pairs[i][1];
      puthex(a); putc(' '); puthex(b); putc(' ');
      putc('0' + p_slt(a, b)); putc(' '); putc('0' + p_sgt(a, b)); putc(' ');
      putc('0' + p_lt(a, b)); putc(' '); putc('0' + p_c(a, b)); putc(' ');
      putc('0' + p_sltbr(a, b)); putc(' '); putc('0' + p_sltnop(a, b)); putc(' ');
      putc('0' + p_sltcpi(a)); putc('\n');
    }
  puts("ALL PASSED\n");
  return 0;
}
