; test_rev3.s - the third chip's 24-bit IX, SP, RA and IA (ttsky26d REV3.md),
; standalone: the reset vector at word 0, the UART at [p:0x10/0x11], ABS areas,
; "ok NN" / "FAIL NN" lines and ALL PASSED / SOME FAILED as sdcc_test's harness
; prints them.  lisa_sim --rev3, the chip testbench with the cache.
	.module test_rev3

	.area CODE (ABS,CODE)
	.org 0
	jal	main			; the reset vector; interrupts unused
	.org 0x40
main:
	ldx	#0xFFFF
	xchg	sp			; SP = 0x00FFFF
	amode	0			; plain shifts
	ldi	#0
	sta	0x1F2			; the fail flag

	; 1: ldxf #imm24 - the top byte with the condition bit, the middle, the low
	ldxf	#0x8A1234
	txah
	cpi	#0x8A
	ldac	eq
	sta	0x1F1
	ldi	#1
	jal	check
	txau
	cpi	#0x12
	ldac	eq
	sta	0x1F1
	ldi	#2
	jal	check
	txa
	cpi	#0x34
	ldac	eq
	sta	0x1F1
	ldi	#3
	jal	check

	; 4: ldx #imm16 is a near value: the bank 0, the condition bit set
	ldx	#0xBEEF
	txah
	cpi	#0x80
	ldac	eq
	sta	0x1F1
	ldi	#4
	jal	check

	; 5: taxh / taxu / tax build a pointer byte by byte
	ldi	#0x7A
	taxh
	ldi	#0x5B
	taxu
	ldi	#0x3C
	tax
	txah
	cpi	#0x7A
	ldac	eq
	sta	0x1F1
	ldi	#5
	jal	check

	; 6, 7: push ix / pop ix move three bytes, {cond, 22:16} at the lowest address
	push	ix			; 0x7A5B3C with the condition bit clear
	ldax	0(sp)
	cpi	#0x7A
	ldac	eq
	sta	0x1F1
	ldi	#6
	jal	check
	ldax	2(sp)
	cpi	#0x3C
	ldac	eq
	sta	0x1F1
	ldi	#7
	jal	check
	ldxf	#0
	pop	ix
	txau
	cpi	#0x5B
	ldac	eq
	sta	0x1F1
	ldi	#8
	jal	check

	; 9, 10: stxx / ldxx: three bytes little-endian at n(sp)
	ads	#-4
	ldxf	#0x912345		; the condition bit set
	stxx	1(sp)
	ldax	1(sp)
	cpi	#0x45
	ldac	eq
	sta	0x1F1
	ldi	#9
	jal	check
	ldax	3(sp)
	cpi	#0x91
	ldac	eq
	sta	0x1F1
	ldi	#10
	jal	check
	ldxf	#0
	ldxx	1(sp)
	txah
	cpi	#0x91
	ldac	eq
	sta	0x1F1
	ldi	#11
	jal	check
	ads	#4

	; 12: a far byte at 1 MB does not alias the near byte at 0x100
	ldi	#0x11
	sta	0x100
	ldxf	#0x100000
	ldi	#0x22
	stax	0(ix)
	ldxf	#0x100000
	ldax	0(ix)
	cpi	#0x22
	ldac	eq
	sta	0x1F1
	ldi	#12
	jal	check
	lda	0x100
	cpi	#0x11
	ldac	eq
	sta	0x1F1
	ldi	#13
	jal	check

	; 14: sra pushes {cond, bank, 0, pc} in three bytes: the routine checks its own RA
	jal	rasub			; the link is in bank 0, word rasub + 1

	; 15, 16: call ix into code bank 2 (word 0x4000 of bank 2 = flash byte 0x10000):
	; the routine returns its bank in A and ret brings the code bank back to 0
	ldxf	#0x824000		; {1, bank 2, pc 0x4000}
	call	ix
	cpi	#0x22
	ldac	eq
	sta	0x1F1
	ldi	#15
	jal	check
	lda	0x215			; CB: back to its value before the call (1, the reset value)
	cpi	#1
	ldac	eq
	sta	0x1F1
	ldi	#16
	jal	check

	; 17: lda (ix++) reads a constant in bank 2 through the pointer's own bank:
	; the word at 0x4008 of bank 2 holds "AB"
	ldxf	#0x828010		; {1, bank 2, (0x4008 << 1)}
	lda	(ix++)
	cpi	#0x41
	ldac	eq
	sta	0x1F1
	ldi	#17
	jal	check
	lda	(ix++)
	cpi	#0x42
	ldac	eq
	sta	0x1F1
	ldi	#18
	jal	check

	; 19: a jump through a 24-bit pointer and back: jmp ix to bank 2's word 0x4002
	ldxf	#0x824002
	call	ix			; the routine there jumps back to bank 0 with jmp ix
	cpi	#0x33
	ldac	eq
	sta	0x1F1
	ldi	#19
	jal	check

	; done
	lda	0x1F2
	cpi	#0
	bz	passed
	ldxf	#0x801016		; s_failed
	jal	puts
	br	halt
