                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_varargs
                                      6 	
                                      7 	.optsdcc -mlisa
                                      8 
                                      9 ; default segment ordering in RAM for linker
                                     10 	.area DATA
                                     11 	.area OSEG (OVR,DATA)
                                     12 
                                     13 ;--------------------------------------------------------
                                     14 ; Public variables in this module
                                     15 ;--------------------------------------------------------
                                     16 	.globl _main
                                     17 	.globl _dig
                                     18 	.globl _va
                                     19 	.globl _f2
                                     20 	.globl _UART_STATUS
                                     21 	.globl _UART_TX
                                     22 ;--------------------------------------------------------
                                     23 ; special function registers
                                     24 ;--------------------------------------------------------
                                     25 	.area RSEG (ABS)
      000000                         26 	.org 0x0000
                           000210    27 _UART_TX	=	0x0210
                           000211    28 _UART_STATUS	=	0x0211
                                     29 ;--------------------------------------------------------
                                     30 ; ram data
                                     31 ;--------------------------------------------------------
                                     32 	.area DATA
      000000                         33 _fails:
      000000                         34 	.ds 1
                                     35 ;--------------------------------------------------------
                                     36 ; ram data
                                     37 ;--------------------------------------------------------
                                     38 	.area INITIALIZED
                                     39 ;--------------------------------------------------------
                                     40 ; overlayable items in ram
                                     41 ;--------------------------------------------------------
                                     42 ;--------------------------------------------------------
                                     43 ; Stack segment in internal ram
                                     44 ;--------------------------------------------------------
                                     45 	.area SSEG
      000001                         46 __start__stack:
      000001                         47 	.ds	1
                                     48 
                                     49 ;--------------------------------------------------------
                                     50 ; absolute external ram data
                                     51 ;--------------------------------------------------------
                                     52 	.area DABS (ABS)
                                     53 ;--------------------------------------------------------
                                     54 ; interrupt vector
                                     55 ;--------------------------------------------------------
                                     56 	.area HOME (CODE)
      000000                         57 __interrupt_vect:
      000000 0C 00                   58 	jal	__sdcc_gsinit_startup
      000002 40 8B                   59 	rets
      000004 40 8B                   60 	rets
      000006 40 8B                   61 	rets
      000008 40 8B                   62 	rets
      00000A 40 8B                   63 	rets
      00000C 40 8B                   64 	rets
      00000E 40 8B                   65 	rets
      000010 40 8B                   66 	rets
      000012 40 8B                   67 	rets
                                     68 ;--------------------------------------------------------
                                     69 ; global & static initialisations
                                     70 ;--------------------------------------------------------
                                     71 	.area HOME (CODE)
                                     72 	.area GSINIT (CODE)
                                     73 	.area GSFINAL (CODE)
                                     74 	.area GSINIT (CODE)
                                     75 	.area GSINIT (CODE)
      000018                         76 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             77 	ldx	#0x007f
      00001C C8 8A                   78 	xchg	sp
      00001E 41 A1                   79 	amode	1
      000020 95 03                   80 	jal	___sdcc_external_startup
      000022 00 A4                   81 	cpi	#0
      000024 01 A2                   82 	if	ne
      000026 0A 00                   83 	jal	__sdcc_program_startup
      000028 00 80                   84 	ldi	#>l_DATA
      00002A 80 A0                   85 	push	a
      00002C 01 80                   86 	ldi	#<l_DATA
      00002E 80 A0                   87 	push	a
      000030 80 A1 00 00             88 	ldx	#s_DATA
      000034                         89 00001$:
      000034 01 F2                   90 	ldax	1(sp)
      000036 02 DA                   91 	or	2(sp)
      000038 08 B8                   92 	bz	00002$
      00003A 00 80                   93 	ldi	#0
      00003C 00 F8                   94 	stax	0(ix)
      00003E 01 98                   95 	adx	#1
      000040 01 9E                   96 	dcx	1(sp)
      000042 03 A2                   97 	if	c
      000044 02 9E                   98 	dcx	2(sp)
      000046 F7 B7                   99 	br	00001$
      000048                        100 00002$:
      000048 00 80                  101 	ldi	#>s_INITIALIZED
      00004A 80 A0                  102 	push	a
      00004C 01 80                  103 	ldi	#<s_INITIALIZED
      00004E 80 A0                  104 	push	a
      000050 00 80                  105 	ldi	#>l_INITIALIZED
      000052 04 FA                  106 	stax	4(sp)
      000054 00 80                  107 	ldi	#<l_INITIALIZED
      000056 03 FA                  108 	stax	3(sp)
      000058 80 A1 00 84            109 	ldx	#s_INITIALIZER
      00005C                        110 00003$:
      00005C 03 F2                  111 	ldax	3(sp)
      00005E 04 DA                  112 	or	4(sp)
      000060 0E B8                  113 	bz	00004$
      000062 80 8A                  114 	call	ix
      000064 01 98                  115 	adx	#1
      000066 68 A1                  116 	push	ix
      000068 03 CC                  117 	ldxx	3(sp)
      00006A 00 F8                  118 	stax	0(ix)
      00006C 03 E6                  119 	inx	3(sp)
      00006E 03 A2                  120 	if	c
      000070 04 E6                  121 	inx	4(sp)
      000072 6C A1                  122 	pop	ix
      000074 03 9E                  123 	dcx	3(sp)
      000076 03 A2                  124 	if	c
      000078 04 9E                  125 	dcx	4(sp)
      00007A F1 B7                  126 	br	00003$
      00007C                        127 00004$:
      00007C 04 94                  128 	ads	#4
                                    129 	.area GSFINAL (CODE)
      00007E 0A 00                  130 	jal	__sdcc_program_startup
                                    131 ;--------------------------------------------------------
                                    132 ; Home
                                    133 ;--------------------------------------------------------
                                    134 	.area HOME (CODE)
                                    135 	.area HOME (CODE)
      000014                        136 __sdcc_program_startup:
      000014 85 02                  137 	jal	_main
      000016                        138 00001$:
      000016 00 B0                  139 	br	00001$
                                    140 ;	return from main will return to caller
                                    141 ;--------------------------------------------------------
                                    142 ; code
                                    143 ;--------------------------------------------------------
                                    144 	.area CODE (CODE)
                                    145 ;	harness.h: 15: static void putc(char c)
                                    146 ;	---------------------------------
                                    147 ;	 Function putc
                                    148 ;	---------------------------------
      000080                        149 _putc:
      000080 FF 97                  150 	ads	#-1
                                    151 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        152 00101$:
      000082 11 F6                  153 	lda	_UART_STATUS
      000084 01 FA                  154 	stax	1(sp)
      000086 02 D4                  155 	andi	#0x02
      000088 FD BF                  156 	bz	00101$
                                    157 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  158 	ldax	2(sp)
      00008C 10 FE                  159 	sta	_UART_TX
      00008E                        160 00104$:
                                    161 ;	harness.h: 20: }
      00008E 01 94                  162 	ads	#1
      000090 00 8A                  163 	ret
                                    164 ;	harness.h: 22: static void puts(const char *s)
                                    165 ;	---------------------------------
                                    166 ;	 Function puts
                                    167 ;	---------------------------------
      000092                        168 _puts:
      000092 60 A1                  169 	sra
      000094 FD 97                  170 	ads	#-3
                                    171 ;	harness.h: 24: while (*s)
      000096 06 F2                  172 	ldax	6(sp)
      000098 02 FA                  173 	stax	2(sp)
      00009A 07 F2                  174 	ldax	7(sp)
      00009C 03 FA                  175 	stax	3(sp)
      00009E                        176 00101$:
      00009E 02 CC                  177 	ldxx	2(sp)
      0000A0 18 A0                  178 	txau
      0000A2 47 A0                  179 	btst	7
      0000A4 04 B8                  180 	bz	00119$
      0000A6 00 F0                  181 	ldax	0(ix)
      0000A8 01 FA                  182 	stax	1(sp)
      0000AA 08 B0                  183 	br	00120$
      0000AC                        184 00119$:
      0000AC 18 A0                  185 	txau
      0000AE 7F D4                  186 	andi	#0x7f
      0000B0 A1 A1                  187 	addaxu
      0000B2 10 A0                  188 	txa
      0000B4 A0 A1                  189 	addax
      0000B6 80 8A                  190 	call	ix
      0000B8 01 FA                  191 	stax	1(sp)
      0000BA                        192 00120$:
      0000BA 01 F2                  193 	ldax	1(sp)
      0000BC 09 B8                  194 	bz	00104$
                                    195 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  196 	inx	2(sp)
      0000C0 03 A2                  197 	if	c
      0000C2 03 E6                  198 	inx.p	3(sp)
      0000C4 01 F2                  199 	ldax	1(sp)
      0000C6 80 A0                  200 	push	a
      0000C8 40 00                  201 	jal	_putc
      0000CA 01 94                  202 	ads	#1
      0000CC E9 B7                  203 	br	00101$
      0000CE                        204 00104$:
                                    205 ;	harness.h: 26: }
      0000CE 03 94                  206 	ads	#3
      0000D0 64 A1                  207 	lra
      0000D2 00 8A                  208 	ret
                                    209 ;	harness.h: 28: static void puthex(unsigned int v)
                                    210 ;	---------------------------------
                                    211 ;	 Function puthex
                                    212 ;	---------------------------------
      0000D4                        213 _puthex:
      0000D4 60 A1                  214 	sra
      0000D6 FD 97                  215 	ads	#-3
                                    216 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    217 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  218 	ldax	7(sp)
      0000DA 02 FA                  219 	stax	2(sp)
      0000DC 00 80                  220 	ldi	#0x00
      0000DE 03 FA                  221 	stax	3(sp)
      0000E0 08 A0                  222 	ldc	#0
      0000E2 04 A0                  223 	shr
      0000E4 03 FA                  224 	stax	3(sp)
      0000E6 02 F2                  225 	ldax	2(sp)
      0000E8 04 A0                  226 	shr
      0000EA 02 FA                  227 	stax	2(sp)
      0000EC 03 F2                  228 	ldax	3(sp)
      0000EE 08 A0                  229 	ldc	#0
      0000F0 04 A0                  230 	shr
      0000F2 03 FA                  231 	stax	3(sp)
      0000F4 02 F2                  232 	ldax	2(sp)
      0000F6 04 A0                  233 	shr
      0000F8 02 FA                  234 	stax	2(sp)
      0000FA 03 F2                  235 	ldax	3(sp)
      0000FC 08 A0                  236 	ldc	#0
      0000FE 04 A0                  237 	shr
      000100 03 FA                  238 	stax	3(sp)
      000102 02 F2                  239 	ldax	2(sp)
      000104 04 A0                  240 	shr
      000106 02 FA                  241 	stax	2(sp)
      000108 03 F2                  242 	ldax	3(sp)
      00010A 08 A0                  243 	ldc	#0
      00010C 04 A0                  244 	shr
      00010E 03 FA                  245 	stax	3(sp)
      000110 02 F2                  246 	ldax	2(sp)
      000112 04 A0                  247 	shr
      000114 02 FA                  248 	stax	2(sp)
      000116 0F D4                  249 	andi	#0x0f
      000118 02 FA                  250 	stax	2(sp)
      00011A 00 80                  251 	ldi	#0x00
      00011C 03 FA                  252 	stax	3(sp)
      00011E 02 F2                  253 	ldax	2(sp)
      000120 08 A0                  254 	ldc	#0
      000122 CC 90                  255 	adc	#<(___str_0 + 0)
      000124 02 FA                  256 	stax	2(sp)
      000126 03 F2                  257 	ldax	3(sp)
      000128 81 90                  258 	adc	#>(___str_0 + 0)
      00012A 03 FA                  259 	stax	3(sp)
      00012C 02 CC                  260 	ldxx	2(sp)
      00012E 18 A0                  261 	txau
      000130 47 A0                  262 	btst	7
      000132 04 B8                  263 	bz	00103$
      000134 00 F0                  264 	ldax	0(ix)
      000136 01 FA                  265 	stax	1(sp)
      000138 08 B0                  266 	br	00104$
      00013A                        267 00103$:
      00013A 18 A0                  268 	txau
      00013C 7F D4                  269 	andi	#0x7f
      00013E A1 A1                  270 	addaxu
      000140 10 A0                  271 	txa
      000142 A0 A1                  272 	addax
      000144 80 8A                  273 	call	ix
      000146 01 FA                  274 	stax	1(sp)
      000148                        275 00104$:
      000148 01 F2                  276 	ldax	1(sp)
      00014A 80 A0                  277 	push	a
      00014C 40 00                  278 	jal	_putc
      00014E 01 94                  279 	ads	#1
                                    280 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  281 	ldax	7(sp)
      000152 02 FA                  282 	stax	2(sp)
      000154 00 80                  283 	ldi	#0x00
      000156 03 FA                  284 	stax	3(sp)
      000158 02 F2                  285 	ldax	2(sp)
      00015A 0F D4                  286 	andi	#0x0f
      00015C 02 FA                  287 	stax	2(sp)
      00015E 00 80                  288 	ldi	#0x00
      000160 03 FA                  289 	stax	3(sp)
      000162 02 F2                  290 	ldax	2(sp)
      000164 08 A0                  291 	ldc	#0
      000166 CC 90                  292 	adc	#<(___str_0 + 0)
      000168 02 FA                  293 	stax	2(sp)
      00016A 03 F2                  294 	ldax	3(sp)
      00016C 81 90                  295 	adc	#>(___str_0 + 0)
      00016E 03 FA                  296 	stax	3(sp)
      000170 02 CC                  297 	ldxx	2(sp)
      000172 18 A0                  298 	txau
      000174 47 A0                  299 	btst	7
      000176 04 B8                  300 	bz	00105$
      000178 00 F0                  301 	ldax	0(ix)
      00017A 01 FA                  302 	stax	1(sp)
      00017C 08 B0                  303 	br	00106$
      00017E                        304 00105$:
      00017E 18 A0                  305 	txau
      000180 7F D4                  306 	andi	#0x7f
      000182 A1 A1                  307 	addaxu
      000184 10 A0                  308 	txa
      000186 A0 A1                  309 	addax
      000188 80 8A                  310 	call	ix
      00018A 01 FA                  311 	stax	1(sp)
      00018C                        312 00106$:
      00018C 01 F2                  313 	ldax	1(sp)
      00018E 80 A0                  314 	push	a
      000190 40 00                  315 	jal	_putc
      000192 01 94                  316 	ads	#1
                                    317 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  318 	ldax	6(sp)
      000196 02 FA                  319 	stax	2(sp)
      000198 07 F2                  320 	ldax	7(sp)
      00019A 03 FA                  321 	stax	3(sp)
      00019C 08 A0                  322 	ldc	#0
      00019E 04 A0                  323 	shr
      0001A0 03 FA                  324 	stax	3(sp)
      0001A2 02 F2                  325 	ldax	2(sp)
      0001A4 04 A0                  326 	shr
      0001A6 02 FA                  327 	stax	2(sp)
      0001A8 03 F2                  328 	ldax	3(sp)
      0001AA 08 A0                  329 	ldc	#0
      0001AC 04 A0                  330 	shr
      0001AE 03 FA                  331 	stax	3(sp)
      0001B0 02 F2                  332 	ldax	2(sp)
      0001B2 04 A0                  333 	shr
      0001B4 02 FA                  334 	stax	2(sp)
      0001B6 03 F2                  335 	ldax	3(sp)
      0001B8 08 A0                  336 	ldc	#0
      0001BA 04 A0                  337 	shr
      0001BC 03 FA                  338 	stax	3(sp)
      0001BE 02 F2                  339 	ldax	2(sp)
      0001C0 04 A0                  340 	shr
      0001C2 02 FA                  341 	stax	2(sp)
      0001C4 03 F2                  342 	ldax	3(sp)
      0001C6 08 A0                  343 	ldc	#0
      0001C8 04 A0                  344 	shr
      0001CA 03 FA                  345 	stax	3(sp)
      0001CC 02 F2                  346 	ldax	2(sp)
      0001CE 04 A0                  347 	shr
      0001D0 02 FA                  348 	stax	2(sp)
      0001D2 0F D4                  349 	andi	#0x0f
      0001D4 02 FA                  350 	stax	2(sp)
      0001D6 00 80                  351 	ldi	#0x00
      0001D8 03 FA                  352 	stax	3(sp)
      0001DA 02 F2                  353 	ldax	2(sp)
      0001DC 08 A0                  354 	ldc	#0
      0001DE CC 90                  355 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  356 	stax	2(sp)
      0001E2 03 F2                  357 	ldax	3(sp)
      0001E4 81 90                  358 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  359 	stax	3(sp)
      0001E8 02 CC                  360 	ldxx	2(sp)
      0001EA 18 A0                  361 	txau
      0001EC 47 A0                  362 	btst	7
      0001EE 04 B8                  363 	bz	00107$
      0001F0 00 F0                  364 	ldax	0(ix)
      0001F2 01 FA                  365 	stax	1(sp)
      0001F4 08 B0                  366 	br	00108$
      0001F6                        367 00107$:
      0001F6 18 A0                  368 	txau
      0001F8 7F D4                  369 	andi	#0x7f
      0001FA A1 A1                  370 	addaxu
      0001FC 10 A0                  371 	txa
      0001FE A0 A1                  372 	addax
      000200 80 8A                  373 	call	ix
      000202 01 FA                  374 	stax	1(sp)
      000204                        375 00108$:
      000204 01 F2                  376 	ldax	1(sp)
      000206 80 A0                  377 	push	a
      000208 40 00                  378 	jal	_putc
      00020A 01 94                  379 	ads	#1
                                    380 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  381 	ldax	6(sp)
      00020E 0F D4                  382 	andi	#0x0f
      000210 02 FA                  383 	stax	2(sp)
      000212 00 80                  384 	ldi	#0x00
      000214 03 FA                  385 	stax	3(sp)
      000216 02 F2                  386 	ldax	2(sp)
      000218 08 A0                  387 	ldc	#0
      00021A CC 90                  388 	adc	#<(___str_0 + 0)
      00021C 02 FA                  389 	stax	2(sp)
      00021E 03 F2                  390 	ldax	3(sp)
      000220 81 90                  391 	adc	#>(___str_0 + 0)
      000222 03 FA                  392 	stax	3(sp)
      000224 02 CC                  393 	ldxx	2(sp)
      000226 18 A0                  394 	txau
      000228 47 A0                  395 	btst	7
      00022A 04 B8                  396 	bz	00109$
      00022C 00 F0                  397 	ldax	0(ix)
      00022E 01 FA                  398 	stax	1(sp)
      000230 08 B0                  399 	br	00110$
      000232                        400 00109$:
      000232 18 A0                  401 	txau
      000234 7F D4                  402 	andi	#0x7f
      000236 A1 A1                  403 	addaxu
      000238 10 A0                  404 	txa
      00023A A0 A1                  405 	addax
      00023C 80 8A                  406 	call	ix
      00023E 01 FA                  407 	stax	1(sp)
      000240                        408 00110$:
      000240 01 F2                  409 	ldax	1(sp)
      000242 80 A0                  410 	push	a
      000244 40 00                  411 	jal	_putc
      000246 01 94                  412 	ads	#1
      000248                        413 00101$:
                                    414 ;	harness.h: 35: }
      000248 03 94                  415 	ads	#3
      00024A 64 A1                  416 	lra
      00024C 00 8A                  417 	ret
                                    418 ;	test_varargs.c: 4: long f2(int x) { long l = x; if (l < 0) l = -l; return l; }
                                    419 ;	---------------------------------
                                    420 ;	 Function f2
                                    421 ;	---------------------------------
      00024E                        422 _f2:
      00024E FC 97                  423 	ads	#-4
      000250 09 F2                  424 	ldax	9(sp)
      000252 01 FA                  425 	stax	1(sp)
      000254 0A F2                  426 	ldax	10(sp)
      000256 02 FA                  427 	stax	2(sp)
      000258 0A F2                  428 	ldax	10(sp)
      00025A 00 A0                  429 	shl
      00025C 13 A2                  430 	ifte	c
      00025E FF 80                  431 	ldi.p	#0xff
      000260 00 80                  432 	ldi.p	#0x00
      000262 03 FA                  433 	stax	3(sp)
      000264 04 FA                  434 	stax	4(sp)
      000266 00 80                  435 	ldi	#0x00
      000268 04 EA                  436 	cmp	4(sp)
      00026A 24 A2                  437 	if	sgt
      00026C 14 B0                  438 	br.p	00111$
      00026E 25 A2                  439 	if	slt
      000270 11 B0                  440 	br.p	00112$
      000272 00 80                  441 	ldi	#0x00
      000274 03 EA                  442 	cmp	3(sp)
      000276 04 A2                  443 	if	gt
      000278 0E B0                  444 	br.p	00111$
      00027A 05 A2                  445 	if	lt
      00027C 0B B0                  446 	br.p	00112$
      00027E 00 80                  447 	ldi	#0x00
      000280 02 EA                  448 	cmp	2(sp)
      000282 04 A2                  449 	if	gt
      000284 08 B0                  450 	br.p	00111$
      000286 05 A2                  451 	if	lt
      000288 05 B0                  452 	br.p	00112$
      00028A 00 80                  453 	ldi	#0x00
      00028C 01 EA                  454 	cmp	1(sp)
      00028E 04 A2                  455 	if	gt
      000290 02 B0                  456 	br.p	00111$
      000292                        457 00112$:
      000292 13 B0                  458 	br	00102$
      000294                        459 00111$:
      000294 00 80                  460 	ldi	#0x00
      000296 01 CA                  461 	sub	1(sp)
      000298 01 FA                  462 	stax	1(sp)
      00029A 78 A1                  463 	savec
      00029C 00 80                  464 	ldi	#0x00
      00029E 7C A1                  465 	restc
      0002A0 02 CA                  466 	sub	2(sp)
      0002A2 02 FA                  467 	stax	2(sp)
      0002A4 78 A1                  468 	savec
      0002A6 00 80                  469 	ldi	#0x00
      0002A8 7C A1                  470 	restc
      0002AA 03 CA                  471 	sub	3(sp)
      0002AC 03 FA                  472 	stax	3(sp)
      0002AE 78 A1                  473 	savec
      0002B0 00 80                  474 	ldi	#0x00
      0002B2 7C A1                  475 	restc
      0002B4 04 CA                  476 	sub	4(sp)
      0002B6 04 FA                  477 	stax	4(sp)
      0002B8                        478 00102$:
      0002B8 01 F2                  479 	ldax	1(sp)
      0002BA 05 FA                  480 	stax	5(sp)
      0002BC 02 F2                  481 	ldax	2(sp)
      0002BE 06 FA                  482 	stax	6(sp)
      0002C0 03 F2                  483 	ldax	3(sp)
      0002C2 07 FA                  484 	stax	7(sp)
      0002C4 04 F2                  485 	ldax	4(sp)
      0002C6 08 FA                  486 	stax	8(sp)
      0002C8                        487 00103$:
      0002C8 04 94                  488 	ads	#4
      0002CA 00 8A                  489 	ret
                                    490 ;	test_varargs.c: 5: long va(int n, ...) {
                                    491 ;	---------------------------------
                                    492 ;	 Function va
                                    493 ;	---------------------------------
      0002CC                        494 _va:
      0002CC 60 A1                  495 	sra
      0002CE F5 97                  496 	ads	#-11
                                    497 ;	test_varargs.c: 7: va_start(ap, n);
                                    498 ;	test_varargs.c: 8: value.l = va_arg(ap, int);
      0002D0 CC 8A                  499 	spix
      0002D2 14 98                  500 	adx	#20
      0002D4 00 F0                  501 	ldax	0(ix)
      0002D6 0A FA                  502 	stax	10(sp)
      0002D8 01 F0                  503 	ldax	1(ix)
      0002DA 0B FA                  504 	stax	11(sp)
      0002DC 0A F2                  505 	ldax	10(sp)
      0002DE 06 FA                  506 	stax	6(sp)
      0002E0 0B F2                  507 	ldax	11(sp)
      0002E2 07 FA                  508 	stax	7(sp)
      0002E4 0B F2                  509 	ldax	11(sp)
      0002E6 00 A0                  510 	shl
      0002E8 13 A2                  511 	ifte	c
      0002EA FF 80                  512 	ldi.p	#0xff
      0002EC 00 80                  513 	ldi.p	#0x00
      0002EE 08 FA                  514 	stax	8(sp)
      0002F0 09 FA                  515 	stax	9(sp)
      0002F2 CC 8A                  516 	spix
      0002F4 01 98                  517 	adx	#1
      0002F6 06 F2                  518 	ldax	6(sp)
      0002F8 00 F8                  519 	stax	0(ix)
      0002FA 07 F2                  520 	ldax	7(sp)
      0002FC 01 F8                  521 	stax	1(ix)
      0002FE 08 F2                  522 	ldax	8(sp)
      000300 02 F8                  523 	stax	2(ix)
      000302 09 F2                  524 	ldax	9(sp)
      000304 03 F8                  525 	stax	3(ix)
                                    526 ;	test_varargs.c: 9: if (value.l < 0) value.l = -value.l;
      000306 00 80                  527 	ldi	#0x00
      000308 09 EA                  528 	cmp	9(sp)
      00030A 24 A2                  529 	if	sgt
      00030C 14 B0                  530 	br.p	00111$
      00030E 25 A2                  531 	if	slt
      000310 11 B0                  532 	br.p	00112$
      000312 00 80                  533 	ldi	#0x00
      000314 08 EA                  534 	cmp	8(sp)
      000316 04 A2                  535 	if	gt
      000318 0E B0                  536 	br.p	00111$
      00031A 05 A2                  537 	if	lt
      00031C 0B B0                  538 	br.p	00112$
      00031E 00 80                  539 	ldi	#0x00
      000320 07 EA                  540 	cmp	7(sp)
      000322 04 A2                  541 	if	gt
      000324 08 B0                  542 	br.p	00111$
      000326 05 A2                  543 	if	lt
      000328 05 B0                  544 	br.p	00112$
      00032A 00 80                  545 	ldi	#0x00
      00032C 06 EA                  546 	cmp	6(sp)
      00032E 04 A2                  547 	if	gt
      000330 02 B0                  548 	br.p	00111$
      000332                        549 00112$:
      000332 25 B0                  550 	br	00102$
      000334                        551 00111$:
      000334 CC 8A                  552 	spix
      000336 01 98                  553 	adx	#1
      000338 00 F0                  554 	ldax	0(ix)
      00033A 06 FA                  555 	stax	6(sp)
      00033C 01 F0                  556 	ldax	1(ix)
      00033E 07 FA                  557 	stax	7(sp)
      000340 02 F0                  558 	ldax	2(ix)
      000342 08 FA                  559 	stax	8(sp)
      000344 03 F0                  560 	ldax	3(ix)
      000346 09 FA                  561 	stax	9(sp)
      000348 00 80                  562 	ldi	#0x00
      00034A 06 CA                  563 	sub	6(sp)
      00034C 06 FA                  564 	stax	6(sp)
      00034E 78 A1                  565 	savec
      000350 00 80                  566 	ldi	#0x00
      000352 7C A1                  567 	restc
      000354 07 CA                  568 	sub	7(sp)
      000356 07 FA                  569 	stax	7(sp)
      000358 78 A1                  570 	savec
      00035A 00 80                  571 	ldi	#0x00
      00035C 7C A1                  572 	restc
      00035E 08 CA                  573 	sub	8(sp)
      000360 08 FA                  574 	stax	8(sp)
      000362 78 A1                  575 	savec
      000364 00 80                  576 	ldi	#0x00
      000366 7C A1                  577 	restc
      000368 09 CA                  578 	sub	9(sp)
      00036A 09 FA                  579 	stax	9(sp)
      00036C 06 F2                  580 	ldax	6(sp)
      00036E 00 F8                  581 	stax	0(ix)
      000370 07 F2                  582 	ldax	7(sp)
      000372 01 F8                  583 	stax	1(ix)
      000374 08 F2                  584 	ldax	8(sp)
      000376 02 F8                  585 	stax	2(ix)
      000378 09 F2                  586 	ldax	9(sp)
      00037A 03 F8                  587 	stax	3(ix)
      00037C                        588 00102$:
                                    589 ;	test_varargs.c: 10: r = value.l;
      00037C CC 8A                  590 	spix
      00037E 01 98                  591 	adx	#1
      000380 00 F0                  592 	ldax	0(ix)
      000382 06 FA                  593 	stax	6(sp)
      000384 01 F0                  594 	ldax	1(ix)
      000386 07 FA                  595 	stax	7(sp)
      000388 02 F0                  596 	ldax	2(ix)
      00038A 08 FA                  597 	stax	8(sp)
      00038C 03 F0                  598 	ldax	3(ix)
      00038E 09 FA                  599 	stax	9(sp)
                                    600 ;	test_varargs.c: 12: return r;
      000390 06 F2                  601 	ldax	6(sp)
      000392 0E FA                  602 	stax	14(sp)
      000394 07 F2                  603 	ldax	7(sp)
      000396 0F FA                  604 	stax	15(sp)
      000398 08 F2                  605 	ldax	8(sp)
      00039A 10 FA                  606 	stax	16(sp)
      00039C 09 F2                  607 	ldax	9(sp)
      00039E 11 FA                  608 	stax	17(sp)
      0003A0                        609 00103$:
                                    610 ;	test_varargs.c: 13: }
      0003A0 0B 94                  611 	ads	#11
      0003A2 64 A1                  612 	lra
      0003A4 00 8A                  613 	ret
                                    614 ;	test_varargs.c: 14: unsigned char dig(unsigned long ul) {
                                    615 ;	---------------------------------
                                    616 ;	 Function dig
                                    617 ;	---------------------------------
      0003A6                        618 _dig:
      0003A6 60 A1                  619 	sra
      0003A8 F4 97                  620 	ads	#-12
                                    621 ;	test_varargs.c: 15: value_t value; unsigned char i = 32; unsigned char *pb4 = &value.byte[4];
                                    622 ;	test_varargs.c: 16: value.ul = ul; *pb4 = 0;
      0003AA CC 8A                  623 	spix
      0003AC 01 98                  624 	adx	#1
      0003AE 0F F2                  625 	ldax	15(sp)
      0003B0 00 F8                  626 	stax	0(ix)
      0003B2 10 F2                  627 	ldax	16(sp)
      0003B4 01 F8                  628 	stax	1(ix)
      0003B6 11 F2                  629 	ldax	17(sp)
      0003B8 02 F8                  630 	stax	2(ix)
      0003BA 12 F2                  631 	ldax	18(sp)
      0003BC 03 F8                  632 	stax	3(ix)
      0003BE CC 8A                  633 	spix
      0003C0 05 98                  634 	adx	#5
      0003C2 00 80                  635 	ldi	#0x00
      0003C4 00 F8                  636 	stax	0(ix)
                                    637 ;	test_varargs.c: 17: do { *pb4 = (*pb4 << 1) | ((ul >> 31) & 0x01); ul <<= 1; if (10 <= *pb4) { *pb4 -= 10; ul |= 1; } } while (--i);
      0003C6 20 80                  638 	ldi	#0x20
      0003C8 0C FA                  639 	stax	12(sp)
      0003CA                        640 00103$:
      0003CA CC 8A                  641 	spix
      0003CC 05 98                  642 	adx	#5
      0003CE 00 F0                  643 	ldax	0(ix)
      0003D0 0B FA                  644 	stax	11(sp)
      0003D2 08 A0                  645 	ldc	#0
      0003D4 00 A0                  646 	shl
      0003D6 0B FA                  647 	stax	11(sp)
      0003D8 12 F2                  648 	ldax	18(sp)
      0003DA 07 FA                  649 	stax	7(sp)
      0003DC 00 80                  650 	ldi	#0x00
      0003DE 08 FA                  651 	stax	8(sp)
      0003E0 00 80                  652 	ldi	#0x00
      0003E2 09 FA                  653 	stax	9(sp)
      0003E4 00 80                  654 	ldi	#0x00
      0003E6 0A FA                  655 	stax	10(sp)
      0003E8 08 A0                  656 	ldc	#0
      0003EA 04 A0                  657 	shr
      0003EC 0A FA                  658 	stax	10(sp)
      0003EE 09 F2                  659 	ldax	9(sp)
      0003F0 04 A0                  660 	shr
      0003F2 09 FA                  661 	stax	9(sp)
      0003F4 08 F2                  662 	ldax	8(sp)
      0003F6 04 A0                  663 	shr
      0003F8 08 FA                  664 	stax	8(sp)
      0003FA 07 F2                  665 	ldax	7(sp)
      0003FC 04 A0                  666 	shr
      0003FE 07 FA                  667 	stax	7(sp)
      000400 0A F2                  668 	ldax	10(sp)
      000402 08 A0                  669 	ldc	#0
      000404 04 A0                  670 	shr
      000406 0A FA                  671 	stax	10(sp)
      000408 09 F2                  672 	ldax	9(sp)
      00040A 04 A0                  673 	shr
      00040C 09 FA                  674 	stax	9(sp)
      00040E 08 F2                  675 	ldax	8(sp)
      000410 04 A0                  676 	shr
      000412 08 FA                  677 	stax	8(sp)
      000414 07 F2                  678 	ldax	7(sp)
      000416 04 A0                  679 	shr
      000418 07 FA                  680 	stax	7(sp)
      00041A 0A F2                  681 	ldax	10(sp)
      00041C 08 A0                  682 	ldc	#0
      00041E 04 A0                  683 	shr
      000420 0A FA                  684 	stax	10(sp)
      000422 09 F2                  685 	ldax	9(sp)
      000424 04 A0                  686 	shr
      000426 09 FA                  687 	stax	9(sp)
      000428 08 F2                  688 	ldax	8(sp)
      00042A 04 A0                  689 	shr
      00042C 08 FA                  690 	stax	8(sp)
      00042E 07 F2                  691 	ldax	7(sp)
      000430 04 A0                  692 	shr
      000432 07 FA                  693 	stax	7(sp)
      000434 0A F2                  694 	ldax	10(sp)
      000436 08 A0                  695 	ldc	#0
      000438 04 A0                  696 	shr
      00043A 0A FA                  697 	stax	10(sp)
      00043C 09 F2                  698 	ldax	9(sp)
      00043E 04 A0                  699 	shr
      000440 09 FA                  700 	stax	9(sp)
      000442 08 F2                  701 	ldax	8(sp)
      000444 04 A0                  702 	shr
      000446 08 FA                  703 	stax	8(sp)
      000448 07 F2                  704 	ldax	7(sp)
      00044A 04 A0                  705 	shr
      00044C 07 FA                  706 	stax	7(sp)
      00044E 0A F2                  707 	ldax	10(sp)
      000450 08 A0                  708 	ldc	#0
      000452 04 A0                  709 	shr
      000454 0A FA                  710 	stax	10(sp)
      000456 09 F2                  711 	ldax	9(sp)
      000458 04 A0                  712 	shr
      00045A 09 FA                  713 	stax	9(sp)
      00045C 08 F2                  714 	ldax	8(sp)
      00045E 04 A0                  715 	shr
      000460 08 FA                  716 	stax	8(sp)
      000462 07 F2                  717 	ldax	7(sp)
      000464 04 A0                  718 	shr
      000466 07 FA                  719 	stax	7(sp)
      000468 0A F2                  720 	ldax	10(sp)
      00046A 08 A0                  721 	ldc	#0
      00046C 04 A0                  722 	shr
      00046E 0A FA                  723 	stax	10(sp)
      000470 09 F2                  724 	ldax	9(sp)
      000472 04 A0                  725 	shr
      000474 09 FA                  726 	stax	9(sp)
      000476 08 F2                  727 	ldax	8(sp)
      000478 04 A0                  728 	shr
      00047A 08 FA                  729 	stax	8(sp)
      00047C 07 F2                  730 	ldax	7(sp)
      00047E 04 A0                  731 	shr
      000480 07 FA                  732 	stax	7(sp)
      000482 0A F2                  733 	ldax	10(sp)
      000484 08 A0                  734 	ldc	#0
      000486 04 A0                  735 	shr
      000488 0A FA                  736 	stax	10(sp)
      00048A 09 F2                  737 	ldax	9(sp)
      00048C 04 A0                  738 	shr
      00048E 09 FA                  739 	stax	9(sp)
      000490 08 F2                  740 	ldax	8(sp)
      000492 04 A0                  741 	shr
      000494 08 FA                  742 	stax	8(sp)
      000496 07 F2                  743 	ldax	7(sp)
      000498 04 A0                  744 	shr
      00049A 07 FA                  745 	stax	7(sp)
      00049C 06 FA                  746 	stax	6(sp)
      00049E 01 D4                  747 	andi	#0x01
      0004A0 06 FA                  748 	stax	6(sp)
      0004A2 0B F2                  749 	ldax	11(sp)
      0004A4 06 DA                  750 	or	6(sp)
      0004A6 06 FA                  751 	stax	6(sp)
      0004A8 00 F8                  752 	stax	0(ix)
      0004AA 0F F2                  753 	ldax	15(sp)
      0004AC 08 A0                  754 	ldc	#0
      0004AE 0F C2                  755 	add	15(sp)
      0004B0 0F FA                  756 	stax	15(sp)
      0004B2 10 F2                  757 	ldax	16(sp)
      0004B4 10 C2                  758 	add	16(sp)
      0004B6 10 FA                  759 	stax	16(sp)
      0004B8 11 F2                  760 	ldax	17(sp)
      0004BA 11 C2                  761 	add	17(sp)
      0004BC 11 FA                  762 	stax	17(sp)
      0004BE 12 F2                  763 	ldax	18(sp)
      0004C0 12 C2                  764 	add	18(sp)
      0004C2 12 FA                  765 	stax	18(sp)
      0004C4 0A 80                  766 	ldi	#0x0a
      0004C6 06 EA                  767 	cmp	6(sp)
      0004C8 04 A2                  768 	if	gt
      0004CA 0C B0                  769 	br.p	00104$
      0004CC 00 F0                  770 	ldax	0(ix)
      0004CE 06 FA                  771 	stax	6(sp)
      0004D0 09 A0                  772 	ldc	#1
      0004D2 F5 90                  773 	adc	#0xf5
      0004D4 06 FA                  774 	stax	6(sp)
      0004D6 00 F8                  775 	stax	0(ix)
      0004D8 0F F2                  776 	ldax	15(sp)
      0004DA FE D4                  777 	andi	#0xfe
      0004DC 08 A0                  778 	ldc	#0
      0004DE 01 90                  779 	adc	#0x01
      0004E0 0F FA                  780 	stax	15(sp)
      0004E2                        781 00104$:
      0004E2 0C 9E                  782 	dcx	12(sp)
      0004E4 0C F2                  783 	ldax	12(sp)
      0004E6 72 AF                  784 	bnz	00103$
                                    785 ;	test_varargs.c: 18: value.ul = ul;
      0004E8 CC 8A                  786 	spix
      0004EA 01 98                  787 	adx	#1
      0004EC 0F F2                  788 	ldax	15(sp)
      0004EE 00 F8                  789 	stax	0(ix)
      0004F0 10 F2                  790 	ldax	16(sp)
      0004F2 01 F8                  791 	stax	1(ix)
      0004F4 11 F2                  792 	ldax	17(sp)
      0004F6 02 F8                  793 	stax	2(ix)
      0004F8 12 F2                  794 	ldax	18(sp)
      0004FA 03 F8                  795 	stax	3(ix)
                                    796 ;	test_varargs.c: 19: return *pb4;
      0004FC CC 8A                  797 	spix
      0004FE 05 98                  798 	adx	#5
      000500 00 F0                  799 	ldax	0(ix)
      000502 06 FA                  800 	stax	6(sp)
      000504                        801 00106$:
                                    802 ;	test_varargs.c: 20: }
      000504 0C 94                  803 	ads	#12
      000506 64 A1                  804 	lra
      000508 00 8A                  805 	ret
                                    806 ;	test_varargs.c: 21: int main(void) {
                                    807 ;	---------------------------------
                                    808 ;	 Function main
                                    809 ;	---------------------------------
      00050A                        810 _main:
      00050A 60 A1                  811 	sra
      00050C F9 97                  812 	ads	#-7
                                    813 ;	test_varargs.c: 22: CHECK(1, f2(-42) == 42);
      00050E FF 80                  814 	ldi	#0xff
      000510 80 A0                  815 	push	a
      000512 D6 80                  816 	ldi	#0xd6
      000514 80 A0                  817 	push	a
      000516 FC 97                  818 	ads	#-4
      000518 27 01                  819 	jal	_f2
      00051A 01 F2                  820 	ldax	1(sp)
      00051C 0A FA                  821 	stax	10(sp)
      00051E 02 F2                  822 	ldax	2(sp)
      000520 0B FA                  823 	stax	11(sp)
      000522 03 F2                  824 	ldax	3(sp)
      000524 0C FA                  825 	stax	12(sp)
      000526 04 F2                  826 	ldax	4(sp)
      000528 0D FA                  827 	stax	13(sp)
      00052A 06 94                  828 	ads	#6
      00052C 04 F2                  829 	ldax	4(sp)
      00052E 2A A4                  830 	cpi	#0x2a
      000530 09 A8                  831 	bnz	00171$
      000532 05 F2                  832 	ldax	5(sp)
      000534 00 A4                  833 	cpi	#0x00
      000536 06 A8                  834 	bnz	00171$
      000538 06 F2                  835 	ldax	6(sp)
      00053A 00 A4                  836 	cpi	#0x00
      00053C 03 A8                  837 	bnz	00171$
      00053E 07 F2                  838 	ldax	7(sp)
      000540 00 A4                  839 	cpi	#0x00
      000542                        840 00171$:
      000542 08 A8                  841 	bnz	00102$
      000544 81 80                  842 	ldi	#>(___str_1 + 0)
      000546 80 A0                  843 	push	a
      000548 DD 80                  844 	ldi	#<(___str_1 + 0)
      00054A 80 A0                  845 	push	a
      00054C 49 00                  846 	jal	_puts
      00054E 02 94                  847 	ads	#2
      000550 0A B0                  848 	br	00103$
      000552                        849 00102$:
      000552 81 80                  850 	ldi	#>(___str_2 + 0)
      000554 80 A0                  851 	push	a
      000556 E1 80                  852 	ldi	#<(___str_2 + 0)
      000558 80 A0                  853 	push	a
      00055A 49 00                  854 	jal	_puts
      00055C 02 94                  855 	ads	#2
      00055E 80 A1 00 00            856 	ldx	#_fails
      000562 00 E4                  857 	inx	0(ix)
      000564                        858 00103$:
      000564 00 80                  859 	ldi	#0x00
      000566 80 A0                  860 	push	a
      000568 01 80                  861 	ldi	#0x01
      00056A 80 A0                  862 	push	a
      00056C 6A 00                  863 	jal	_puthex
      00056E 02 94                  864 	ads	#2
      000570 0A 80                  865 	ldi	#0x0a
      000572 80 A0                  866 	push	a
      000574 40 00                  867 	jal	_putc
      000576 01 94                  868 	ads	#1
                                    869 ;	test_varargs.c: 23: CHECK(2, va(1, -42) == 42);
      000578 FF 80                  870 	ldi	#0xff
      00057A 80 A0                  871 	push	a
      00057C D6 80                  872 	ldi	#0xd6
      00057E 80 A0                  873 	push	a
      000580 00 80                  874 	ldi	#0x00
      000582 80 A0                  875 	push	a
      000584 01 80                  876 	ldi	#0x01
      000586 80 A0                  877 	push	a
      000588 FC 97                  878 	ads	#-4
      00058A 66 01                  879 	jal	_va
      00058C 01 F2                  880 	ldax	1(sp)
      00058E 0C FA                  881 	stax	12(sp)
      000590 02 F2                  882 	ldax	2(sp)
      000592 0D FA                  883 	stax	13(sp)
      000594 03 F2                  884 	ldax	3(sp)
      000596 0E FA                  885 	stax	14(sp)
      000598 04 F2                  886 	ldax	4(sp)
      00059A 0F FA                  887 	stax	15(sp)
      00059C 08 94                  888 	ads	#8
      00059E 04 F2                  889 	ldax	4(sp)
      0005A0 2A A4                  890 	cpi	#0x2a
      0005A2 09 A8                  891 	bnz	00173$
      0005A4 05 F2                  892 	ldax	5(sp)
      0005A6 00 A4                  893 	cpi	#0x00
      0005A8 06 A8                  894 	bnz	00173$
      0005AA 06 F2                  895 	ldax	6(sp)
      0005AC 00 A4                  896 	cpi	#0x00
      0005AE 03 A8                  897 	bnz	00173$
      0005B0 07 F2                  898 	ldax	7(sp)
      0005B2 00 A4                  899 	cpi	#0x00
      0005B4                        900 00173$:
      0005B4 08 A8                  901 	bnz	00108$
      0005B6 81 80                  902 	ldi	#>(___str_1 + 0)
      0005B8 80 A0                  903 	push	a
      0005BA DD 80                  904 	ldi	#<(___str_1 + 0)
      0005BC 80 A0                  905 	push	a
      0005BE 49 00                  906 	jal	_puts
      0005C0 02 94                  907 	ads	#2
      0005C2 0A B0                  908 	br	00109$
      0005C4                        909 00108$:
      0005C4 81 80                  910 	ldi	#>(___str_2 + 0)
      0005C6 80 A0                  911 	push	a
      0005C8 E1 80                  912 	ldi	#<(___str_2 + 0)
      0005CA 80 A0                  913 	push	a
      0005CC 49 00                  914 	jal	_puts
      0005CE 02 94                  915 	ads	#2
      0005D0 80 A1 00 00            916 	ldx	#_fails
      0005D4 00 E4                  917 	inx	0(ix)
      0005D6                        918 00109$:
      0005D6 00 80                  919 	ldi	#0x00
      0005D8 80 A0                  920 	push	a
      0005DA 02 80                  921 	ldi	#0x02
      0005DC 80 A0                  922 	push	a
      0005DE 6A 00                  923 	jal	_puthex
      0005E0 02 94                  924 	ads	#2
      0005E2 0A 80                  925 	ldi	#0x0a
      0005E4 80 A0                  926 	push	a
      0005E6 40 00                  927 	jal	_putc
      0005E8 01 94                  928 	ads	#1
                                    929 ;	test_varargs.c: 24: CHECK(3, va(1, 42) == 42);
      0005EA 00 80                  930 	ldi	#0x00
      0005EC 80 A0                  931 	push	a
      0005EE 2A 80                  932 	ldi	#0x2a
      0005F0 80 A0                  933 	push	a
      0005F2 00 80                  934 	ldi	#0x00
      0005F4 80 A0                  935 	push	a
      0005F6 01 80                  936 	ldi	#0x01
      0005F8 80 A0                  937 	push	a
      0005FA FC 97                  938 	ads	#-4
      0005FC 66 01                  939 	jal	_va
      0005FE 01 F2                  940 	ldax	1(sp)
      000600 0C FA                  941 	stax	12(sp)
      000602 02 F2                  942 	ldax	2(sp)
      000604 0D FA                  943 	stax	13(sp)
      000606 03 F2                  944 	ldax	3(sp)
      000608 0E FA                  945 	stax	14(sp)
      00060A 04 F2                  946 	ldax	4(sp)
      00060C 0F FA                  947 	stax	15(sp)
      00060E 08 94                  948 	ads	#8
      000610 04 F2                  949 	ldax	4(sp)
      000612 2A A4                  950 	cpi	#0x2a
      000614 09 A8                  951 	bnz	00175$
      000616 05 F2                  952 	ldax	5(sp)
      000618 00 A4                  953 	cpi	#0x00
      00061A 06 A8                  954 	bnz	00175$
      00061C 06 F2                  955 	ldax	6(sp)
      00061E 00 A4                  956 	cpi	#0x00
      000620 03 A8                  957 	bnz	00175$
      000622 07 F2                  958 	ldax	7(sp)
      000624 00 A4                  959 	cpi	#0x00
      000626                        960 00175$:
      000626 08 A8                  961 	bnz	00114$
      000628 81 80                  962 	ldi	#>(___str_1 + 0)
      00062A 80 A0                  963 	push	a
      00062C DD 80                  964 	ldi	#<(___str_1 + 0)
      00062E 80 A0                  965 	push	a
      000630 49 00                  966 	jal	_puts
      000632 02 94                  967 	ads	#2
      000634 0A B0                  968 	br	00115$
      000636                        969 00114$:
      000636 81 80                  970 	ldi	#>(___str_2 + 0)
      000638 80 A0                  971 	push	a
      00063A E1 80                  972 	ldi	#<(___str_2 + 0)
      00063C 80 A0                  973 	push	a
      00063E 49 00                  974 	jal	_puts
      000640 02 94                  975 	ads	#2
      000642 80 A1 00 00            976 	ldx	#_fails
      000646 00 E4                  977 	inx	0(ix)
      000648                        978 00115$:
      000648 00 80                  979 	ldi	#0x00
      00064A 80 A0                  980 	push	a
      00064C 03 80                  981 	ldi	#0x03
      00064E 80 A0                  982 	push	a
      000650 6A 00                  983 	jal	_puthex
      000652 02 94                  984 	ads	#2
      000654 0A 80                  985 	ldi	#0x0a
      000656 80 A0                  986 	push	a
      000658 40 00                  987 	jal	_putc
      00065A 01 94                  988 	ads	#1
                                    989 ;	test_varargs.c: 25: CHECK(4, dig(42) == 2);
      00065C 00 80                  990 	ldi	#0x00
      00065E 80 A0                  991 	push	a
      000660 00 80                  992 	ldi	#0x00
      000662 80 A0                  993 	push	a
      000664 00 80                  994 	ldi	#0x00
      000666 80 A0                  995 	push	a
      000668 2A 80                  996 	ldi	#0x2a
      00066A 80 A0                  997 	push	a
      00066C D3 01                  998 	jal	_dig
      00066E 07 FA                  999 	stax	7(sp)
      000670 04 94                 1000 	ads	#4
      000672 03 F2                 1001 	ldax	3(sp)
      000674 02 A4                 1002 	cpi	#0x02
      000676 08 A8                 1003 	bnz	00120$
      000678 81 80                 1004 	ldi	#>(___str_1 + 0)
      00067A 80 A0                 1005 	push	a
      00067C DD 80                 1006 	ldi	#<(___str_1 + 0)
      00067E 80 A0                 1007 	push	a
      000680 49 00                 1008 	jal	_puts
      000682 02 94                 1009 	ads	#2
      000684 0A B0                 1010 	br	00121$
      000686                       1011 00120$:
      000686 81 80                 1012 	ldi	#>(___str_2 + 0)
      000688 80 A0                 1013 	push	a
      00068A E1 80                 1014 	ldi	#<(___str_2 + 0)
      00068C 80 A0                 1015 	push	a
      00068E 49 00                 1016 	jal	_puts
      000690 02 94                 1017 	ads	#2
      000692 80 A1 00 00           1018 	ldx	#_fails
      000696 00 E4                 1019 	inx	0(ix)
      000698                       1020 00121$:
      000698 00 80                 1021 	ldi	#0x00
      00069A 80 A0                 1022 	push	a
      00069C 04 80                 1023 	ldi	#0x04
      00069E 80 A0                 1024 	push	a
      0006A0 6A 00                 1025 	jal	_puthex
      0006A2 02 94                 1026 	ads	#2
      0006A4 0A 80                 1027 	ldi	#0x0a
      0006A6 80 A0                 1028 	push	a
      0006A8 40 00                 1029 	jal	_putc
      0006AA 01 94                 1030 	ads	#1
                                   1031 ;	test_varargs.c: 26: CHECK(5, dig(123456) == 6);
      0006AC 00 80                 1032 	ldi	#0x00
      0006AE 80 A0                 1033 	push	a
      0006B0 01 80                 1034 	ldi	#0x01
      0006B2 80 A0                 1035 	push	a
      0006B4 E2 80                 1036 	ldi	#0xe2
      0006B6 80 A0                 1037 	push	a
      0006B8 40 80                 1038 	ldi	#0x40
      0006BA 80 A0                 1039 	push	a
      0006BC D3 01                 1040 	jal	_dig
      0006BE 07 FA                 1041 	stax	7(sp)
      0006C0 04 94                 1042 	ads	#4
      0006C2 03 F2                 1043 	ldax	3(sp)
      0006C4 06 A4                 1044 	cpi	#0x06
      0006C6 08 A8                 1045 	bnz	00126$
      0006C8 81 80                 1046 	ldi	#>(___str_1 + 0)
      0006CA 80 A0                 1047 	push	a
      0006CC DD 80                 1048 	ldi	#<(___str_1 + 0)
      0006CE 80 A0                 1049 	push	a
      0006D0 49 00                 1050 	jal	_puts
      0006D2 02 94                 1051 	ads	#2
      0006D4 0A B0                 1052 	br	00127$
      0006D6                       1053 00126$:
      0006D6 81 80                 1054 	ldi	#>(___str_2 + 0)
      0006D8 80 A0                 1055 	push	a
      0006DA E1 80                 1056 	ldi	#<(___str_2 + 0)
      0006DC 80 A0                 1057 	push	a
      0006DE 49 00                 1058 	jal	_puts
      0006E0 02 94                 1059 	ads	#2
      0006E2 80 A1 00 00           1060 	ldx	#_fails
      0006E6 00 E4                 1061 	inx	0(ix)
      0006E8                       1062 00127$:
      0006E8 00 80                 1063 	ldi	#0x00
      0006EA 80 A0                 1064 	push	a
      0006EC 05 80                 1065 	ldi	#0x05
      0006EE 80 A0                 1066 	push	a
      0006F0 6A 00                 1067 	jal	_puthex
      0006F2 02 94                 1068 	ads	#2
      0006F4 0A 80                 1069 	ldi	#0x0a
      0006F6 80 A0                 1070 	push	a
      0006F8 40 00                 1071 	jal	_putc
      0006FA 01 94                 1072 	ads	#1
                                   1073 ;	test_varargs.c: 27: DONE();
      0006FC 00 F4                 1074 	lda	_fails
      0006FE 06 B8                 1075 	bz	00133$
      000700 E7 80                 1076 	ldi	#<(___str_3 + 0)
      000702 01 FA                 1077 	stax	1(sp)
      000704 81 80                 1078 	ldi	#>(___str_3 + 0)
      000706 02 FA                 1079 	stax	2(sp)
      000708 05 B0                 1080 	br	00134$
      00070A                       1081 00133$:
      00070A F4 80                 1082 	ldi	#<(___str_4 + 0)
      00070C 01 FA                 1083 	stax	1(sp)
      00070E 81 80                 1084 	ldi	#>(___str_4 + 0)
      000710 02 FA                 1085 	stax	2(sp)
      000712                       1086 00134$:
      000712 02 F2                 1087 	ldax	2(sp)
      000714 80 A0                 1088 	push	a
      000716 02 F2                 1089 	ldax	2(sp)
      000718 80 A0                 1090 	push	a
      00071A 49 00                 1091 	jal	_puts
      00071C 02 94                 1092 	ads	#2
                                   1093 ;	test_varargs.c: 28: return 0;
      00071E 00 80                 1094 	ldi	#0x00
      000720 0A FA                 1095 	stax	10(sp)
      000722 0B FA                 1096 	stax	11(sp)
      000724                       1097 00131$:
                                   1098 ;	test_varargs.c: 29: }
      000724 07 94                 1099 	ads	#7
      000726 64 A1                 1100 	lra
      000728 00 8A                 1101 	ret
                                   1102 	.area CODE (CODE)
                                   1103 	.area CONST (CODE,CDATA)
                                   1104 	.area CONST (CODE,CDATA)
      000730                       1105 ___str_0:
      000730 30 80 00 8A 31 80 00  1106 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      000770 00 80 00 8A           1107 	.db 0x00
                                   1108 	.area CODE (CODE)
                                   1109 	.area CONST (CODE,CDATA)
      000774                       1110 ___str_1:
      000774 6F 80 00 8A 6B 80 00  1111 	.ascii "ok "
             8A 20 80 00 8A
      000780 00 80 00 8A           1112 	.db 0x00
                                   1113 	.area CODE (CODE)
                                   1114 	.area CONST (CODE,CDATA)
      000784                       1115 ___str_2:
      000784 46 80 00 8A 41 80 00  1116 	.ascii "FAIL "
             8A 49 80 00 8A 4C 80
             00 8A 20 80 00 8A
      000798 00 80 00 8A           1117 	.db 0x00
                                   1118 	.area CODE (CODE)
                                   1119 	.area CONST (CODE,CDATA)
      00079C                       1120 ___str_3:
      00079C 53 80 00 8A 4F 80 00  1121 	.ascii "SOME FAILED"
             8A 4D 80 00 8A 45 80
             00 8A 20 80 00 8A 46
             80 00 8A 41 80 00 8A
             49 80 00 8A 4C 80 00
             8A 45 80 00 8A 44 80
             00 8A
      0007C8 0A 80 00 8A           1122 	.db 0x0a
      0007CC 00 80 00 8A           1123 	.db 0x00
                                   1124 	.area CODE (CODE)
                                   1125 	.area CONST (CODE,CDATA)
      0007D0                       1126 ___str_4:
      0007D0 41 80 00 8A 4C 80 00  1127 	.ascii "ALL PASSED"
             8A 4C 80 00 8A 20 80
             00 8A 50 80 00 8A 41
             80 00 8A 53 80 00 8A
             53 80 00 8A 45 80 00
             8A 44 80 00 8A
      0007F8 0A 80 00 8A           1128 	.db 0x0a
      0007FC 00 80 00 8A           1129 	.db 0x00
                                   1130 	.area CODE (CODE)
                                   1131 	.area INITIALIZER (CODE,CDATA)
                                   1132 	.area CABS (ABS,CODE,CDATA)
