#include "harness.h"
volatile unsigned char r_a, ral, rah;
static void show(const char *name)
{
  puts(name); putc('='); puthex(r_a); puts(" ra="); puthex(rah); puthex(ral); putc('\n');
}
int main(void)
{
  __asm
	sra
	ldi	#0x01
	push	a
	ldi	#0x50
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0150/0001");
  __asm
	sra
	ldi	#0x01
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0100/0001");
  __asm
	sra
	ldi	#0x00
	push	a
	ldi	#0xff
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("00ff/0001");
  __asm
	sra
	ldi	#0x7f
	push	a
	ldi	#0xff
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("7fff/0001");
  __asm
	sra
	ldi	#0xff
	push	a
	ldi	#0xff
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("ffff/0001");
  __asm
	sra
	ldi	#0x80
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("8000/0001");
  __asm
	sra
	ldi	#0x01
	push	a
	ldi	#0x01
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0101/0001");
  __asm
	sra
	ldi	#0xc3
	push	a
	ldi	#0x50
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x01
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("c350/0001");
  __asm
	sra
	ldi	#0x12
	push	a
	ldi	#0x40
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x03
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1240/0003");
  __asm
	sra
	ldi	#0x12
	push	a
	ldi	#0x40
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x05
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1240/0005");
  __asm
	sra
	ldi	#0x12
	push	a
	ldi	#0x40
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x07
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1240/0007");
  __asm
	sra
	ldi	#0x12
	push	a
	ldi	#0x40
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x09
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1240/0009");
  __asm
	sra
	ldi	#0x02
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x02
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0200/0002");
  __asm
	sra
	ldi	#0x02
	push	a
	ldi	#0x01
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x02
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0201/0002");
  __asm
	sra
	ldi	#0x04
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x04
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0400/0004");
  __asm
	sra
	ldi	#0x04
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x03
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0400/0003");
  __asm
	sra
	ldi	#0x05
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x05
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0500/0005");
  __asm
	sra
	ldi	#0x06
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x06
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0600/0006");
  __asm
	sra
	ldi	#0x07
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x07
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0700/0007");
  __asm
	sra
	ldi	#0x08
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x08
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0800/0008");
  __asm
	sra
	ldi	#0x09
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x09
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("0900/0009");
  __asm
	sra
	ldi	#0x10
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x10
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1000/0010");
  __asm
	sra
	ldi	#0x20
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x10
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("2000/0010");
  __asm
	sra
	ldi	#0x10
	push	a
	ldi	#0x00
	lddiv	1(sp)
	ads	#1
	ldi	#0x00
	push	a
	push	a
	ldi	#0x11
	div	0, 2(sp)
	ads	#2
	sta	_r_a
	xchg	ra
	txa
	sta	_ral
	txau
	sta	_rah
	xchg	ra
	lra
  __endasm;
  show("1000/0011");
  puts("ALL PASSED\n");
  return 0;
}
