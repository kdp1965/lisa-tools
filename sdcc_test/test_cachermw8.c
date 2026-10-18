/*
 * test_cachermw8.c - test_cachermw for the second chip's data cache
 * (ttsky26d data_cache8.v: 8 direct-mapped lines of 16 bytes, the index is
 * address bits 6:4): the evicting address is 0x80 away (0x100 / 0x180; the
 * TT07 4-line cache's 0x40 is another line here, so test_cachermw never
 * misses on it), the sp cases use the rev-2 stack (the top byte at 0(sp))
 * with the stack at 0x3fff so that sp + 0x80 stays below 0x8000.  Build:
 * sdcc -mlisa --rev2 --stack-loc 0x3fff; run with the data cache on.  No
 * cmp before any read-modify-write: the RTL fix (RESPIN.md item 2) is what
 * makes them right.
 */
#include "harness.h"

static unsigned char ix_v05_b05(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x05
    stax 0(ix)
    ldx #0x100
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char ix_v05_b77(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x100
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char ix_v33_b77(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x33
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x100
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char ix_nop(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x100
    nop
    nop
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char ix_ldx_far(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x0f0
    inx 16(ix)
    ldax 16(ix)
    ret
  __endasm;
}

static unsigned char ix_rd_first(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x100
    ldax 1(ix)
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char ix_clean(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldax 0(ix)
    ldx #0x100
    inx 0(ix)
    ldax 0(ix)
    ret
  __endasm;
}

static unsigned char sp_dirty(void) __naked
{
  __asm
    ldi #0x05
    push a
    spix
    adx #0x40
    adx #0x40
    ldi #0x77
    stax 0(ix)
    inx 0(sp)
    pop a
    ret
  __endasm;
}

static unsigned char sp_clean(void) __naked
{
  __asm
    ldi #0x05
    push a
    spix
    adx #0x40
    adx #0x40
    ldax 0(ix)
    inx 0(sp)
    pop a
    ret
  __endasm;
}

static unsigned char ix_add(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x180
    ldi #0x77
    stax 0(ix)
    ldx #0x100
    ldi #0x01
    add 0(ix)
    ret
  __endasm;
}

int main(void)
{
  unsigned char r;
  r = ix_v05_b05(); puts("ix_v05_b05 "); puthex(r); putc('\n'); CHECK(1, r == 0x06);
  r = ix_v05_b77(); puts("ix_v05_b77 "); puthex(r); putc('\n'); CHECK(2, r == 0x06);
  r = ix_v33_b77(); puts("ix_v33_b77 "); puthex(r); putc('\n'); CHECK(3, r == 0x34);
  r = ix_nop(); puts("ix_nop "); puthex(r); putc('\n'); CHECK(4, r == 0x06);
  r = ix_ldx_far(); puts("ix_ldx_far "); puthex(r); putc('\n'); CHECK(5, r == 0x06);
  r = ix_rd_first(); puts("ix_rd_first "); puthex(r); putc('\n'); CHECK(6, r == 0x06);
  r = ix_clean(); puts("ix_clean "); puthex(r); putc('\n'); CHECK(7, r == 0x06);
  r = sp_dirty(); puts("sp_dirty "); puthex(r); putc('\n'); CHECK(8, r == 0x06);
  r = sp_clean(); puts("sp_clean "); puthex(r); putc('\n'); CHECK(9, r == 0x06);
  r = ix_add(); puts("ix_add "); puthex(r); putc('\n'); CHECK(10, r == 0x06);
  DONE();
  return 0;
}
