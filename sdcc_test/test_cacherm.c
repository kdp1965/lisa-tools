/*
 * test_cacherm.c - swap n(sp) through the data cache (hw_test.mjs spiram=):
 * the TT07 core forms its address as sp + n - 512 (lisa_core.v sp_adder),
 * which only the 128-byte RAM hides.
 */
#include "harness.h"

static unsigned char sw_m512a(void) __naked
{
  __asm
    ldi #0x80
    push a
    spix
    adx #1
    adx #-512
    ldi #0x5a
    stax 0(ix)
    ldi #0x11
    swap 1(sp)
    ads #1
    ret
  __endasm;
}

static unsigned char sw_m512m(void) __naked
{
  __asm
    ldi #0x80
    push a
    spix
    adx #1
    adx #-512
    ldi #0x5a
    stax 0(ix)
    ldi #0x11
    swap 1(sp)
    ldax 0(ix)
    ads #1
    ret
  __endasm;
}

static unsigned char sw_m512s(void) __naked
{
  __asm
    ldi #0x80
    push a
    spix
    adx #1
    adx #-512
    ldi #0x5a
    stax 0(ix)
    ldi #0x11
    swap 1(sp)
    pop a
    ret
  __endasm;
}

int main(void)
{
  unsigned char r;
  r = sw_m512a(); puts("sw_m512a "); puthex(r); putc('\n'); CHECK(1, r == 0x5a);
  r = sw_m512m(); puts("sw_m512m "); puthex(r); putc('\n'); CHECK(2, r == 0x11);
  r = sw_m512s(); puts("sw_m512s "); puthex(r); putc('\n'); CHECK(3, r == 0x80);
  DONE();
  return 0;
}
