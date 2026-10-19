/* test_ldk2.c - the words test_ldk reads from bank 2 (--codeseg BANK2) */
void pattern2(void) __banked __naked
{
  __asm
	ldi	#0x56
	ldi	#0x78
	ldi	#0xCD
	ret
  __endasm;
}