passed:
	ldxf	#0x80100A		; s_passed
	jal	puts
halt:
	br	halt

; the subroutine of check 14: its RA is {0 cond, bank 0, 0, the return word}
rasub:
	sra
	ldax	0(sp)			; {cond, bank}: the call was unconditional, the code bank 1 (its reset value)
	cpi	#0x81
	ldac	eq
	sta	0x1F1
	ldi	#14
	jal	check
	lra
	ret

; check: A = the number, 0x101 = 1 for a pass; prints "ok NN" or "FAIL NN"
check:
	sra
	push	ix			; the checks keep IX across (three bytes here)
	sta	0x1F3
	lda	0x1F1
	cpi	#1
	bz	1$
	ldi	#1
	sta	0x1F2			; a failure
	ldxf	#0x801004		; s_fail
	jal	puts
	br	2$
1$:	ldxf	#0x801000		; s_ok (byte 0x1000)
	jal	puts
2$:	lda	0x1F3
	shr
	shr
	shr
	shr
	jal	hexdigit
	lda	0x1F3
	andi	#0x0F
	jal	hexdigit
	ldi	#10
	jal	putc
	pop	ix
	lra
	ret

hexdigit:				; A = 0..15: the digit from a table in the code space
	sra
	sta	0x1F4
	ldxf	#0x801023		; s_hex
	lda	0x1F4
	addax
	lda	(ix++)
	jal	putc
	lra
	ret

; puts: IX = a 24-bit code pointer to a NUL-terminated string (lda (ix++))
puts:
	sra
1$:	lda	(ix++)
	bz	2$
	jal	putc
	br	1$
2$:	lra
	ret

; putc: A = the char; waits for the transmitter
putc:
	sta	0x1F0
1$:	lda	0x211
	andi	#2
	bz	1$
	lda	0x1F0
	sta	0x210
	ret

	.area KSTR (ABS,CODE,KDATA)		; packed bytes, two to a word: lda (ix++) reads them
	.org 0x1000
s_ok:		.ascii	"ok "
		.db	0
s_fail:		.ascii	"FAIL "
		.db	0
s_passed:	.ascii	"ALL PASSED"
		.db	10, 0
s_failed:	.ascii	"SOME FAILED"
		.db	10, 0
s_hex:		.ascii	"0123456789abcdef"

; code bank 2: word 0x4000 = flash byte 0x10000
	.area FAR2 (ABS,CODE)		; code bank 2: byte 0x10000 = word 0x8000 = window word 0x4000
	.org 0x10000
	ldi	#0x22			; word 0x4000: returns its bank
	ret
	ldi	#0x33			; word 0x4002: back to bank 0 through a pointer
	ldxf	#0x800C00		; back (word 0xC00 of bank 0)
	jmp	ix

	.area KFAR2 (ABS,CODE,KDATA)	; packed constants in bank 2: byte 0x10010 = word 0x8008 = window word 0x4008
	.org 0x10010
	.ascii	"AB"

	.area CODE2 (ABS,CODE)
	.org 0x1800			; byte 0x1800 = word 0xC00
back:
	ret				; RA is still the caller's: bank 0
