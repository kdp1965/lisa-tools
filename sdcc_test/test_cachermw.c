/*
 * test_cachermw.c - read-modify-write instructions on a data cache MISS
 * (hw_test.mjs spiram=): 0x100 and 0x140 share a cache line, so a store to
 * one evicts the other.  Each returns what the core got.  On the TT07
 * silicon the ix_* inx/dcx cases fail (a constant garbage result: the RMW
 * works on stale data, lisa_isa.md), ix_rd_first and ix_add pass; the sp_*
 * cases do not actually evict (the +0x40 crosses 0x8000) and pass.
 */
#include "harness.h"

static unsigned char ix_v05_b05(void) __naked
{
  __asm
    ldx #0x100
    ldi #0x05
    stax 0(ix)
    ldx #0x140
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
    ldx #0x140
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
    ldx #0x140
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
    ldx #0x140
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
    ldx #0x140
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
    ldx #0x140
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
    ldx #0x140
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
    adx #1
    adx #0x40
    ldi #0x77
    stax 0(ix)
    inx 1(sp)
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
    adx #1
    adx #0x40
    ldax 0(ix)
    inx 1(sp)
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
    ldx #0x140
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
