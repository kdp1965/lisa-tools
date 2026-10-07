/*
 * test_cachepush.c - push ix (a two-byte write) on a data cache miss at
 * every stack alignment (hw_test.mjs spiram=): the stack starts at 0x7fff,
 * every upper-half line is evicted first.  Returns 0x01 if the high byte
 * (written second, at the lower address) read back wrong, 0x02 for the low.
 */
#include "harness.h"

static unsigned char p00(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-0
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #2
    ret
  __endasm;
}

static unsigned char p01(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-1
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #3
    ret
  __endasm;
}

static unsigned char p02(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-2
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #4
    ret
  __endasm;
}

static unsigned char p03(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-3
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #5
    ret
  __endasm;
}

static unsigned char p04(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-4
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #6
    ret
  __endasm;
}

static unsigned char p05(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-5
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #7
    ret
  __endasm;
}

static unsigned char p06(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-6
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #8
    ret
  __endasm;
}

static unsigned char p07(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-7
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #9
    ret
  __endasm;
}

static unsigned char p08(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-8
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #10
    ret
  __endasm;
}

static unsigned char p09(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-9
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #11
    ret
  __endasm;
}

static unsigned char p10(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-10
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #12
    ret
  __endasm;
}

static unsigned char p11(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-11
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #13
    ret
  __endasm;
}

static unsigned char p12(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-12
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #14
    ret
  __endasm;
}

static unsigned char p13(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-13
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #15
    ret
  __endasm;
}

static unsigned char p14(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-14
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #16
    ret
  __endasm;
}

static unsigned char p15(void) __naked
{
  __asm
    ldx #0x4000
    ldi #0x00
    stax 0(ix)
    ldx #0x4010
    stax 0(ix)
    ldx #0x4020
    stax 0(ix)
    ldx #0x4030
    stax 0(ix)
    ads #-15
    ldx #0x4142
    push ix
    ldax 1(sp)
    cpi #0xc1
    ldac ne
    stax 1(sp)
    ldax 2(sp)
    cpi #0x42
    ldac ne
    ldc #0
    shl
    or 1(sp)
    ads #17
    ret
  __endasm;
}

int main(void)
{
  unsigned char r, bad = 0;
  r = p00(); puthex(r); putc(' '); bad |= r;
  r = p01(); puthex(r); putc(' '); bad |= r;
  r = p02(); puthex(r); putc(' '); bad |= r;
  r = p03(); puthex(r); putc(' '); bad |= r;
  r = p04(); puthex(r); putc(' '); bad |= r;
  r = p05(); puthex(r); putc(' '); bad |= r;
  r = p06(); puthex(r); putc(' '); bad |= r;
  r = p07(); puthex(r); putc(' '); bad |= r;
  r = p08(); puthex(r); putc(' '); bad |= r;
  r = p09(); puthex(r); putc(' '); bad |= r;
  r = p10(); puthex(r); putc(' '); bad |= r;
  r = p11(); puthex(r); putc(' '); bad |= r;
  r = p12(); puthex(r); putc(' '); bad |= r;
  r = p13(); puthex(r); putc(' '); bad |= r;
  r = p14(); puthex(r); putc(' '); bad |= r;
  r = p15(); puthex(r); putc(' '); bad |= r;
  putc('\n');
  CHECK(1, bad == 0);
  DONE();
  return 0;
}
