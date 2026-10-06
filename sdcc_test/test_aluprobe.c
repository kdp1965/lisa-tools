/* probe the silicon ALU flags.  Each line: name result carry */
#include "harness.h"

/* 16-bit return slot at 3(sp)/4(sp), a at 5(sp), b at 6(sp) (RA saved by the inline asm) */
unsigned int add_c1(unsigned char a, unsigned char b)   /* ldc 1; a + b -> result | carry<<8 */
{ __asm
    ldax 5(sp)
    ldc #1
    add 6(sp)
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }
unsigned int sub_c0(unsigned char a, unsigned char b)   /* ldc 0; a - b */
{ __asm
    ldax 5(sp)
    ldc #0
    sub 6(sp)
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }
unsigned int sub_c1(unsigned char a, unsigned char b)   /* ldc 1; a - b - 1 */
{ __asm
    ldax 5(sp)
    ldc #1
    sub 6(sp)
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }
unsigned int adcff_c1(unsigned char a)                  /* ldc 1; adc #0xff */
{ __asm
    ldax 5(sp)
    ldc #1
    adc #0xff
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }
unsigned int adc00_c1(unsigned char a)                  /* ldc 1; adc #0x00 */
{ __asm
    ldax 5(sp)
    ldc #1
    adc #0x00
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }
unsigned int cmp0(unsigned char a)                      /* cmp with memory 0: carry */
{ __asm
    ldi #0
    stax 6(sp)
    ldax 5(sp)
    cmp 6(sp)
    stax 3(sp)
    ldac c
    stax 4(sp)
__endasm; }

static void line(const char *n, unsigned int r) { puts(n); putc(' '); puthex(r); putc('\n'); }

int main(void)
{
  line("add_c1 01+01  ", add_c1(1, 1));        /* doc: 0103, carry-less add: 0002 */
  line("add_c1 ff+01  ", add_c1(0xff, 1));     /* carry-less: 0100 */
  line("sub_c0 05-00  ", sub_c0(5, 0));        /* right: 0005; silicon: 0105 */
  line("sub_c0 05-03  ", sub_c0(5, 3));        /* 0002 */
  line("sub_c0 03-05  ", sub_c0(3, 5));        /* 01fe */
  line("sub_c1 05-00-1", sub_c1(5, 0));        /* 0004 */
  line("sub_c1 00-ff-1", sub_c1(0, 0xff));     /* 0100: result 0, borrow */
  line("adcff_c1 05   ", adcff_c1(5));         /* right: 0105; silicon: 0005 */
  line("adcff_c1 00   ", adcff_c1(0));         /* right: 0100; silicon: 0000 */
  line("adc00_c1 ff   ", adc00_c1(0xff));      /* 0100 */
  line("cmp0 05       ", cmp0(5));             /* right: 0005 (no borrow); silicon: 0105 */
  line("cmp0 00       ", cmp0(0));             /* right: 0000; silicon: 0100 */
  puts("ALL PASSED\n");
  return 0;
}
