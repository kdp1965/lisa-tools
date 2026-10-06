                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_core
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
                                     16 	.globl _citab
                                     17 	.globl _ctab
                                     18 	.globl _main
                                     19 	.globl _sw
                                     20 	.globl _fill
                                     21 	.globl _sum
                                     22 	.globl _mul8s
                                     23 	.globl _mul16
                                     24 	.globl _cmp16u
                                     25 	.globl _cmp16
                                     26 	.globl _cmps
                                     27 	.globl _cmpu
                                     28 	.globl _sar16
                                     29 	.globl _shr16
                                     30 	.globl _shl
                                     31 	.globl _sub16
                                     32 	.globl _add32
                                     33 	.globl _add16
                                     34 	.globl _add8
                                     35 	.globl _UART_STATUS
                                     36 	.globl _UART_TX
                                     37 	.globl _arr
                                     38 	.globl _g32
                                     39 	.globl _g16
                                     40 	.globl _g8
                                     41 	.globl _fp
                                     42 ;--------------------------------------------------------
                                     43 ; special function registers
                                     44 ;--------------------------------------------------------
                                     45 	.area RSEG (ABS)
      000000                         46 	.org 0x0000
                           000210    47 _UART_TX	=	0x0210
                           000211    48 _UART_STATUS	=	0x0211
                                     49 ;--------------------------------------------------------
                                     50 ; ram data
                                     51 ;--------------------------------------------------------
                                     52 	.area DATA
      000000                         53 _fails:
      000000                         54 	.ds 1
      000001                         55 _fp::
      000001                         56 	.ds 2
                                     57 ;--------------------------------------------------------
                                     58 ; ram data
                                     59 ;--------------------------------------------------------
                                     60 	.area INITIALIZED
      000003                         61 _g8::
      000003                         62 	.ds 1
      000004                         63 _g16::
      000004                         64 	.ds 2
      000006                         65 _g32::
      000006                         66 	.ds 4
      00000A                         67 _arr::
      00000A                         68 	.ds 4
                                     69 ;--------------------------------------------------------
                                     70 ; overlayable items in ram
                                     71 ;--------------------------------------------------------
                                     72 ;--------------------------------------------------------
                                     73 ; Stack segment in internal ram
                                     74 ;--------------------------------------------------------
                                     75 	.area SSEG
      00000E                         76 __start__stack:
      00000E                         77 	.ds	1
                                     78 
                                     79 ;--------------------------------------------------------
                                     80 ; absolute external ram data
                                     81 ;--------------------------------------------------------
                                     82 	.area DABS (ABS)
                                     83 ;--------------------------------------------------------
                                     84 ; interrupt vector
                                     85 ;--------------------------------------------------------
                                     86 	.area HOME (CODE)
      000000                         87 __interrupt_vect:
      000000 0C 00                   88 	jal	__sdcc_gsinit_startup
      000002 40 8B                   89 	rets
      000004 40 8B                   90 	rets
      000006 40 8B                   91 	rets
      000008 40 8B                   92 	rets
      00000A 40 8B                   93 	rets
      00000C 40 8B                   94 	rets
      00000E 40 8B                   95 	rets
      000010 40 8B                   96 	rets
      000012 40 8B                   97 	rets
                                     98 ;--------------------------------------------------------
                                     99 ; global & static initialisations
                                    100 ;--------------------------------------------------------
                                    101 	.area HOME (CODE)
                                    102 	.area GSINIT (CODE)
                                    103 	.area GSFINAL (CODE)
                                    104 	.area GSINIT (CODE)
                                    105 	.area GSINIT (CODE)
      000018                        106 __sdcc_gsinit_startup::
      000018 80 A1 7F 00            107 	ldx	#0x007f
      00001C C8 8A                  108 	xchg	sp
      00001E 41 A1                  109 	amode	1
      000020 53 07                  110 	jal	___sdcc_external_startup
      000022 00 A4                  111 	cpi	#0
      000024 01 A2                  112 	if	ne
      000026 0A 00                  113 	jal	__sdcc_program_startup
      000028 00 80                  114 	ldi	#>l_DATA
      00002A 80 A0                  115 	push	a
      00002C 03 80                  116 	ldi	#<l_DATA
      00002E 80 A0                  117 	push	a
      000030 80 A1 00 00            118 	ldx	#s_DATA
      000034                        119 00001$:
      000034 01 F2                  120 	ldax	1(sp)
      000036 02 DA                  121 	or	2(sp)
      000038 08 B8                  122 	bz	00002$
      00003A 00 80                  123 	ldi	#0
      00003C 00 F8                  124 	stax	0(ix)
      00003E 01 98                  125 	adx	#1
      000040 01 9E                  126 	dcx	1(sp)
      000042 03 A2                  127 	if	c
      000044 02 9E                  128 	dcx	2(sp)
      000046 F7 B7                  129 	br	00001$
      000048                        130 00002$:
      000048 00 80                  131 	ldi	#>s_INITIALIZED
      00004A 80 A0                  132 	push	a
      00004C 03 80                  133 	ldi	#<s_INITIALIZED
      00004E 80 A0                  134 	push	a
      000050 00 80                  135 	ldi	#>l_INITIALIZED
      000052 04 FA                  136 	stax	4(sp)
      000054 0B 80                  137 	ldi	#<l_INITIALIZED
      000056 03 FA                  138 	stax	3(sp)
      000058 80 A1 7E 89            139 	ldx	#s_INITIALIZER
      00005C                        140 00003$:
      00005C 03 F2                  141 	ldax	3(sp)
      00005E 04 DA                  142 	or	4(sp)
      000060 0E B8                  143 	bz	00004$
      000062 80 8A                  144 	call	ix
      000064 01 98                  145 	adx	#1
      000066 68 A1                  146 	push	ix
      000068 03 CC                  147 	ldxx	3(sp)
      00006A 00 F8                  148 	stax	0(ix)
      00006C 03 E6                  149 	inx	3(sp)
      00006E 03 A2                  150 	if	c
      000070 04 E6                  151 	inx	4(sp)
      000072 6C A1                  152 	pop	ix
      000074 03 9E                  153 	dcx	3(sp)
      000076 03 A2                  154 	if	c
      000078 04 9E                  155 	dcx	4(sp)
      00007A F1 B7                  156 	br	00003$
      00007C                        157 00004$:
      00007C 04 94                  158 	ads	#4
                                    159 	.area GSFINAL (CODE)
      00007E 0A 00                  160 	jal	__sdcc_program_startup
                                    161 ;--------------------------------------------------------
                                    162 ; Home
                                    163 ;--------------------------------------------------------
                                    164 	.area HOME (CODE)
                                    165 	.area HOME (CODE)
      000014                        166 __sdcc_program_startup:
      000014 62 02                  167 	jal	_main
      000016                        168 00001$:
      000016 00 B0                  169 	br	00001$
                                    170 ;	return from main will return to caller
                                    171 ;--------------------------------------------------------
                                    172 ; code
                                    173 ;--------------------------------------------------------
                                    174 	.area CODE (CODE)
                                    175 ;	harness.h: 15: static void putc(char c)
                                    176 ;	---------------------------------
                                    177 ;	 Function putc
                                    178 ;	---------------------------------
      000080                        179 _putc:
      000080 FF 97                  180 	ads	#-1
                                    181 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        182 00101$:
      000082 11 F6                  183 	lda	_UART_STATUS
      000084 01 FA                  184 	stax	1(sp)
      000086 02 D4                  185 	andi	#0x02
      000088 FD BF                  186 	bz	00101$
                                    187 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  188 	ldax	2(sp)
      00008C 10 FE                  189 	sta	_UART_TX
      00008E                        190 00104$:
                                    191 ;	harness.h: 20: }
      00008E 01 94                  192 	ads	#1
      000090 00 8A                  193 	ret
                                    194 ;	harness.h: 22: static void puts(const char *s)
                                    195 ;	---------------------------------
                                    196 ;	 Function puts
                                    197 ;	---------------------------------
      000092                        198 _puts:
      000092 60 A1                  199 	sra
      000094 FD 97                  200 	ads	#-3
                                    201 ;	harness.h: 24: while (*s)
      000096 06 F2                  202 	ldax	6(sp)
      000098 02 FA                  203 	stax	2(sp)
      00009A 07 F2                  204 	ldax	7(sp)
      00009C 03 FA                  205 	stax	3(sp)
      00009E                        206 00101$:
      00009E 02 CC                  207 	ldxx	2(sp)
      0000A0 18 A0                  208 	txau
      0000A2 47 A0                  209 	btst	7
      0000A4 04 B8                  210 	bz	00119$
      0000A6 00 F0                  211 	ldax	0(ix)
      0000A8 01 FA                  212 	stax	1(sp)
      0000AA 08 B0                  213 	br	00120$
      0000AC                        214 00119$:
      0000AC 18 A0                  215 	txau
      0000AE 7F D4                  216 	andi	#0x7f
      0000B0 A1 A1                  217 	addaxu
      0000B2 10 A0                  218 	txa
      0000B4 A0 A1                  219 	addax
      0000B6 80 8A                  220 	call	ix
      0000B8 01 FA                  221 	stax	1(sp)
      0000BA                        222 00120$:
      0000BA 01 F2                  223 	ldax	1(sp)
      0000BC 09 B8                  224 	bz	00104$
                                    225 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  226 	inx	2(sp)
      0000C0 03 A2                  227 	if	c
      0000C2 03 E6                  228 	inx.p	3(sp)
      0000C4 01 F2                  229 	ldax	1(sp)
      0000C6 80 A0                  230 	push	a
      0000C8 40 00                  231 	jal	_putc
      0000CA 01 94                  232 	ads	#1
      0000CC E9 B7                  233 	br	00101$
      0000CE                        234 00104$:
                                    235 ;	harness.h: 26: }
      0000CE 03 94                  236 	ads	#3
      0000D0 64 A1                  237 	lra
      0000D2 00 8A                  238 	ret
                                    239 ;	harness.h: 28: static void puthex(unsigned int v)
                                    240 ;	---------------------------------
                                    241 ;	 Function puthex
                                    242 ;	---------------------------------
      0000D4                        243 _puthex:
      0000D4 60 A1                  244 	sra
      0000D6 FD 97                  245 	ads	#-3
                                    246 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    247 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  248 	ldax	7(sp)
      0000DA 02 FA                  249 	stax	2(sp)
      0000DC 00 80                  250 	ldi	#0x00
      0000DE 03 FA                  251 	stax	3(sp)
      0000E0 08 A0                  252 	ldc	#0
      0000E2 04 A0                  253 	shr
      0000E4 03 FA                  254 	stax	3(sp)
      0000E6 02 F2                  255 	ldax	2(sp)
      0000E8 04 A0                  256 	shr
      0000EA 02 FA                  257 	stax	2(sp)
      0000EC 03 F2                  258 	ldax	3(sp)
      0000EE 08 A0                  259 	ldc	#0
      0000F0 04 A0                  260 	shr
      0000F2 03 FA                  261 	stax	3(sp)
      0000F4 02 F2                  262 	ldax	2(sp)
      0000F6 04 A0                  263 	shr
      0000F8 02 FA                  264 	stax	2(sp)
      0000FA 03 F2                  265 	ldax	3(sp)
      0000FC 08 A0                  266 	ldc	#0
      0000FE 04 A0                  267 	shr
      000100 03 FA                  268 	stax	3(sp)
      000102 02 F2                  269 	ldax	2(sp)
      000104 04 A0                  270 	shr
      000106 02 FA                  271 	stax	2(sp)
      000108 03 F2                  272 	ldax	3(sp)
      00010A 08 A0                  273 	ldc	#0
      00010C 04 A0                  274 	shr
      00010E 03 FA                  275 	stax	3(sp)
      000110 02 F2                  276 	ldax	2(sp)
      000112 04 A0                  277 	shr
      000114 02 FA                  278 	stax	2(sp)
      000116 0F D4                  279 	andi	#0x0f
      000118 02 FA                  280 	stax	2(sp)
      00011A 00 80                  281 	ldi	#0x00
      00011C 03 FA                  282 	stax	3(sp)
      00011E 02 F2                  283 	ldax	2(sp)
      000120 08 A0                  284 	ldc	#0
      000122 81 90                  285 	adc	#<(___str_0 + 0)
      000124 02 FA                  286 	stax	2(sp)
      000126 03 F2                  287 	ldax	3(sp)
      000128 84 90                  288 	adc	#>(___str_0 + 0)
      00012A 03 FA                  289 	stax	3(sp)
      00012C 02 CC                  290 	ldxx	2(sp)
      00012E 18 A0                  291 	txau
      000130 47 A0                  292 	btst	7
      000132 04 B8                  293 	bz	00103$
      000134 00 F0                  294 	ldax	0(ix)
      000136 01 FA                  295 	stax	1(sp)
      000138 08 B0                  296 	br	00104$
      00013A                        297 00103$:
      00013A 18 A0                  298 	txau
      00013C 7F D4                  299 	andi	#0x7f
      00013E A1 A1                  300 	addaxu
      000140 10 A0                  301 	txa
      000142 A0 A1                  302 	addax
      000144 80 8A                  303 	call	ix
      000146 01 FA                  304 	stax	1(sp)
      000148                        305 00104$:
      000148 01 F2                  306 	ldax	1(sp)
      00014A 80 A0                  307 	push	a
      00014C 40 00                  308 	jal	_putc
      00014E 01 94                  309 	ads	#1
                                    310 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  311 	ldax	7(sp)
      000152 02 FA                  312 	stax	2(sp)
      000154 00 80                  313 	ldi	#0x00
      000156 03 FA                  314 	stax	3(sp)
      000158 02 F2                  315 	ldax	2(sp)
      00015A 0F D4                  316 	andi	#0x0f
      00015C 02 FA                  317 	stax	2(sp)
      00015E 00 80                  318 	ldi	#0x00
      000160 03 FA                  319 	stax	3(sp)
      000162 02 F2                  320 	ldax	2(sp)
      000164 08 A0                  321 	ldc	#0
      000166 81 90                  322 	adc	#<(___str_0 + 0)
      000168 02 FA                  323 	stax	2(sp)
      00016A 03 F2                  324 	ldax	3(sp)
      00016C 84 90                  325 	adc	#>(___str_0 + 0)
      00016E 03 FA                  326 	stax	3(sp)
      000170 02 CC                  327 	ldxx	2(sp)
      000172 18 A0                  328 	txau
      000174 47 A0                  329 	btst	7
      000176 04 B8                  330 	bz	00105$
      000178 00 F0                  331 	ldax	0(ix)
      00017A 01 FA                  332 	stax	1(sp)
      00017C 08 B0                  333 	br	00106$
      00017E                        334 00105$:
      00017E 18 A0                  335 	txau
      000180 7F D4                  336 	andi	#0x7f
      000182 A1 A1                  337 	addaxu
      000184 10 A0                  338 	txa
      000186 A0 A1                  339 	addax
      000188 80 8A                  340 	call	ix
      00018A 01 FA                  341 	stax	1(sp)
      00018C                        342 00106$:
      00018C 01 F2                  343 	ldax	1(sp)
      00018E 80 A0                  344 	push	a
      000190 40 00                  345 	jal	_putc
      000192 01 94                  346 	ads	#1
                                    347 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  348 	ldax	6(sp)
      000196 02 FA                  349 	stax	2(sp)
      000198 07 F2                  350 	ldax	7(sp)
      00019A 03 FA                  351 	stax	3(sp)
      00019C 08 A0                  352 	ldc	#0
      00019E 04 A0                  353 	shr
      0001A0 03 FA                  354 	stax	3(sp)
      0001A2 02 F2                  355 	ldax	2(sp)
      0001A4 04 A0                  356 	shr
      0001A6 02 FA                  357 	stax	2(sp)
      0001A8 03 F2                  358 	ldax	3(sp)
      0001AA 08 A0                  359 	ldc	#0
      0001AC 04 A0                  360 	shr
      0001AE 03 FA                  361 	stax	3(sp)
      0001B0 02 F2                  362 	ldax	2(sp)
      0001B2 04 A0                  363 	shr
      0001B4 02 FA                  364 	stax	2(sp)
      0001B6 03 F2                  365 	ldax	3(sp)
      0001B8 08 A0                  366 	ldc	#0
      0001BA 04 A0                  367 	shr
      0001BC 03 FA                  368 	stax	3(sp)
      0001BE 02 F2                  369 	ldax	2(sp)
      0001C0 04 A0                  370 	shr
      0001C2 02 FA                  371 	stax	2(sp)
      0001C4 03 F2                  372 	ldax	3(sp)
      0001C6 08 A0                  373 	ldc	#0
      0001C8 04 A0                  374 	shr
      0001CA 03 FA                  375 	stax	3(sp)
      0001CC 02 F2                  376 	ldax	2(sp)
      0001CE 04 A0                  377 	shr
      0001D0 02 FA                  378 	stax	2(sp)
      0001D2 0F D4                  379 	andi	#0x0f
      0001D4 02 FA                  380 	stax	2(sp)
      0001D6 00 80                  381 	ldi	#0x00
      0001D8 03 FA                  382 	stax	3(sp)
      0001DA 02 F2                  383 	ldax	2(sp)
      0001DC 08 A0                  384 	ldc	#0
      0001DE 81 90                  385 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  386 	stax	2(sp)
      0001E2 03 F2                  387 	ldax	3(sp)
      0001E4 84 90                  388 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  389 	stax	3(sp)
      0001E8 02 CC                  390 	ldxx	2(sp)
      0001EA 18 A0                  391 	txau
      0001EC 47 A0                  392 	btst	7
      0001EE 04 B8                  393 	bz	00107$
      0001F0 00 F0                  394 	ldax	0(ix)
      0001F2 01 FA                  395 	stax	1(sp)
      0001F4 08 B0                  396 	br	00108$
      0001F6                        397 00107$:
      0001F6 18 A0                  398 	txau
      0001F8 7F D4                  399 	andi	#0x7f
      0001FA A1 A1                  400 	addaxu
      0001FC 10 A0                  401 	txa
      0001FE A0 A1                  402 	addax
      000200 80 8A                  403 	call	ix
      000202 01 FA                  404 	stax	1(sp)
      000204                        405 00108$:
      000204 01 F2                  406 	ldax	1(sp)
      000206 80 A0                  407 	push	a
      000208 40 00                  408 	jal	_putc
      00020A 01 94                  409 	ads	#1
                                    410 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  411 	ldax	6(sp)
      00020E 0F D4                  412 	andi	#0x0f
      000210 02 FA                  413 	stax	2(sp)
      000212 00 80                  414 	ldi	#0x00
      000214 03 FA                  415 	stax	3(sp)
      000216 02 F2                  416 	ldax	2(sp)
      000218 08 A0                  417 	ldc	#0
      00021A 81 90                  418 	adc	#<(___str_0 + 0)
      00021C 02 FA                  419 	stax	2(sp)
      00021E 03 F2                  420 	ldax	3(sp)
      000220 84 90                  421 	adc	#>(___str_0 + 0)
      000222 03 FA                  422 	stax	3(sp)
      000224 02 CC                  423 	ldxx	2(sp)
      000226 18 A0                  424 	txau
      000228 47 A0                  425 	btst	7
      00022A 04 B8                  426 	bz	00109$
      00022C 00 F0                  427 	ldax	0(ix)
      00022E 01 FA                  428 	stax	1(sp)
      000230 08 B0                  429 	br	00110$
      000232                        430 00109$:
      000232 18 A0                  431 	txau
      000234 7F D4                  432 	andi	#0x7f
      000236 A1 A1                  433 	addaxu
      000238 10 A0                  434 	txa
      00023A A0 A1                  435 	addax
      00023C 80 8A                  436 	call	ix
      00023E 01 FA                  437 	stax	1(sp)
      000240                        438 00110$:
      000240 01 F2                  439 	ldax	1(sp)
      000242 80 A0                  440 	push	a
      000244 40 00                  441 	jal	_putc
      000246 01 94                  442 	ads	#1
      000248                        443 00101$:
                                    444 ;	harness.h: 35: }
      000248 03 94                  445 	ads	#3
      00024A 64 A1                  446 	lra
      00024C 00 8A                  447 	ret
                                    448 ;	test_core.c: 8: unsigned char add8(unsigned char a, unsigned char b) { return a + b; }
                                    449 ;	---------------------------------
                                    450 ;	 Function add8
                                    451 ;	---------------------------------
      00024E                        452 _add8:
      00024E FF 97                  453 	ads	#-1
      000250 02 F2                  454 	ldax	2(sp)
      000252 08 A0                  455 	ldc	#0
      000254 03 C2                  456 	add	3(sp)
      000256 01 FA                  457 	stax	1(sp)
      000258                        458 00101$:
      000258 01 94                  459 	ads	#1
      00025A 00 8A                  460 	ret
                                    461 ;	test_core.c: 9: int add16(int a, int b) { return a + b; }
                                    462 ;	---------------------------------
                                    463 ;	 Function add16
                                    464 ;	---------------------------------
      00025C                        465 _add16:
      00025C FE 97                  466 	ads	#-2
      00025E 05 F2                  467 	ldax	5(sp)
      000260 08 A0                  468 	ldc	#0
      000262 07 C2                  469 	add	7(sp)
      000264 01 FA                  470 	stax	1(sp)
      000266 06 F2                  471 	ldax	6(sp)
      000268 08 C2                  472 	add	8(sp)
      00026A 02 FA                  473 	stax	2(sp)
      00026C 01 F2                  474 	ldax	1(sp)
      00026E 03 FA                  475 	stax	3(sp)
      000270 02 F2                  476 	ldax	2(sp)
      000272 04 FA                  477 	stax	4(sp)
      000274                        478 00101$:
      000274 02 94                  479 	ads	#2
      000276 00 8A                  480 	ret
                                    481 ;	test_core.c: 10: long add32(long a, long b) { return a + b; }
                                    482 ;	---------------------------------
                                    483 ;	 Function add32
                                    484 ;	---------------------------------
      000278                        485 _add32:
      000278 FC 97                  486 	ads	#-4
      00027A 09 F2                  487 	ldax	9(sp)
      00027C 08 A0                  488 	ldc	#0
      00027E 0D C2                  489 	add	13(sp)
      000280 01 FA                  490 	stax	1(sp)
      000282 0A F2                  491 	ldax	10(sp)
      000284 0E C2                  492 	add	14(sp)
      000286 02 FA                  493 	stax	2(sp)
      000288 0B F2                  494 	ldax	11(sp)
      00028A 0F C2                  495 	add	15(sp)
      00028C 03 FA                  496 	stax	3(sp)
      00028E 0C F2                  497 	ldax	12(sp)
      000290 10 C2                  498 	add	16(sp)
      000292 04 FA                  499 	stax	4(sp)
      000294 01 F2                  500 	ldax	1(sp)
      000296 05 FA                  501 	stax	5(sp)
      000298 02 F2                  502 	ldax	2(sp)
      00029A 06 FA                  503 	stax	6(sp)
      00029C 03 F2                  504 	ldax	3(sp)
      00029E 07 FA                  505 	stax	7(sp)
      0002A0 04 F2                  506 	ldax	4(sp)
      0002A2 08 FA                  507 	stax	8(sp)
      0002A4                        508 00101$:
      0002A4 04 94                  509 	ads	#4
      0002A6 00 8A                  510 	ret
                                    511 ;	test_core.c: 11: int sub16(int a, int b) { return a - b; }
                                    512 ;	---------------------------------
                                    513 ;	 Function sub16
                                    514 ;	---------------------------------
      0002A8                        515 _sub16:
      0002A8 FE 97                  516 	ads	#-2
      0002AA 05 F2                  517 	ldax	5(sp)
      0002AC 08 A0                  518 	ldc	#0
      0002AE 07 CA                  519 	sub	7(sp)
      0002B0 01 FA                  520 	stax	1(sp)
      0002B2 06 F2                  521 	ldax	6(sp)
      0002B4 08 CA                  522 	sub	8(sp)
      0002B6 02 FA                  523 	stax	2(sp)
      0002B8 01 F2                  524 	ldax	1(sp)
      0002BA 03 FA                  525 	stax	3(sp)
      0002BC 02 F2                  526 	ldax	2(sp)
      0002BE 04 FA                  527 	stax	4(sp)
      0002C0                        528 00101$:
      0002C0 02 94                  529 	ads	#2
      0002C2 00 8A                  530 	ret
                                    531 ;	test_core.c: 12: unsigned char shl(unsigned char a, unsigned char n) { return a << n; }
                                    532 ;	---------------------------------
                                    533 ;	 Function shl
                                    534 ;	---------------------------------
      0002C4                        535 _shl:
      0002C4 FF 97                  536 	ads	#-1
      0002C6 02 F2                  537 	ldax	2(sp)
      0002C8 01 FA                  538 	stax	1(sp)
      0002CA 03 F2                  539 	ldax	3(sp)
      0002CC 80 A0                  540 	push	a
      0002CE                        541 00103$:
      0002CE 01 F2                  542 	ldax	1(sp)
      0002D0 07 B8                  543 	bz	00104$
      0002D2 01 9E                  544 	dcx	1(sp)
      0002D4 02 F2                  545 	ldax	2(sp)
      0002D6 08 A0                  546 	ldc	#0
      0002D8 00 A0                  547 	shl
      0002DA 02 FA                  548 	stax	2(sp)
      0002DC F9 B7                  549 	br	00103$
      0002DE                        550 00104$:
      0002DE 01 94                  551 	ads	#1
      0002E0 01 F2                  552 	ldax	1(sp)
      0002E2                        553 00101$:
      0002E2 01 94                  554 	ads	#1
      0002E4 00 8A                  555 	ret
                                    556 ;	test_core.c: 13: unsigned int shr16(unsigned int a, unsigned char n) { return a >> n; }
                                    557 ;	---------------------------------
                                    558 ;	 Function shr16
                                    559 ;	---------------------------------
      0002E6                        560 _shr16:
      0002E6 FE 97                  561 	ads	#-2
      0002E8 05 F2                  562 	ldax	5(sp)
      0002EA 01 FA                  563 	stax	1(sp)
      0002EC 06 F2                  564 	ldax	6(sp)
      0002EE 02 FA                  565 	stax	2(sp)
      0002F0 07 F2                  566 	ldax	7(sp)
      0002F2 80 A0                  567 	push	a
      0002F4                        568 00103$:
      0002F4 01 F2                  569 	ldax	1(sp)
      0002F6 0A B8                  570 	bz	00104$
      0002F8 01 9E                  571 	dcx	1(sp)
      0002FA 03 F2                  572 	ldax	3(sp)
      0002FC 08 A0                  573 	ldc	#0
      0002FE 04 A0                  574 	shr
      000300 03 FA                  575 	stax	3(sp)
      000302 02 F2                  576 	ldax	2(sp)
      000304 04 A0                  577 	shr
      000306 02 FA                  578 	stax	2(sp)
      000308 F6 B7                  579 	br	00103$
      00030A                        580 00104$:
      00030A 01 94                  581 	ads	#1
      00030C 01 F2                  582 	ldax	1(sp)
      00030E 03 FA                  583 	stax	3(sp)
      000310 02 F2                  584 	ldax	2(sp)
      000312 04 FA                  585 	stax	4(sp)
      000314                        586 00101$:
      000314 02 94                  587 	ads	#2
      000316 00 8A                  588 	ret
                                    589 ;	test_core.c: 14: int sar16(int a) { return a >> 3; }
                                    590 ;	---------------------------------
                                    591 ;	 Function sar16
                                    592 ;	---------------------------------
      000318                        593 _sar16:
      000318 FE 97                  594 	ads	#-2
      00031A 05 F2                  595 	ldax	5(sp)
      00031C 01 FA                  596 	stax	1(sp)
      00031E 06 F2                  597 	ldax	6(sp)
      000320 02 FA                  598 	stax	2(sp)
      000322 43 A1                  599 	amode	3
      000324 02 F2                  600 	ldax	2(sp)
      000326 04 A0                  601 	shr
      000328 41 A1                  602 	amode	1
      00032A 02 FA                  603 	stax	2(sp)
      00032C 01 F2                  604 	ldax	1(sp)
      00032E 04 A0                  605 	shr
      000330 01 FA                  606 	stax	1(sp)
      000332 43 A1                  607 	amode	3
      000334 02 F2                  608 	ldax	2(sp)
      000336 04 A0                  609 	shr
      000338 41 A1                  610 	amode	1
      00033A 02 FA                  611 	stax	2(sp)
      00033C 01 F2                  612 	ldax	1(sp)
      00033E 04 A0                  613 	shr
      000340 01 FA                  614 	stax	1(sp)
      000342 43 A1                  615 	amode	3
      000344 02 F2                  616 	ldax	2(sp)
      000346 04 A0                  617 	shr
      000348 41 A1                  618 	amode	1
      00034A 02 FA                  619 	stax	2(sp)
      00034C 01 F2                  620 	ldax	1(sp)
      00034E 04 A0                  621 	shr
      000350 01 FA                  622 	stax	1(sp)
      000352 03 FA                  623 	stax	3(sp)
      000354 02 F2                  624 	ldax	2(sp)
      000356 04 FA                  625 	stax	4(sp)
      000358                        626 00101$:
      000358 02 94                  627 	ads	#2
      00035A 00 8A                  628 	ret
                                    629 ;	test_core.c: 15: unsigned char cmpu(unsigned char a, unsigned char b) { return a < b; }
                                    630 ;	---------------------------------
                                    631 ;	 Function cmpu
                                    632 ;	---------------------------------
      00035C                        633 _cmpu:
      00035C FF 97                  634 	ads	#-1
      00035E 02 F2                  635 	ldax	2(sp)
      000360 03 EA                  636 	cmp	3(sp)
      000362 C5 A1                  637 	ldac	lt
      000364 01 FA                  638 	stax	1(sp)
      000366                        639 00101$:
      000366 01 94                  640 	ads	#1
      000368 00 8A                  641 	ret
                                    642 ;	test_core.c: 16: unsigned char cmps(signed char a, signed char b) { return a < b; }
                                    643 ;	---------------------------------
                                    644 ;	 Function cmps
                                    645 ;	---------------------------------
      00036A                        646 _cmps:
      00036A FF 97                  647 	ads	#-1
      00036C 02 F2                  648 	ldax	2(sp)
      00036E 03 EA                  649 	cmp	3(sp)
      000370 35 A2                  650 	ifte	slt
      000372 01 80                  651 	ldi.p	#0x01
      000374 00 80                  652 	ldi.p	#0x00
      000376 01 FA                  653 	stax	1(sp)
      000378                        654 00101$:
      000378 01 94                  655 	ads	#1
      00037A 00 8A                  656 	ret
                                    657 ;	test_core.c: 17: unsigned char cmp16(int a, int b) { return a < b; }
                                    658 ;	---------------------------------
                                    659 ;	 Function cmp16
                                    660 ;	---------------------------------
      00037C                        661 _cmp16:
      00037C FF 97                  662 	ads	#-1
      00037E 03 F2                  663 	ldax	3(sp)
      000380 05 EA                  664 	cmp	5(sp)
      000382 25 A2                  665 	if	slt
      000384 09 B0                  666 	br.p	00103$
      000386 24 A2                  667 	if	sgt
      000388 05 B0                  668 	br.p	00104$
      00038A 02 F2                  669 	ldax	2(sp)
      00038C 04 EA                  670 	cmp	4(sp)
      00038E 05 A2                  671 	if	lt
      000390 03 B0                  672 	br.p	00103$
      000392                        673 00104$:
      000392 00 80                  674 	ldi	#0x00
      000394 02 B0                  675 	br	00105$
      000396                        676 00103$:
      000396 01 80                  677 	ldi	#0x01
      000398                        678 00105$:
      000398 01 FA                  679 	stax	1(sp)
      00039A                        680 00101$:
      00039A 01 94                  681 	ads	#1
      00039C 00 8A                  682 	ret
                                    683 ;	test_core.c: 18: unsigned char cmp16u(unsigned int a, unsigned int b) { return a > b; }
                                    684 ;	---------------------------------
                                    685 ;	 Function cmp16u
                                    686 ;	---------------------------------
      00039E                        687 _cmp16u:
      00039E FF 97                  688 	ads	#-1
      0003A0 03 F2                  689 	ldax	3(sp)
      0003A2 05 EA                  690 	cmp	5(sp)
      0003A4 04 A2                  691 	if	gt
      0003A6 09 B0                  692 	br.p	00103$
      0003A8 05 A2                  693 	if	lt
      0003AA 05 B0                  694 	br.p	00104$
      0003AC 02 F2                  695 	ldax	2(sp)
      0003AE 04 EA                  696 	cmp	4(sp)
      0003B0 04 A2                  697 	if	gt
      0003B2 03 B0                  698 	br.p	00103$
      0003B4                        699 00104$:
      0003B4 00 80                  700 	ldi	#0x00
      0003B6 02 B0                  701 	br	00105$
      0003B8                        702 00103$:
      0003B8 01 80                  703 	ldi	#0x01
      0003BA                        704 00105$:
      0003BA 01 FA                  705 	stax	1(sp)
      0003BC                        706 00101$:
      0003BC 01 94                  707 	ads	#1
      0003BE 00 8A                  708 	ret
                                    709 ;	test_core.c: 19: unsigned int mul16(unsigned int a, unsigned int b) { return a * b; }
                                    710 ;	---------------------------------
                                    711 ;	 Function mul16
                                    712 ;	---------------------------------
      0003C0                        713 _mul16:
      0003C0 FE 97                  714 	ads	#-2
      0003C2 05 F2                  715 	ldax	5(sp)
      0003C4 07 86                  716 	mulu	7(sp)
      0003C6 80 A0                  717 	push	a
      0003C8 06 F2                  718 	ldax	6(sp)
      0003CA 09 C6                  719 	mul	9(sp)
      0003CC 08 A0                  720 	ldc	#0
      0003CE 01 C2                  721 	add	1(sp)
      0003D0 01 FA                  722 	stax	1(sp)
      0003D2 07 F2                  723 	ldax	7(sp)
      0003D4 08 C6                  724 	mul	8(sp)
      0003D6 08 A0                  725 	ldc	#0
      0003D8 01 C2                  726 	add	1(sp)
      0003DA 01 FA                  727 	stax	1(sp)
      0003DC 06 F2                  728 	ldax	6(sp)
      0003DE 08 C6                  729 	mul	8(sp)
      0003E0 02 FA                  730 	stax	2(sp)
      0003E2 C0 A0                  731 	pop	a
      0003E4 02 FA                  732 	stax	2(sp)
      0003E6 01 F2                  733 	ldax	1(sp)
      0003E8 03 FA                  734 	stax	3(sp)
      0003EA 02 F2                  735 	ldax	2(sp)
      0003EC 04 FA                  736 	stax	4(sp)
      0003EE                        737 00101$:
      0003EE 02 94                  738 	ads	#2
      0003F0 00 8A                  739 	ret
                                    740 ;	test_core.c: 20: int mul8s(signed char a, signed char b) { return a * b; }
                                    741 ;	---------------------------------
                                    742 ;	 Function mul8s
                                    743 ;	---------------------------------
      0003F2                        744 _mul8s:
      0003F2 FE 97                  745 	ads	#-2
      0003F4 43 A1                  746 	amode	3
      0003F6 05 F2                  747 	ldax	5(sp)
      0003F8 06 C6                  748 	mul	6(sp)
      0003FA 80 A0                  749 	push	a
      0003FC 06 F2                  750 	ldax	6(sp)
      0003FE 07 86                  751 	mulu	7(sp)
      000400 41 A1                  752 	amode	1
      000402 03 FA                  753 	stax	3(sp)
      000404 C0 A0                  754 	pop	a
      000406 01 FA                  755 	stax	1(sp)
      000408 03 FA                  756 	stax	3(sp)
      00040A 02 F2                  757 	ldax	2(sp)
      00040C 04 FA                  758 	stax	4(sp)
      00040E                        759 00101$:
      00040E 02 94                  760 	ads	#2
      000410 00 8A                  761 	ret
                                    762 ;	test_core.c: 21: unsigned char sum(const unsigned char *p, unsigned char n) { unsigned char s = 0; while (n--) s += *p++; return s; }
                                    763 ;	---------------------------------
                                    764 ;	 Function sum
                                    765 ;	---------------------------------
      000412                        766 _sum:
      000412 60 A1                  767 	sra
      000414 FB 97                  768 	ads	#-5
      000416 00 80                  769 	ldi	#0x00
      000418 05 FA                  770 	stax	5(sp)
      00041A 08 F2                  771 	ldax	8(sp)
      00041C 03 FA                  772 	stax	3(sp)
      00041E 09 F2                  773 	ldax	9(sp)
      000420 04 FA                  774 	stax	4(sp)
      000422 0A F2                  775 	ldax	10(sp)
      000424 02 FA                  776 	stax	2(sp)
      000426                        777 00101$:
      000426 02 F2                  778 	ldax	2(sp)
      000428 01 FA                  779 	stax	1(sp)
      00042A 02 9E                  780 	dcx	2(sp)
      00042C 01 F2                  781 	ldax	1(sp)
      00042E 17 B8                  782 	bz	00103$
      000430 03 CC                  783 	ldxx	3(sp)
      000432 18 A0                  784 	txau
      000434 47 A0                  785 	btst	7
      000436 04 B8                  786 	bz	00119$
      000438 00 F0                  787 	ldax	0(ix)
      00043A 01 FA                  788 	stax	1(sp)
      00043C 08 B0                  789 	br	00120$
      00043E                        790 00119$:
      00043E 18 A0                  791 	txau
      000440 7F D4                  792 	andi	#0x7f
      000442 A1 A1                  793 	addaxu
      000444 10 A0                  794 	txa
      000446 A0 A1                  795 	addax
      000448 80 8A                  796 	call	ix
      00044A 01 FA                  797 	stax	1(sp)
      00044C                        798 00120$:
      00044C 03 E6                  799 	inx	3(sp)
      00044E 03 A2                  800 	if	c
      000450 04 E6                  801 	inx.p	4(sp)
      000452 05 F2                  802 	ldax	5(sp)
      000454 08 A0                  803 	ldc	#0
      000456 01 C2                  804 	add	1(sp)
      000458 05 FA                  805 	stax	5(sp)
      00045A E6 B7                  806 	br	00101$
      00045C                        807 00103$:
      00045C 05 F2                  808 	ldax	5(sp)
      00045E                        809 00104$:
      00045E 05 94                  810 	ads	#5
      000460 64 A1                  811 	lra
      000462 00 8A                  812 	ret
                                    813 ;	test_core.c: 22: void fill(unsigned char *p, unsigned char n, unsigned char v) { while (n--) *p++ = v++; }
                                    814 ;	---------------------------------
                                    815 ;	 Function fill
                                    816 ;	---------------------------------
      000464                        817 _fill:
      000464 60 A1                  818 	sra
      000466 FB 97                  819 	ads	#-5
      000468 0B F2                  820 	ldax	11(sp)
      00046A 05 FA                  821 	stax	5(sp)
      00046C 08 F2                  822 	ldax	8(sp)
      00046E 03 FA                  823 	stax	3(sp)
      000470 09 F2                  824 	ldax	9(sp)
      000472 04 FA                  825 	stax	4(sp)
      000474 0A F2                  826 	ldax	10(sp)
      000476 02 FA                  827 	stax	2(sp)
      000478                        828 00101$:
      000478 02 F2                  829 	ldax	2(sp)
      00047A 01 FA                  830 	stax	1(sp)
      00047C 02 9E                  831 	dcx	2(sp)
      00047E 01 F2                  832 	ldax	1(sp)
      000480 09 B8                  833 	bz	00104$
      000482 03 CC                  834 	ldxx	3(sp)
      000484 05 F2                  835 	ldax	5(sp)
      000486 00 F8                  836 	stax	0(ix)
      000488 05 E6                  837 	inx	5(sp)
      00048A 03 E6                  838 	inx	3(sp)
      00048C 03 A2                  839 	if	c
      00048E 04 E6                  840 	inx.p	4(sp)
      000490 F4 B7                  841 	br	00101$
      000492                        842 00104$:
      000492 05 94                  843 	ads	#5
      000494 64 A1                  844 	lra
      000496 00 8A                  845 	ret
                                    846 ;	test_core.c: 23: unsigned char sw(unsigned char x) { switch (x) { case 0: return 10; case 1: return 11; case 2: return 12; case 5: return 15; default: return 99; } }
                                    847 ;	---------------------------------
                                    848 ;	 Function sw
                                    849 ;	---------------------------------
      000498                        850 _sw:
      000498 01 F2                  851 	ldax	1(sp)
      00049A 0B B8                  852 	bz	00101$
      00049C 01 F2                  853 	ldax	1(sp)
      00049E 01 A4                  854 	cpi	#0x01
      0004A0 0A B8                  855 	bz	00102$
      0004A2 01 F2                  856 	ldax	1(sp)
      0004A4 02 A4                  857 	cpi	#0x02
      0004A6 09 B8                  858 	bz	00103$
      0004A8 01 F2                  859 	ldax	1(sp)
      0004AA 05 A4                  860 	cpi	#0x05
      0004AC 08 B8                  861 	bz	00104$
      0004AE 09 B0                  862 	br	00105$
      0004B0                        863 00101$:
      0004B0 0A 80                  864 	ldi	#0x0a
      0004B2 08 B0                  865 	br	00107$
      0004B4                        866 00102$:
      0004B4 0B 80                  867 	ldi	#0x0b
      0004B6 06 B0                  868 	br	00107$
      0004B8                        869 00103$:
      0004B8 0C 80                  870 	ldi	#0x0c
      0004BA 04 B0                  871 	br	00107$
      0004BC                        872 00104$:
      0004BC 0F 80                  873 	ldi	#0x0f
      0004BE 02 B0                  874 	br	00107$
      0004C0                        875 00105$:
      0004C0 63 80                  876 	ldi	#0x63
      0004C2                        877 00107$:
      0004C2 00 8A                  878 	ret
                                    879 ;	test_core.c: 25: int main(void) {
                                    880 ;	---------------------------------
                                    881 ;	 Function main
                                    882 ;	---------------------------------
      0004C4                        883 _main:
      0004C4 60 A1                  884 	sra
      0004C6 F8 97                  885 	ads	#-8
                                    886 ;	test_core.c: 27: CHECK(1, add8(200, 100) == 44);
      0004C8 64 80                  887 	ldi	#0x64
      0004CA 80 A0                  888 	push	a
      0004CC C8 80                  889 	ldi	#0xc8
      0004CE 80 A0                  890 	push	a
      0004D0 27 01                  891 	jal	_add8
      0004D2 0A FA                  892 	stax	10(sp)
      0004D4 02 94                  893 	ads	#2
      0004D6 08 F2                  894 	ldax	8(sp)
      0004D8 2C A4                  895 	cpi	#0x2c
      0004DA 08 A8                  896 	bnz	00102$
      0004DC 84 80                  897 	ldi	#>(___str_1 + 0)
      0004DE 80 A0                  898 	push	a
      0004E0 9C 80                  899 	ldi	#<(___str_1 + 0)
      0004E2 80 A0                  900 	push	a
      0004E4 49 00                  901 	jal	_puts
      0004E6 02 94                  902 	ads	#2
      0004E8 0A B0                  903 	br	00103$
      0004EA                        904 00102$:
      0004EA 84 80                  905 	ldi	#>(___str_2 + 0)
      0004EC 80 A0                  906 	push	a
      0004EE A0 80                  907 	ldi	#<(___str_2 + 0)
      0004F0 80 A0                  908 	push	a
      0004F2 49 00                  909 	jal	_puts
      0004F4 02 94                  910 	ads	#2
      0004F6 80 A1 00 00            911 	ldx	#_fails
      0004FA 00 E4                  912 	inx	0(ix)
      0004FC                        913 00103$:
      0004FC 00 80                  914 	ldi	#0x00
      0004FE 80 A0                  915 	push	a
      000500 01 80                  916 	ldi	#0x01
      000502 80 A0                  917 	push	a
      000504 6A 00                  918 	jal	_puthex
      000506 02 94                  919 	ads	#2
      000508 0A 80                  920 	ldi	#0x0a
      00050A 80 A0                  921 	push	a
      00050C 40 00                  922 	jal	_putc
      00050E 01 94                  923 	ads	#1
                                    924 ;	test_core.c: 28: CHECK(2, add16(-1234, 1300) == 66);
      000510 05 80                  925 	ldi	#0x05
      000512 80 A0                  926 	push	a
      000514 14 80                  927 	ldi	#0x14
      000516 80 A0                  928 	push	a
      000518 FB 80                  929 	ldi	#0xfb
      00051A 80 A0                  930 	push	a
      00051C 2E 80                  931 	ldi	#0x2e
      00051E 80 A0                  932 	push	a
      000520 FE 97                  933 	ads	#-2
      000522 2E 01                  934 	jal	_add16
      000524 01 F2                  935 	ldax	1(sp)
      000526 0C FA                  936 	stax	12(sp)
      000528 02 F2                  937 	ldax	2(sp)
      00052A 0D FA                  938 	stax	13(sp)
      00052C 06 94                  939 	ads	#6
      00052E 06 F2                  940 	ldax	6(sp)
      000530 42 A4                  941 	cpi	#0x42
      000532 03 A8                  942 	bnz	00541$
      000534 07 F2                  943 	ldax	7(sp)
      000536 00 A4                  944 	cpi	#0x00
      000538                        945 00541$:
      000538 08 A8                  946 	bnz	00108$
      00053A 84 80                  947 	ldi	#>(___str_1 + 0)
      00053C 80 A0                  948 	push	a
      00053E 9C 80                  949 	ldi	#<(___str_1 + 0)
      000540 80 A0                  950 	push	a
      000542 49 00                  951 	jal	_puts
      000544 02 94                  952 	ads	#2
      000546 0A B0                  953 	br	00109$
      000548                        954 00108$:
      000548 84 80                  955 	ldi	#>(___str_2 + 0)
      00054A 80 A0                  956 	push	a
      00054C A0 80                  957 	ldi	#<(___str_2 + 0)
      00054E 80 A0                  958 	push	a
      000550 49 00                  959 	jal	_puts
      000552 02 94                  960 	ads	#2
      000554 80 A1 00 00            961 	ldx	#_fails
      000558 00 E4                  962 	inx	0(ix)
      00055A                        963 00109$:
      00055A 00 80                  964 	ldi	#0x00
      00055C 80 A0                  965 	push	a
      00055E 02 80                  966 	ldi	#0x02
      000560 80 A0                  967 	push	a
      000562 6A 00                  968 	jal	_puthex
      000564 02 94                  969 	ads	#2
      000566 0A 80                  970 	ldi	#0x0a
      000568 80 A0                  971 	push	a
      00056A 40 00                  972 	jal	_putc
      00056C 01 94                  973 	ads	#1
                                    974 ;	test_core.c: 29: CHECK(3, add32(0x12345678, 0x11111111) == 0x23456789);
      00056E 11 80                  975 	ldi	#0x11
      000570 80 A0                  976 	push	a
      000572 11 80                  977 	ldi	#0x11
      000574 80 A0                  978 	push	a
      000576 11 80                  979 	ldi	#0x11
      000578 80 A0                  980 	push	a
      00057A 11 80                  981 	ldi	#0x11
      00057C 80 A0                  982 	push	a
      00057E 12 80                  983 	ldi	#0x12
      000580 80 A0                  984 	push	a
      000582 34 80                  985 	ldi	#0x34
      000584 80 A0                  986 	push	a
      000586 56 80                  987 	ldi	#0x56
      000588 80 A0                  988 	push	a
      00058A 78 80                  989 	ldi	#0x78
      00058C 80 A0                  990 	push	a
      00058E FC 97                  991 	ads	#-4
      000590 3C 01                  992 	jal	_add32
      000592 01 F2                  993 	ldax	1(sp)
      000594 0E FA                  994 	stax	14(sp)
      000596 02 F2                  995 	ldax	2(sp)
      000598 0F FA                  996 	stax	15(sp)
      00059A 03 F2                  997 	ldax	3(sp)
      00059C 10 FA                  998 	stax	16(sp)
      00059E 04 F2                  999 	ldax	4(sp)
      0005A0 11 FA                 1000 	stax	17(sp)
      0005A2 0C 94                 1001 	ads	#12
      0005A4 02 F2                 1002 	ldax	2(sp)
      0005A6 89 A4                 1003 	cpi	#0x89
      0005A8 09 A8                 1004 	bnz	00543$
      0005AA 03 F2                 1005 	ldax	3(sp)
      0005AC 67 A4                 1006 	cpi	#0x67
      0005AE 06 A8                 1007 	bnz	00543$
      0005B0 04 F2                 1008 	ldax	4(sp)
      0005B2 45 A4                 1009 	cpi	#0x45
      0005B4 03 A8                 1010 	bnz	00543$
      0005B6 05 F2                 1011 	ldax	5(sp)
      0005B8 23 A4                 1012 	cpi	#0x23
      0005BA                       1013 00543$:
      0005BA 08 A8                 1014 	bnz	00114$
      0005BC 84 80                 1015 	ldi	#>(___str_1 + 0)
      0005BE 80 A0                 1016 	push	a
      0005C0 9C 80                 1017 	ldi	#<(___str_1 + 0)
      0005C2 80 A0                 1018 	push	a
      0005C4 49 00                 1019 	jal	_puts
      0005C6 02 94                 1020 	ads	#2
      0005C8 0A B0                 1021 	br	00115$
      0005CA                       1022 00114$:
      0005CA 84 80                 1023 	ldi	#>(___str_2 + 0)
      0005CC 80 A0                 1024 	push	a
      0005CE A0 80                 1025 	ldi	#<(___str_2 + 0)
      0005D0 80 A0                 1026 	push	a
      0005D2 49 00                 1027 	jal	_puts
      0005D4 02 94                 1028 	ads	#2
      0005D6 80 A1 00 00           1029 	ldx	#_fails
      0005DA 00 E4                 1030 	inx	0(ix)
      0005DC                       1031 00115$:
      0005DC 00 80                 1032 	ldi	#0x00
      0005DE 80 A0                 1033 	push	a
      0005E0 03 80                 1034 	ldi	#0x03
      0005E2 80 A0                 1035 	push	a
      0005E4 6A 00                 1036 	jal	_puthex
      0005E6 02 94                 1037 	ads	#2
      0005E8 0A 80                 1038 	ldi	#0x0a
      0005EA 80 A0                 1039 	push	a
      0005EC 40 00                 1040 	jal	_putc
      0005EE 01 94                 1041 	ads	#1
                                   1042 ;	test_core.c: 30: CHECK(4, sub16(100, 300) == -200);
      0005F0 01 80                 1043 	ldi	#0x01
      0005F2 80 A0                 1044 	push	a
      0005F4 2C 80                 1045 	ldi	#0x2c
      0005F6 80 A0                 1046 	push	a
      0005F8 00 80                 1047 	ldi	#0x00
      0005FA 80 A0                 1048 	push	a
      0005FC 64 80                 1049 	ldi	#0x64
      0005FE 80 A0                 1050 	push	a
      000600 FE 97                 1051 	ads	#-2
      000602 54 01                 1052 	jal	_sub16
      000604 01 F2                 1053 	ldax	1(sp)
      000606 0C FA                 1054 	stax	12(sp)
      000608 02 F2                 1055 	ldax	2(sp)
      00060A 0D FA                 1056 	stax	13(sp)
      00060C 06 94                 1057 	ads	#6
      00060E 06 F2                 1058 	ldax	6(sp)
      000610 38 A4                 1059 	cpi	#0x38
      000612 03 A8                 1060 	bnz	00545$
      000614 07 F2                 1061 	ldax	7(sp)
      000616 FF A4                 1062 	cpi	#0xff
      000618                       1063 00545$:
      000618 08 A8                 1064 	bnz	00120$
      00061A 84 80                 1065 	ldi	#>(___str_1 + 0)
      00061C 80 A0                 1066 	push	a
      00061E 9C 80                 1067 	ldi	#<(___str_1 + 0)
      000620 80 A0                 1068 	push	a
      000622 49 00                 1069 	jal	_puts
      000624 02 94                 1070 	ads	#2
      000626 0A B0                 1071 	br	00121$
      000628                       1072 00120$:
      000628 84 80                 1073 	ldi	#>(___str_2 + 0)
      00062A 80 A0                 1074 	push	a
      00062C A0 80                 1075 	ldi	#<(___str_2 + 0)
      00062E 80 A0                 1076 	push	a
      000630 49 00                 1077 	jal	_puts
      000632 02 94                 1078 	ads	#2
      000634 80 A1 00 00           1079 	ldx	#_fails
      000638 00 E4                 1080 	inx	0(ix)
      00063A                       1081 00121$:
      00063A 00 80                 1082 	ldi	#0x00
      00063C 80 A0                 1083 	push	a
      00063E 04 80                 1084 	ldi	#0x04
      000640 80 A0                 1085 	push	a
      000642 6A 00                 1086 	jal	_puthex
      000644 02 94                 1087 	ads	#2
      000646 0A 80                 1088 	ldi	#0x0a
      000648 80 A0                 1089 	push	a
      00064A 40 00                 1090 	jal	_putc
      00064C 01 94                 1091 	ads	#1
                                   1092 ;	test_core.c: 31: CHECK(5, shl(3, 4) == 48);
      00064E 04 80                 1093 	ldi	#0x04
      000650 80 A0                 1094 	push	a
      000652 03 80                 1095 	ldi	#0x03
      000654 80 A0                 1096 	push	a
      000656 62 01                 1097 	jal	_shl
      000658 0A FA                 1098 	stax	10(sp)
      00065A 02 94                 1099 	ads	#2
      00065C 08 F2                 1100 	ldax	8(sp)
      00065E 30 A4                 1101 	cpi	#0x30
      000660 08 A8                 1102 	bnz	00126$
      000662 84 80                 1103 	ldi	#>(___str_1 + 0)
      000664 80 A0                 1104 	push	a
      000666 9C 80                 1105 	ldi	#<(___str_1 + 0)
      000668 80 A0                 1106 	push	a
      00066A 49 00                 1107 	jal	_puts
      00066C 02 94                 1108 	ads	#2
      00066E 0A B0                 1109 	br	00127$
      000670                       1110 00126$:
      000670 84 80                 1111 	ldi	#>(___str_2 + 0)
      000672 80 A0                 1112 	push	a
      000674 A0 80                 1113 	ldi	#<(___str_2 + 0)
      000676 80 A0                 1114 	push	a
      000678 49 00                 1115 	jal	_puts
      00067A 02 94                 1116 	ads	#2
      00067C 80 A1 00 00           1117 	ldx	#_fails
      000680 00 E4                 1118 	inx	0(ix)
      000682                       1119 00127$:
      000682 00 80                 1120 	ldi	#0x00
      000684 80 A0                 1121 	push	a
      000686 05 80                 1122 	ldi	#0x05
      000688 80 A0                 1123 	push	a
      00068A 6A 00                 1124 	jal	_puthex
      00068C 02 94                 1125 	ads	#2
      00068E 0A 80                 1126 	ldi	#0x0a
      000690 80 A0                 1127 	push	a
      000692 40 00                 1128 	jal	_putc
      000694 01 94                 1129 	ads	#1
                                   1130 ;	test_core.c: 32: CHECK(6, shr16(0x8000, 15) == 1);
      000696 0F 80                 1131 	ldi	#0x0f
      000698 80 A0                 1132 	push	a
      00069A 80 80                 1133 	ldi	#0x80
      00069C 80 A0                 1134 	push	a
      00069E 00 80                 1135 	ldi	#0x00
      0006A0 80 A0                 1136 	push	a
      0006A2 FE 97                 1137 	ads	#-2
      0006A4 73 01                 1138 	jal	_shr16
      0006A6 01 F2                 1139 	ldax	1(sp)
      0006A8 0B FA                 1140 	stax	11(sp)
      0006AA 02 F2                 1141 	ldax	2(sp)
      0006AC 0C FA                 1142 	stax	12(sp)
      0006AE 05 94                 1143 	ads	#5
      0006B0 06 F2                 1144 	ldax	6(sp)
      0006B2 01 A4                 1145 	cpi	#0x01
      0006B4 03 A8                 1146 	bnz	00549$
      0006B6 07 F2                 1147 	ldax	7(sp)
      0006B8 00 A4                 1148 	cpi	#0x00
      0006BA                       1149 00549$:
      0006BA 08 A8                 1150 	bnz	00132$
      0006BC 84 80                 1151 	ldi	#>(___str_1 + 0)
      0006BE 80 A0                 1152 	push	a
      0006C0 9C 80                 1153 	ldi	#<(___str_1 + 0)
      0006C2 80 A0                 1154 	push	a
      0006C4 49 00                 1155 	jal	_puts
      0006C6 02 94                 1156 	ads	#2
      0006C8 0A B0                 1157 	br	00133$
      0006CA                       1158 00132$:
      0006CA 84 80                 1159 	ldi	#>(___str_2 + 0)
      0006CC 80 A0                 1160 	push	a
      0006CE A0 80                 1161 	ldi	#<(___str_2 + 0)
      0006D0 80 A0                 1162 	push	a
      0006D2 49 00                 1163 	jal	_puts
      0006D4 02 94                 1164 	ads	#2
      0006D6 80 A1 00 00           1165 	ldx	#_fails
      0006DA 00 E4                 1166 	inx	0(ix)
      0006DC                       1167 00133$:
      0006DC 00 80                 1168 	ldi	#0x00
      0006DE 80 A0                 1169 	push	a
      0006E0 06 80                 1170 	ldi	#0x06
      0006E2 80 A0                 1171 	push	a
      0006E4 6A 00                 1172 	jal	_puthex
      0006E6 02 94                 1173 	ads	#2
      0006E8 0A 80                 1174 	ldi	#0x0a
      0006EA 80 A0                 1175 	push	a
      0006EC 40 00                 1176 	jal	_putc
      0006EE 01 94                 1177 	ads	#1
                                   1178 ;	test_core.c: 33: CHECK(7, sar16(-64) == -8);
      0006F0 FF 80                 1179 	ldi	#0xff
      0006F2 80 A0                 1180 	push	a
      0006F4 C0 80                 1181 	ldi	#0xc0
      0006F6 80 A0                 1182 	push	a
      0006F8 FE 97                 1183 	ads	#-2
      0006FA 8C 01                 1184 	jal	_sar16
      0006FC 01 F2                 1185 	ldax	1(sp)
      0006FE 0A FA                 1186 	stax	10(sp)
      000700 02 F2                 1187 	ldax	2(sp)
      000702 0B FA                 1188 	stax	11(sp)
      000704 04 94                 1189 	ads	#4
      000706 06 F2                 1190 	ldax	6(sp)
      000708 F8 A4                 1191 	cpi	#0xf8
      00070A 03 A8                 1192 	bnz	00551$
      00070C 07 F2                 1193 	ldax	7(sp)
      00070E FF A4                 1194 	cpi	#0xff
      000710                       1195 00551$:
      000710 08 A8                 1196 	bnz	00138$
      000712 84 80                 1197 	ldi	#>(___str_1 + 0)
      000714 80 A0                 1198 	push	a
      000716 9C 80                 1199 	ldi	#<(___str_1 + 0)
      000718 80 A0                 1200 	push	a
      00071A 49 00                 1201 	jal	_puts
      00071C 02 94                 1202 	ads	#2
      00071E 0A B0                 1203 	br	00139$
      000720                       1204 00138$:
      000720 84 80                 1205 	ldi	#>(___str_2 + 0)
      000722 80 A0                 1206 	push	a
      000724 A0 80                 1207 	ldi	#<(___str_2 + 0)
      000726 80 A0                 1208 	push	a
      000728 49 00                 1209 	jal	_puts
      00072A 02 94                 1210 	ads	#2
      00072C 80 A1 00 00           1211 	ldx	#_fails
      000730 00 E4                 1212 	inx	0(ix)
      000732                       1213 00139$:
      000732 00 80                 1214 	ldi	#0x00
      000734 80 A0                 1215 	push	a
      000736 07 80                 1216 	ldi	#0x07
      000738 80 A0                 1217 	push	a
      00073A 6A 00                 1218 	jal	_puthex
      00073C 02 94                 1219 	ads	#2
      00073E 0A 80                 1220 	ldi	#0x0a
      000740 80 A0                 1221 	push	a
      000742 40 00                 1222 	jal	_putc
      000744 01 94                 1223 	ads	#1
                                   1224 ;	test_core.c: 34: CHECK(8, cmpu(3, 200) == 1 && cmpu(200, 3) == 0);
      000746 C8 80                 1225 	ldi	#0xc8
      000748 80 A0                 1226 	push	a
      00074A 03 80                 1227 	ldi	#0x03
      00074C 80 A0                 1228 	push	a
      00074E AE 01                 1229 	jal	_cmpu
      000750 0A FA                 1230 	stax	10(sp)
      000752 02 94                 1231 	ads	#2
      000754 08 F2                 1232 	ldax	8(sp)
      000756 01 A4                 1233 	cpi	#0x01
      000758 11 A8                 1234 	bnz	00144$
      00075A 03 80                 1235 	ldi	#0x03
      00075C 80 A0                 1236 	push	a
      00075E C8 80                 1237 	ldi	#0xc8
      000760 80 A0                 1238 	push	a
      000762 AE 01                 1239 	jal	_cmpu
      000764 0A FA                 1240 	stax	10(sp)
      000766 02 94                 1241 	ads	#2
      000768 08 F2                 1242 	ldax	8(sp)
      00076A 08 A8                 1243 	bnz	00144$
      00076C 84 80                 1244 	ldi	#>(___str_1 + 0)
      00076E 80 A0                 1245 	push	a
      000770 9C 80                 1246 	ldi	#<(___str_1 + 0)
      000772 80 A0                 1247 	push	a
      000774 49 00                 1248 	jal	_puts
      000776 02 94                 1249 	ads	#2
      000778 0A B0                 1250 	br	00145$
      00077A                       1251 00144$:
      00077A 84 80                 1252 	ldi	#>(___str_2 + 0)
      00077C 80 A0                 1253 	push	a
      00077E A0 80                 1254 	ldi	#<(___str_2 + 0)
      000780 80 A0                 1255 	push	a
      000782 49 00                 1256 	jal	_puts
      000784 02 94                 1257 	ads	#2
      000786 80 A1 00 00           1258 	ldx	#_fails
      00078A 00 E4                 1259 	inx	0(ix)
      00078C                       1260 00145$:
      00078C 00 80                 1261 	ldi	#0x00
      00078E 80 A0                 1262 	push	a
      000790 08 80                 1263 	ldi	#0x08
      000792 80 A0                 1264 	push	a
      000794 6A 00                 1265 	jal	_puthex
      000796 02 94                 1266 	ads	#2
      000798 0A 80                 1267 	ldi	#0x0a
      00079A 80 A0                 1268 	push	a
      00079C 40 00                 1269 	jal	_putc
      00079E 01 94                 1270 	ads	#1
                                   1271 ;	test_core.c: 35: CHECK(9, cmps(-3, 2) == 1 && cmps(2, -3) == 0 && cmps(-3, -3) == 0);
      0007A0 02 80                 1272 	ldi	#0x02
      0007A2 80 A0                 1273 	push	a
      0007A4 FD 80                 1274 	ldi	#0xfd
      0007A6 80 A0                 1275 	push	a
      0007A8 B5 01                 1276 	jal	_cmps
      0007AA 0A FA                 1277 	stax	10(sp)
      0007AC 02 94                 1278 	ads	#2
      0007AE 08 F2                 1279 	ldax	8(sp)
      0007B0 01 A4                 1280 	cpi	#0x01
      0007B2 1A A8                 1281 	bnz	00151$
      0007B4 FD 80                 1282 	ldi	#0xfd
      0007B6 80 A0                 1283 	push	a
      0007B8 02 80                 1284 	ldi	#0x02
      0007BA 80 A0                 1285 	push	a
      0007BC B5 01                 1286 	jal	_cmps
      0007BE 0A FA                 1287 	stax	10(sp)
      0007C0 02 94                 1288 	ads	#2
      0007C2 08 F2                 1289 	ldax	8(sp)
      0007C4 11 A8                 1290 	bnz	00151$
      0007C6 FD 80                 1291 	ldi	#0xfd
      0007C8 80 A0                 1292 	push	a
      0007CA FD 80                 1293 	ldi	#0xfd
      0007CC 80 A0                 1294 	push	a
      0007CE B5 01                 1295 	jal	_cmps
      0007D0 0A FA                 1296 	stax	10(sp)
      0007D2 02 94                 1297 	ads	#2
      0007D4 08 F2                 1298 	ldax	8(sp)
      0007D6 08 A8                 1299 	bnz	00151$
      0007D8 84 80                 1300 	ldi	#>(___str_1 + 0)
      0007DA 80 A0                 1301 	push	a
      0007DC 9C 80                 1302 	ldi	#<(___str_1 + 0)
      0007DE 80 A0                 1303 	push	a
      0007E0 49 00                 1304 	jal	_puts
      0007E2 02 94                 1305 	ads	#2
      0007E4 0A B0                 1306 	br	00152$
      0007E6                       1307 00151$:
      0007E6 84 80                 1308 	ldi	#>(___str_2 + 0)
      0007E8 80 A0                 1309 	push	a
      0007EA A0 80                 1310 	ldi	#<(___str_2 + 0)
      0007EC 80 A0                 1311 	push	a
      0007EE 49 00                 1312 	jal	_puts
      0007F0 02 94                 1313 	ads	#2
      0007F2 80 A1 00 00           1314 	ldx	#_fails
      0007F6 00 E4                 1315 	inx	0(ix)
      0007F8                       1316 00152$:
      0007F8 00 80                 1317 	ldi	#0x00
      0007FA 80 A0                 1318 	push	a
      0007FC 09 80                 1319 	ldi	#0x09
      0007FE 80 A0                 1320 	push	a
      000800 6A 00                 1321 	jal	_puthex
      000802 02 94                 1322 	ads	#2
      000804 0A 80                 1323 	ldi	#0x0a
      000806 80 A0                 1324 	push	a
      000808 40 00                 1325 	jal	_putc
      00080A 01 94                 1326 	ads	#1
                                   1327 ;	test_core.c: 36: CHECK(10, cmp16(-300, 200) == 1 && cmp16(200, -300) == 0 && cmp16(-300, -301) == 0);
      00080C 00 80                 1328 	ldi	#0x00
      00080E 80 A0                 1329 	push	a
      000810 C8 80                 1330 	ldi	#0xc8
      000812 80 A0                 1331 	push	a
      000814 FE 80                 1332 	ldi	#0xfe
      000816 80 A0                 1333 	push	a
      000818 D4 80                 1334 	ldi	#0xd4
      00081A 80 A0                 1335 	push	a
      00081C BE 01                 1336 	jal	_cmp16
      00081E 0C FA                 1337 	stax	12(sp)
      000820 04 94                 1338 	ads	#4
      000822 08 F2                 1339 	ldax	8(sp)
      000824 01 A4                 1340 	cpi	#0x01
      000826 22 A8                 1341 	bnz	00159$
      000828 FE 80                 1342 	ldi	#0xfe
      00082A 80 A0                 1343 	push	a
      00082C D4 80                 1344 	ldi	#0xd4
      00082E 80 A0                 1345 	push	a
      000830 00 80                 1346 	ldi	#0x00
      000832 80 A0                 1347 	push	a
      000834 C8 80                 1348 	ldi	#0xc8
      000836 80 A0                 1349 	push	a
      000838 BE 01                 1350 	jal	_cmp16
      00083A 0C FA                 1351 	stax	12(sp)
      00083C 04 94                 1352 	ads	#4
      00083E 08 F2                 1353 	ldax	8(sp)
      000840 15 A8                 1354 	bnz	00159$
      000842 FE 80                 1355 	ldi	#0xfe
      000844 80 A0                 1356 	push	a
      000846 D3 80                 1357 	ldi	#0xd3
      000848 80 A0                 1358 	push	a
      00084A FE 80                 1359 	ldi	#0xfe
      00084C 80 A0                 1360 	push	a
      00084E D4 80                 1361 	ldi	#0xd4
      000850 80 A0                 1362 	push	a
      000852 BE 01                 1363 	jal	_cmp16
      000854 0C FA                 1364 	stax	12(sp)
      000856 04 94                 1365 	ads	#4
      000858 08 F2                 1366 	ldax	8(sp)
      00085A 08 A8                 1367 	bnz	00159$
      00085C 84 80                 1368 	ldi	#>(___str_1 + 0)
      00085E 80 A0                 1369 	push	a
      000860 9C 80                 1370 	ldi	#<(___str_1 + 0)
      000862 80 A0                 1371 	push	a
      000864 49 00                 1372 	jal	_puts
      000866 02 94                 1373 	ads	#2
      000868 0A B0                 1374 	br	00160$
      00086A                       1375 00159$:
      00086A 84 80                 1376 	ldi	#>(___str_2 + 0)
      00086C 80 A0                 1377 	push	a
      00086E A0 80                 1378 	ldi	#<(___str_2 + 0)
      000870 80 A0                 1379 	push	a
      000872 49 00                 1380 	jal	_puts
      000874 02 94                 1381 	ads	#2
      000876 80 A1 00 00           1382 	ldx	#_fails
      00087A 00 E4                 1383 	inx	0(ix)
      00087C                       1384 00160$:
      00087C 00 80                 1385 	ldi	#0x00
      00087E 80 A0                 1386 	push	a
      000880 0A 80                 1387 	ldi	#0x0a
      000882 80 A0                 1388 	push	a
      000884 6A 00                 1389 	jal	_puthex
      000886 02 94                 1390 	ads	#2
      000888 0A 80                 1391 	ldi	#0x0a
      00088A 80 A0                 1392 	push	a
      00088C 40 00                 1393 	jal	_putc
      00088E 01 94                 1394 	ads	#1
                                   1395 ;	test_core.c: 37: CHECK(11, cmp16u(0x8000, 0x7fff) == 1 && cmp16u(1, 2) == 0);
      000890 7F 80                 1396 	ldi	#0x7f
      000892 80 A0                 1397 	push	a
      000894 FF 80                 1398 	ldi	#0xff
      000896 80 A0                 1399 	push	a
      000898 80 80                 1400 	ldi	#0x80
      00089A 80 A0                 1401 	push	a
      00089C 00 80                 1402 	ldi	#0x00
      00089E 80 A0                 1403 	push	a
      0008A0 CF 01                 1404 	jal	_cmp16u
      0008A2 0C FA                 1405 	stax	12(sp)
      0008A4 04 94                 1406 	ads	#4
      0008A6 08 F2                 1407 	ldax	8(sp)
      0008A8 01 A4                 1408 	cpi	#0x01
      0008AA 15 A8                 1409 	bnz	00167$
      0008AC 00 80                 1410 	ldi	#0x00
      0008AE 80 A0                 1411 	push	a
      0008B0 02 80                 1412 	ldi	#0x02
      0008B2 80 A0                 1413 	push	a
      0008B4 00 80                 1414 	ldi	#0x00
      0008B6 80 A0                 1415 	push	a
      0008B8 01 80                 1416 	ldi	#0x01
      0008BA 80 A0                 1417 	push	a
      0008BC CF 01                 1418 	jal	_cmp16u
      0008BE 0C FA                 1419 	stax	12(sp)
      0008C0 04 94                 1420 	ads	#4
      0008C2 08 F2                 1421 	ldax	8(sp)
      0008C4 08 A8                 1422 	bnz	00167$
      0008C6 84 80                 1423 	ldi	#>(___str_1 + 0)
      0008C8 80 A0                 1424 	push	a
      0008CA 9C 80                 1425 	ldi	#<(___str_1 + 0)
      0008CC 80 A0                 1426 	push	a
      0008CE 49 00                 1427 	jal	_puts
      0008D0 02 94                 1428 	ads	#2
      0008D2 0A B0                 1429 	br	00168$
      0008D4                       1430 00167$:
      0008D4 84 80                 1431 	ldi	#>(___str_2 + 0)
      0008D6 80 A0                 1432 	push	a
      0008D8 A0 80                 1433 	ldi	#<(___str_2 + 0)
      0008DA 80 A0                 1434 	push	a
      0008DC 49 00                 1435 	jal	_puts
      0008DE 02 94                 1436 	ads	#2
      0008E0 80 A1 00 00           1437 	ldx	#_fails
      0008E4 00 E4                 1438 	inx	0(ix)
      0008E6                       1439 00168$:
      0008E6 00 80                 1440 	ldi	#0x00
      0008E8 80 A0                 1441 	push	a
      0008EA 0B 80                 1442 	ldi	#0x0b
      0008EC 80 A0                 1443 	push	a
      0008EE 6A 00                 1444 	jal	_puthex
      0008F0 02 94                 1445 	ads	#2
      0008F2 0A 80                 1446 	ldi	#0x0a
      0008F4 80 A0                 1447 	push	a
      0008F6 40 00                 1448 	jal	_putc
      0008F8 01 94                 1449 	ads	#1
                                   1450 ;	test_core.c: 38: CHECK(12, mul16(300, 7) == 2100);
      0008FA 00 80                 1451 	ldi	#0x00
      0008FC 80 A0                 1452 	push	a
      0008FE 07 80                 1453 	ldi	#0x07
      000900 80 A0                 1454 	push	a
      000902 01 80                 1455 	ldi	#0x01
      000904 80 A0                 1456 	push	a
      000906 2C 80                 1457 	ldi	#0x2c
      000908 80 A0                 1458 	push	a
      00090A FE 97                 1459 	ads	#-2
      00090C E0 01                 1460 	jal	_mul16
      00090E 01 F2                 1461 	ldax	1(sp)
      000910 0C FA                 1462 	stax	12(sp)
      000912 02 F2                 1463 	ldax	2(sp)
      000914 0D FA                 1464 	stax	13(sp)
      000916 06 94                 1465 	ads	#6
      000918 06 F2                 1466 	ldax	6(sp)
      00091A 34 A4                 1467 	cpi	#0x34
      00091C 03 A8                 1468 	bnz	00561$
      00091E 07 F2                 1469 	ldax	7(sp)
      000920 08 A4                 1470 	cpi	#0x08
      000922                       1471 00561$:
      000922 08 A8                 1472 	bnz	00174$
      000924 84 80                 1473 	ldi	#>(___str_1 + 0)
      000926 80 A0                 1474 	push	a
      000928 9C 80                 1475 	ldi	#<(___str_1 + 0)
      00092A 80 A0                 1476 	push	a
      00092C 49 00                 1477 	jal	_puts
      00092E 02 94                 1478 	ads	#2
      000930 0A B0                 1479 	br	00175$
      000932                       1480 00174$:
      000932 84 80                 1481 	ldi	#>(___str_2 + 0)
      000934 80 A0                 1482 	push	a
      000936 A0 80                 1483 	ldi	#<(___str_2 + 0)
      000938 80 A0                 1484 	push	a
      00093A 49 00                 1485 	jal	_puts
      00093C 02 94                 1486 	ads	#2
      00093E 80 A1 00 00           1487 	ldx	#_fails
      000942 00 E4                 1488 	inx	0(ix)
      000944                       1489 00175$:
      000944 00 80                 1490 	ldi	#0x00
      000946 80 A0                 1491 	push	a
      000948 0C 80                 1492 	ldi	#0x0c
      00094A 80 A0                 1493 	push	a
      00094C 6A 00                 1494 	jal	_puthex
      00094E 02 94                 1495 	ads	#2
      000950 0A 80                 1496 	ldi	#0x0a
      000952 80 A0                 1497 	push	a
      000954 40 00                 1498 	jal	_putc
      000956 01 94                 1499 	ads	#1
                                   1500 ;	test_core.c: 39: CHECK(13, mul8s(-5, 7) == -35 && mul8s(-5, -7) == 35);
      000958 07 80                 1501 	ldi	#0x07
      00095A 80 A0                 1502 	push	a
      00095C FB 80                 1503 	ldi	#0xfb
      00095E 80 A0                 1504 	push	a
      000960 FE 97                 1505 	ads	#-2
      000962 F9 01                 1506 	jal	_mul8s
      000964 01 F2                 1507 	ldax	1(sp)
      000966 0A FA                 1508 	stax	10(sp)
      000968 02 F2                 1509 	ldax	2(sp)
      00096A 0B FA                 1510 	stax	11(sp)
      00096C 04 94                 1511 	ads	#4
      00096E 06 F2                 1512 	ldax	6(sp)
      000970 DD A4                 1513 	cpi	#0xdd
      000972 03 A8                 1514 	bnz	00563$
      000974 07 F2                 1515 	ldax	7(sp)
      000976 FF A4                 1516 	cpi	#0xff
      000978                       1517 00563$:
      000978 19 A8                 1518 	bnz	00180$
      00097A F9 80                 1519 	ldi	#0xf9
      00097C 80 A0                 1520 	push	a
      00097E FB 80                 1521 	ldi	#0xfb
      000980 80 A0                 1522 	push	a
      000982 FE 97                 1523 	ads	#-2
      000984 F9 01                 1524 	jal	_mul8s
      000986 01 F2                 1525 	ldax	1(sp)
      000988 0A FA                 1526 	stax	10(sp)
      00098A 02 F2                 1527 	ldax	2(sp)
      00098C 0B FA                 1528 	stax	11(sp)
      00098E 04 94                 1529 	ads	#4
      000990 06 F2                 1530 	ldax	6(sp)
      000992 23 A4                 1531 	cpi	#0x23
      000994 03 A8                 1532 	bnz	00565$
      000996 07 F2                 1533 	ldax	7(sp)
      000998 00 A4                 1534 	cpi	#0x00
      00099A                       1535 00565$:
      00099A 08 A8                 1536 	bnz	00180$
      00099C 84 80                 1537 	ldi	#>(___str_1 + 0)
      00099E 80 A0                 1538 	push	a
      0009A0 9C 80                 1539 	ldi	#<(___str_1 + 0)
      0009A2 80 A0                 1540 	push	a
      0009A4 49 00                 1541 	jal	_puts
      0009A6 02 94                 1542 	ads	#2
      0009A8 0A B0                 1543 	br	00181$
      0009AA                       1544 00180$:
      0009AA 84 80                 1545 	ldi	#>(___str_2 + 0)
      0009AC 80 A0                 1546 	push	a
      0009AE A0 80                 1547 	ldi	#<(___str_2 + 0)
      0009B0 80 A0                 1548 	push	a
      0009B2 49 00                 1549 	jal	_puts
      0009B4 02 94                 1550 	ads	#2
      0009B6 80 A1 00 00           1551 	ldx	#_fails
      0009BA 00 E4                 1552 	inx	0(ix)
      0009BC                       1553 00181$:
      0009BC 00 80                 1554 	ldi	#0x00
      0009BE 80 A0                 1555 	push	a
      0009C0 0D 80                 1556 	ldi	#0x0d
      0009C2 80 A0                 1557 	push	a
      0009C4 6A 00                 1558 	jal	_puthex
      0009C6 02 94                 1559 	ads	#2
      0009C8 0A 80                 1560 	ldi	#0x0a
      0009CA 80 A0                 1561 	push	a
      0009CC 40 00                 1562 	jal	_putc
      0009CE 01 94                 1563 	ads	#1
                                   1564 ;	test_core.c: 40: CHECK(14, sum(arr, 4) == 10);
      0009D0 04 80                 1565 	ldi	#0x04
      0009D2 80 A0                 1566 	push	a
      0009D4 00 80                 1567 	ldi	#>(_arr + 0)
      0009D6 80 A0                 1568 	push	a
      0009D8 0A 80                 1569 	ldi	#<(_arr + 0)
      0009DA 80 A0                 1570 	push	a
      0009DC 09 02                 1571 	jal	_sum
      0009DE 0B FA                 1572 	stax	11(sp)
      0009E0 03 94                 1573 	ads	#3
      0009E2 08 F2                 1574 	ldax	8(sp)
      0009E4 0A A4                 1575 	cpi	#0x0a
      0009E6 08 A8                 1576 	bnz	00187$
      0009E8 84 80                 1577 	ldi	#>(___str_1 + 0)
      0009EA 80 A0                 1578 	push	a
      0009EC 9C 80                 1579 	ldi	#<(___str_1 + 0)
      0009EE 80 A0                 1580 	push	a
      0009F0 49 00                 1581 	jal	_puts
      0009F2 02 94                 1582 	ads	#2
      0009F4 0A B0                 1583 	br	00188$
      0009F6                       1584 00187$:
      0009F6 84 80                 1585 	ldi	#>(___str_2 + 0)
      0009F8 80 A0                 1586 	push	a
      0009FA A0 80                 1587 	ldi	#<(___str_2 + 0)
      0009FC 80 A0                 1588 	push	a
      0009FE 49 00                 1589 	jal	_puts
      000A00 02 94                 1590 	ads	#2
      000A02 80 A1 00 00           1591 	ldx	#_fails
      000A06 00 E4                 1592 	inx	0(ix)
      000A08                       1593 00188$:
      000A08 00 80                 1594 	ldi	#0x00
      000A0A 80 A0                 1595 	push	a
      000A0C 0E 80                 1596 	ldi	#0x0e
      000A0E 80 A0                 1597 	push	a
      000A10 6A 00                 1598 	jal	_puthex
      000A12 02 94                 1599 	ads	#2
      000A14 0A 80                 1600 	ldi	#0x0a
      000A16 80 A0                 1601 	push	a
      000A18 40 00                 1602 	jal	_putc
      000A1A 01 94                 1603 	ads	#1
                                   1604 ;	test_core.c: 41: CHECK(15, sum(ctab, 4) == 100);
      000A1C 04 80                 1605 	ldi	#0x04
      000A1E 80 A0                 1606 	push	a
      000A20 84 80                 1607 	ldi	#>(_ctab + 0)
      000A22 80 A0                 1608 	push	a
      000A24 92 80                 1609 	ldi	#<(_ctab + 0)
      000A26 80 A0                 1610 	push	a
      000A28 09 02                 1611 	jal	_sum
      000A2A 0B FA                 1612 	stax	11(sp)
      000A2C 03 94                 1613 	ads	#3
      000A2E 08 F2                 1614 	ldax	8(sp)
      000A30 64 A4                 1615 	cpi	#0x64
      000A32 08 A8                 1616 	bnz	00193$
      000A34 84 80                 1617 	ldi	#>(___str_1 + 0)
      000A36 80 A0                 1618 	push	a
      000A38 9C 80                 1619 	ldi	#<(___str_1 + 0)
      000A3A 80 A0                 1620 	push	a
      000A3C 49 00                 1621 	jal	_puts
      000A3E 02 94                 1622 	ads	#2
      000A40 0A B0                 1623 	br	00194$
      000A42                       1624 00193$:
      000A42 84 80                 1625 	ldi	#>(___str_2 + 0)
      000A44 80 A0                 1626 	push	a
      000A46 A0 80                 1627 	ldi	#<(___str_2 + 0)
      000A48 80 A0                 1628 	push	a
      000A4A 49 00                 1629 	jal	_puts
      000A4C 02 94                 1630 	ads	#2
      000A4E 80 A1 00 00           1631 	ldx	#_fails
      000A52 00 E4                 1632 	inx	0(ix)
      000A54                       1633 00194$:
      000A54 00 80                 1634 	ldi	#0x00
      000A56 80 A0                 1635 	push	a
      000A58 0F 80                 1636 	ldi	#0x0f
      000A5A 80 A0                 1637 	push	a
      000A5C 6A 00                 1638 	jal	_puthex
      000A5E 02 94                 1639 	ads	#2
      000A60 0A 80                 1640 	ldi	#0x0a
      000A62 80 A0                 1641 	push	a
      000A64 40 00                 1642 	jal	_putc
      000A66 01 94                 1643 	ads	#1
                                   1644 ;	test_core.c: 42: CHECK(16, citab[1] == 300 && citab[2] == 32000 && citab[0] == -1);
      000A68 80 A1 30 89           1645 	ldx	#(_citab + 8)
      000A6C 80 8A                 1646 	call	ix
      000A6E 01 98                 1647 	adx	#1
      000A70 06 FA                 1648 	stax	6(sp)
      000A72 80 8A                 1649 	call	ix
      000A74 07 FA                 1650 	stax	7(sp)
      000A76 06 F2                 1651 	ldax	6(sp)
      000A78 2C A4                 1652 	cpi	#0x2c
      000A7A 03 A8                 1653 	bnz	00571$
      000A7C 07 F2                 1654 	ldax	7(sp)
      000A7E 01 A4                 1655 	cpi	#0x01
      000A80                       1656 00571$:
      000A80 22 A8                 1657 	bnz	00199$
      000A82 80 A1 34 89           1658 	ldx	#(_citab + 16)
      000A86 80 8A                 1659 	call	ix
      000A88 01 98                 1660 	adx	#1
      000A8A 06 FA                 1661 	stax	6(sp)
      000A8C 80 8A                 1662 	call	ix
      000A8E 07 FA                 1663 	stax	7(sp)
      000A90 06 F2                 1664 	ldax	6(sp)
      000A92 00 A4                 1665 	cpi	#0x00
      000A94 03 A8                 1666 	bnz	00573$
      000A96 07 F2                 1667 	ldax	7(sp)
      000A98 7D A4                 1668 	cpi	#0x7d
      000A9A                       1669 00573$:
      000A9A 15 A8                 1670 	bnz	00199$
      000A9C 80 A1 2C 89           1671 	ldx	#_citab
      000AA0 80 8A                 1672 	call	ix
      000AA2 01 98                 1673 	adx	#1
      000AA4 06 FA                 1674 	stax	6(sp)
      000AA6 80 8A                 1675 	call	ix
      000AA8 07 FA                 1676 	stax	7(sp)
      000AAA 06 F2                 1677 	ldax	6(sp)
      000AAC FF A4                 1678 	cpi	#0xff
      000AAE 03 A8                 1679 	bnz	00575$
      000AB0 07 F2                 1680 	ldax	7(sp)
      000AB2 FF A4                 1681 	cpi	#0xff
      000AB4                       1682 00575$:
      000AB4 08 A8                 1683 	bnz	00199$
      000AB6 84 80                 1684 	ldi	#>(___str_1 + 0)
      000AB8 80 A0                 1685 	push	a
      000ABA 9C 80                 1686 	ldi	#<(___str_1 + 0)
      000ABC 80 A0                 1687 	push	a
      000ABE 49 00                 1688 	jal	_puts
      000AC0 02 94                 1689 	ads	#2
      000AC2 0A B0                 1690 	br	00200$
      000AC4                       1691 00199$:
      000AC4 84 80                 1692 	ldi	#>(___str_2 + 0)
      000AC6 80 A0                 1693 	push	a
      000AC8 A0 80                 1694 	ldi	#<(___str_2 + 0)
      000ACA 80 A0                 1695 	push	a
      000ACC 49 00                 1696 	jal	_puts
      000ACE 02 94                 1697 	ads	#2
      000AD0 80 A1 00 00           1698 	ldx	#_fails
      000AD4 00 E4                 1699 	inx	0(ix)
      000AD6                       1700 00200$:
      000AD6 00 80                 1701 	ldi	#0x00
      000AD8 80 A0                 1702 	push	a
      000ADA 10 80                 1703 	ldi	#0x10
      000ADC 80 A0                 1704 	push	a
      000ADE 6A 00                 1705 	jal	_puthex
      000AE0 02 94                 1706 	ads	#2
      000AE2 0A 80                 1707 	ldi	#0x0a
      000AE4 80 A0                 1708 	push	a
      000AE6 40 00                 1709 	jal	_putc
      000AE8 01 94                 1710 	ads	#1
                                   1711 ;	test_core.c: 43: fill(arr, 4, 9);
      000AEA 09 80                 1712 	ldi	#0x09
      000AEC 80 A0                 1713 	push	a
      000AEE 04 80                 1714 	ldi	#0x04
      000AF0 80 A0                 1715 	push	a
      000AF2 00 80                 1716 	ldi	#>(_arr + 0)
      000AF4 80 A0                 1717 	push	a
      000AF6 0A 80                 1718 	ldi	#<(_arr + 0)
      000AF8 80 A0                 1719 	push	a
      000AFA 32 02                 1720 	jal	_fill
      000AFC 04 94                 1721 	ads	#4
                                   1722 ;	test_core.c: 44: CHECK(17, arr[0] == 9 && arr[3] == 12);
      000AFE 0A F4                 1723 	lda	_arr
      000B00 08 FA                 1724 	stax	8(sp)
      000B02 09 A4                 1725 	cpi	#0x09
      000B04 0C A8                 1726 	bnz	00207$
      000B06 0D F4                 1727 	lda	_arr+3
      000B08 08 FA                 1728 	stax	8(sp)
      000B0A 0C A4                 1729 	cpi	#0x0c
      000B0C 08 A8                 1730 	bnz	00207$
      000B0E 84 80                 1731 	ldi	#>(___str_1 + 0)
      000B10 80 A0                 1732 	push	a
      000B12 9C 80                 1733 	ldi	#<(___str_1 + 0)
      000B14 80 A0                 1734 	push	a
      000B16 49 00                 1735 	jal	_puts
      000B18 02 94                 1736 	ads	#2
      000B1A 0A B0                 1737 	br	00208$
      000B1C                       1738 00207$:
      000B1C 84 80                 1739 	ldi	#>(___str_2 + 0)
      000B1E 80 A0                 1740 	push	a
      000B20 A0 80                 1741 	ldi	#<(___str_2 + 0)
      000B22 80 A0                 1742 	push	a
      000B24 49 00                 1743 	jal	_puts
      000B26 02 94                 1744 	ads	#2
      000B28 80 A1 00 00           1745 	ldx	#_fails
      000B2C 00 E4                 1746 	inx	0(ix)
      000B2E                       1747 00208$:
      000B2E 00 80                 1748 	ldi	#0x00
      000B30 80 A0                 1749 	push	a
      000B32 11 80                 1750 	ldi	#0x11
      000B34 80 A0                 1751 	push	a
      000B36 6A 00                 1752 	jal	_puthex
      000B38 02 94                 1753 	ads	#2
      000B3A 0A 80                 1754 	ldi	#0x0a
      000B3C 80 A0                 1755 	push	a
      000B3E 40 00                 1756 	jal	_putc
      000B40 01 94                 1757 	ads	#1
                                   1758 ;	test_core.c: 45: CHECK(18, g8 == 7 && g16 == -1234 && g32 == 0x12345678);
      000B42 03 F4                 1759 	lda	_g8
      000B44 07 A4                 1760 	cpi	#0x07
      000B46 1A A8                 1761 	bnz	00214$
      000B48 04 F4                 1762 	lda	_g16
      000B4A 2E A4                 1763 	cpi	#0x2e
      000B4C 03 A8                 1764 	bnz	00583$
      000B4E 05 F4                 1765 	lda	_g16+1
      000B50 FB A4                 1766 	cpi	#0xfb
      000B52                       1767 00583$:
      000B52 14 A8                 1768 	bnz	00214$
      000B54 06 F4                 1769 	lda	_g32
      000B56 78 A4                 1770 	cpi	#0x78
      000B58 09 A8                 1771 	bnz	00585$
      000B5A 07 F4                 1772 	lda	_g32+1
      000B5C 56 A4                 1773 	cpi	#0x56
      000B5E 06 A8                 1774 	bnz	00585$
      000B60 08 F4                 1775 	lda	_g32+2
      000B62 34 A4                 1776 	cpi	#0x34
      000B64 03 A8                 1777 	bnz	00585$
      000B66 09 F4                 1778 	lda	_g32+3
      000B68 12 A4                 1779 	cpi	#0x12
      000B6A                       1780 00585$:
      000B6A 08 A8                 1781 	bnz	00214$
      000B6C 84 80                 1782 	ldi	#>(___str_1 + 0)
      000B6E 80 A0                 1783 	push	a
      000B70 9C 80                 1784 	ldi	#<(___str_1 + 0)
      000B72 80 A0                 1785 	push	a
      000B74 49 00                 1786 	jal	_puts
      000B76 02 94                 1787 	ads	#2
      000B78 0A B0                 1788 	br	00215$
      000B7A                       1789 00214$:
      000B7A 84 80                 1790 	ldi	#>(___str_2 + 0)
      000B7C 80 A0                 1791 	push	a
      000B7E A0 80                 1792 	ldi	#<(___str_2 + 0)
      000B80 80 A0                 1793 	push	a
      000B82 49 00                 1794 	jal	_puts
      000B84 02 94                 1795 	ads	#2
      000B86 80 A1 00 00           1796 	ldx	#_fails
      000B8A 00 E4                 1797 	inx	0(ix)
      000B8C                       1798 00215$:
      000B8C 00 80                 1799 	ldi	#0x00
      000B8E 80 A0                 1800 	push	a
      000B90 12 80                 1801 	ldi	#0x12
      000B92 80 A0                 1802 	push	a
      000B94 6A 00                 1803 	jal	_puthex
      000B96 02 94                 1804 	ads	#2
      000B98 0A 80                 1805 	ldi	#0x0a
      000B9A 80 A0                 1806 	push	a
      000B9C 40 00                 1807 	jal	_putc
      000B9E 01 94                 1808 	ads	#1
                                   1809 ;	test_core.c: 46: s = 0; for (i = 0; i < 10; i++) s += i;
      000BA0 00 80                 1810 	ldi	#0x00
      000BA2 08 FA                 1811 	stax	8(sp)
      000BA4 00 80                 1812 	ldi	#0x00
      000BA6 01 FA                 1813 	stax	1(sp)
      000BA8                       1814 00268$:
      000BA8 08 F2                 1815 	ldax	8(sp)
      000BAA 08 A0                 1816 	ldc	#0
      000BAC 01 C2                 1817 	add	1(sp)
      000BAE 08 FA                 1818 	stax	8(sp)
      000BB0 01 E6                 1819 	inx	1(sp)
      000BB2 01 F2                 1820 	ldax	1(sp)
      000BB4 0A A4                 1821 	cpi	#0x0a
      000BB6 05 A2                 1822 	if	lt
      000BB8 F8 B7                 1823 	br.p	00268$
                                   1824 ;	test_core.c: 47: CHECK(19, s == 45);
      000BBA 08 F2                 1825 	ldax	8(sp)
      000BBC 2D A4                 1826 	cpi	#0x2d
      000BBE 08 A8                 1827 	bnz	00223$
      000BC0 84 80                 1828 	ldi	#>(___str_1 + 0)
      000BC2 80 A0                 1829 	push	a
      000BC4 9C 80                 1830 	ldi	#<(___str_1 + 0)
      000BC6 80 A0                 1831 	push	a
      000BC8 49 00                 1832 	jal	_puts
      000BCA 02 94                 1833 	ads	#2
      000BCC 0A B0                 1834 	br	00224$
      000BCE                       1835 00223$:
      000BCE 84 80                 1836 	ldi	#>(___str_2 + 0)
      000BD0 80 A0                 1837 	push	a
      000BD2 A0 80                 1838 	ldi	#<(___str_2 + 0)
      000BD4 80 A0                 1839 	push	a
      000BD6 49 00                 1840 	jal	_puts
      000BD8 02 94                 1841 	ads	#2
      000BDA 80 A1 00 00           1842 	ldx	#_fails
      000BDE 00 E4                 1843 	inx	0(ix)
      000BE0                       1844 00224$:
      000BE0 00 80                 1845 	ldi	#0x00
      000BE2 80 A0                 1846 	push	a
      000BE4 13 80                 1847 	ldi	#0x13
      000BE6 80 A0                 1848 	push	a
      000BE8 6A 00                 1849 	jal	_puthex
      000BEA 02 94                 1850 	ads	#2
      000BEC 0A 80                 1851 	ldi	#0x0a
      000BEE 80 A0                 1852 	push	a
      000BF0 40 00                 1853 	jal	_putc
      000BF2 01 94                 1854 	ads	#1
                                   1855 ;	test_core.c: 48: CHECK(20, sw(0) == 10 && sw(2) == 12 && sw(5) == 15 && sw(3) == 99);
      000BF4 00 80                 1856 	ldi	#0x00
      000BF6 80 A0                 1857 	push	a
      000BF8 4C 02                 1858 	jal	_sw
      000BFA 02 FA                 1859 	stax	2(sp)
      000BFC 01 94                 1860 	ads	#1
      000BFE 01 F2                 1861 	ldax	1(sp)
      000C00 0A A4                 1862 	cpi	#0x0a
      000C02 20 A8                 1863 	bnz	00229$
      000C04 02 80                 1864 	ldi	#0x02
      000C06 80 A0                 1865 	push	a
      000C08 4C 02                 1866 	jal	_sw
      000C0A 02 FA                 1867 	stax	2(sp)
      000C0C 01 94                 1868 	ads	#1
      000C0E 01 F2                 1869 	ldax	1(sp)
      000C10 0C A4                 1870 	cpi	#0x0c
      000C12 18 A8                 1871 	bnz	00229$
      000C14 05 80                 1872 	ldi	#0x05
      000C16 80 A0                 1873 	push	a
      000C18 4C 02                 1874 	jal	_sw
      000C1A 02 FA                 1875 	stax	2(sp)
      000C1C 01 94                 1876 	ads	#1
      000C1E 01 F2                 1877 	ldax	1(sp)
      000C20 0F A4                 1878 	cpi	#0x0f
      000C22 10 A8                 1879 	bnz	00229$
      000C24 03 80                 1880 	ldi	#0x03
      000C26 80 A0                 1881 	push	a
      000C28 4C 02                 1882 	jal	_sw
      000C2A 02 FA                 1883 	stax	2(sp)
      000C2C 01 94                 1884 	ads	#1
      000C2E 01 F2                 1885 	ldax	1(sp)
      000C30 63 A4                 1886 	cpi	#0x63
      000C32 08 A8                 1887 	bnz	00229$
      000C34 84 80                 1888 	ldi	#>(___str_1 + 0)
      000C36 80 A0                 1889 	push	a
      000C38 9C 80                 1890 	ldi	#<(___str_1 + 0)
      000C3A 80 A0                 1891 	push	a
      000C3C 49 00                 1892 	jal	_puts
      000C3E 02 94                 1893 	ads	#2
      000C40 0A B0                 1894 	br	00230$
      000C42                       1895 00229$:
      000C42 84 80                 1896 	ldi	#>(___str_2 + 0)
      000C44 80 A0                 1897 	push	a
      000C46 A0 80                 1898 	ldi	#<(___str_2 + 0)
      000C48 80 A0                 1899 	push	a
      000C4A 49 00                 1900 	jal	_puts
      000C4C 02 94                 1901 	ads	#2
      000C4E 80 A1 00 00           1902 	ldx	#_fails
      000C52 00 E4                 1903 	inx	0(ix)
      000C54                       1904 00230$:
      000C54 00 80                 1905 	ldi	#0x00
      000C56 80 A0                 1906 	push	a
      000C58 14 80                 1907 	ldi	#0x14
      000C5A 80 A0                 1908 	push	a
      000C5C 6A 00                 1909 	jal	_puthex
      000C5E 02 94                 1910 	ads	#2
      000C60 0A 80                 1911 	ldi	#0x0a
      000C62 80 A0                 1912 	push	a
      000C64 40 00                 1913 	jal	_putc
      000C66 01 94                 1914 	ads	#1
                                   1915 ;	test_core.c: 49: fp = add8;
      000C68 27 80                 1916 	ldi	#<(_add8 + 0)
      000C6A 01 FC                 1917 	sta	_fp
      000C6C 81 80                 1918 	ldi	#>(_add8 + 0)
      000C6E 02 FC                 1919 	sta	_fp+1
                                   1920 ;	test_core.c: 50: CHECK(21, fp(1, 2) == 3);
      000C70 02 80                 1921 	ldi	#0x02
      000C72 80 A0                 1922 	push	a
      000C74 01 80                 1923 	ldi	#0x01
      000C76 80 A0                 1924 	push	a
      000C78 01 F4                 1925 	lda	_fp
      000C7A 00 A1                 1926 	tax
      000C7C 02 F4                 1927 	lda	_fp+1
      000C7E 08 A1                 1928 	taxu
      000C80 80 8A                 1929 	call	ix
      000C82 03 FA                 1930 	stax	3(sp)
      000C84 02 94                 1931 	ads	#2
      000C86 01 F2                 1932 	ldax	1(sp)
      000C88 03 A4                 1933 	cpi	#0x03
      000C8A 08 A8                 1934 	bnz	00238$
      000C8C 84 80                 1935 	ldi	#>(___str_1 + 0)
      000C8E 80 A0                 1936 	push	a
      000C90 9C 80                 1937 	ldi	#<(___str_1 + 0)
      000C92 80 A0                 1938 	push	a
      000C94 49 00                 1939 	jal	_puts
      000C96 02 94                 1940 	ads	#2
      000C98 0A B0                 1941 	br	00239$
      000C9A                       1942 00238$:
      000C9A 84 80                 1943 	ldi	#>(___str_2 + 0)
      000C9C 80 A0                 1944 	push	a
      000C9E A0 80                 1945 	ldi	#<(___str_2 + 0)
      000CA0 80 A0                 1946 	push	a
      000CA2 49 00                 1947 	jal	_puts
      000CA4 02 94                 1948 	ads	#2
      000CA6 80 A1 00 00           1949 	ldx	#_fails
      000CAA 00 E4                 1950 	inx	0(ix)
      000CAC                       1951 00239$:
      000CAC 00 80                 1952 	ldi	#0x00
      000CAE 80 A0                 1953 	push	a
      000CB0 15 80                 1954 	ldi	#0x15
      000CB2 80 A0                 1955 	push	a
      000CB4 6A 00                 1956 	jal	_puthex
      000CB6 02 94                 1957 	ads	#2
      000CB8 0A 80                 1958 	ldi	#0x0a
      000CBA 80 A0                 1959 	push	a
      000CBC 40 00                 1960 	jal	_putc
      000CBE 01 94                 1961 	ads	#1
                                   1962 ;	test_core.c: 51: g16 = g16 * 2 + g8;
      000CC0 05 F4                 1963 	lda	_g16+1
      000CC2 07 FA                 1964 	stax	7(sp)
      000CC4 04 F4                 1965 	lda	_g16
      000CC6 06 FA                 1966 	stax	6(sp)
      000CC8 08 A0                 1967 	ldc	#0
      000CCA 06 C2                 1968 	add	6(sp)
      000CCC 06 FA                 1969 	stax	6(sp)
      000CCE 07 F2                 1970 	ldax	7(sp)
      000CD0 07 C2                 1971 	add	7(sp)
      000CD2 07 FA                 1972 	stax	7(sp)
      000CD4 06 F2                 1973 	ldax	6(sp)
      000CD6 08 A0                 1974 	ldc	#0
      000CD8 80 A1 03 00           1975 	ldx	#_g8
      000CDC 00 C0                 1976 	add	0(ix)
      000CDE 04 FC                 1977 	sta	_g16
      000CE0 07 F2                 1978 	ldax	7(sp)
      000CE2 00 90                 1979 	adc	#0x00
      000CE4 05 FC                 1980 	sta	_g16+1
                                   1981 ;	test_core.c: 52: CHECK(22, g16 == -2461);
      000CE6 04 F4                 1982 	lda	_g16
      000CE8 63 A4                 1983 	cpi	#0x63
      000CEA 03 A8                 1984 	bnz	00602$
      000CEC 05 F4                 1985 	lda	_g16+1
      000CEE F6 A4                 1986 	cpi	#0xf6
      000CF0                       1987 00602$:
      000CF0 08 A8                 1988 	bnz	00244$
      000CF2 84 80                 1989 	ldi	#>(___str_1 + 0)
      000CF4 80 A0                 1990 	push	a
      000CF6 9C 80                 1991 	ldi	#<(___str_1 + 0)
      000CF8 80 A0                 1992 	push	a
      000CFA 49 00                 1993 	jal	_puts
      000CFC 02 94                 1994 	ads	#2
      000CFE 0A B0                 1995 	br	00245$
      000D00                       1996 00244$:
      000D00 84 80                 1997 	ldi	#>(___str_2 + 0)
      000D02 80 A0                 1998 	push	a
      000D04 A0 80                 1999 	ldi	#<(___str_2 + 0)
      000D06 80 A0                 2000 	push	a
      000D08 49 00                 2001 	jal	_puts
      000D0A 02 94                 2002 	ads	#2
      000D0C 80 A1 00 00           2003 	ldx	#_fails
      000D10 00 E4                 2004 	inx	0(ix)
      000D12                       2005 00245$:
      000D12 00 80                 2006 	ldi	#0x00
      000D14 80 A0                 2007 	push	a
      000D16 16 80                 2008 	ldi	#0x16
      000D18 80 A0                 2009 	push	a
      000D1A 6A 00                 2010 	jal	_puthex
      000D1C 02 94                 2011 	ads	#2
      000D1E 0A 80                 2012 	ldi	#0x0a
      000D20 80 A0                 2013 	push	a
      000D22 40 00                 2014 	jal	_putc
      000D24 01 94                 2015 	ads	#1
                                   2016 ;	test_core.c: 53: g32 += 0x100;
      000D26 06 F4                 2017 	lda	_g32
      000D28 08 A0                 2018 	ldc	#0
      000D2A 00 90                 2019 	adc	#0x00
      000D2C 06 FC                 2020 	sta	_g32
      000D2E 07 F4                 2021 	lda	_g32+1
      000D30 01 90                 2022 	adc	#0x01
      000D32 07 FC                 2023 	sta	_g32+1
      000D34 08 F4                 2024 	lda	_g32+2
      000D36 00 90                 2025 	adc	#0x00
      000D38 08 FC                 2026 	sta	_g32+2
      000D3A 09 F4                 2027 	lda	_g32+3
      000D3C 00 90                 2028 	adc	#0x00
      000D3E 09 FC                 2029 	sta	_g32+3
                                   2030 ;	test_core.c: 54: CHECK(23, g32 == 0x12345778);
      000D40 06 F4                 2031 	lda	_g32
      000D42 78 A4                 2032 	cpi	#0x78
      000D44 09 A8                 2033 	bnz	00604$
      000D46 07 F4                 2034 	lda	_g32+1
      000D48 57 A4                 2035 	cpi	#0x57
      000D4A 06 A8                 2036 	bnz	00604$
      000D4C 08 F4                 2037 	lda	_g32+2
      000D4E 34 A4                 2038 	cpi	#0x34
      000D50 03 A8                 2039 	bnz	00604$
      000D52 09 F4                 2040 	lda	_g32+3
      000D54 12 A4                 2041 	cpi	#0x12
      000D56                       2042 00604$:
      000D56 08 A8                 2043 	bnz	00250$
      000D58 84 80                 2044 	ldi	#>(___str_1 + 0)
      000D5A 80 A0                 2045 	push	a
      000D5C 9C 80                 2046 	ldi	#<(___str_1 + 0)
      000D5E 80 A0                 2047 	push	a
      000D60 49 00                 2048 	jal	_puts
      000D62 02 94                 2049 	ads	#2
      000D64 0A B0                 2050 	br	00251$
      000D66                       2051 00250$:
      000D66 84 80                 2052 	ldi	#>(___str_2 + 0)
      000D68 80 A0                 2053 	push	a
      000D6A A0 80                 2054 	ldi	#<(___str_2 + 0)
      000D6C 80 A0                 2055 	push	a
      000D6E 49 00                 2056 	jal	_puts
      000D70 02 94                 2057 	ads	#2
      000D72 80 A1 00 00           2058 	ldx	#_fails
      000D76 00 E4                 2059 	inx	0(ix)
      000D78                       2060 00251$:
      000D78 00 80                 2061 	ldi	#0x00
      000D7A 80 A0                 2062 	push	a
      000D7C 17 80                 2063 	ldi	#0x17
      000D7E 80 A0                 2064 	push	a
      000D80 6A 00                 2065 	jal	_puthex
      000D82 02 94                 2066 	ads	#2
      000D84 0A 80                 2067 	ldi	#0x0a
      000D86 80 A0                 2068 	push	a
      000D88 40 00                 2069 	jal	_putc
      000D8A 01 94                 2070 	ads	#1
                                   2071 ;	test_core.c: 55: g8 |= 0x80; g8 &= 0xf0; g8 ^= 0x11;
      000D8C 03 F4                 2072 	lda	_g8
      000D8E 7F D4                 2073 	andi	#0x7f
      000D90 08 A0                 2074 	ldc	#0
      000D92 80 90                 2075 	adc	#0x80
      000D94 03 FC                 2076 	sta	_g8
      000D96 03 F4                 2077 	lda	_g8
      000D98 01 FA                 2078 	stax	1(sp)
      000D9A F0 D4                 2079 	andi	#0xf0
      000D9C 03 FC                 2080 	sta	_g8
      000D9E 03 F4                 2081 	lda	_g8
      000DA0 80 A0                 2082 	push	a
      000DA2 11 80                 2083 	ldi	#0x11
      000DA4 01 EE                 2084 	swap	1(sp)
      000DA6 01 E2                 2085 	xor	1(sp)
      000DA8 01 94                 2086 	ads	#1
      000DAA 03 FC                 2087 	sta	_g8
                                   2088 ;	test_core.c: 56: CHECK(24, g8 == 0x91);
      000DAC 03 F4                 2089 	lda	_g8
      000DAE 91 A4                 2090 	cpi	#0x91
      000DB0 08 A8                 2091 	bnz	00256$
      000DB2 84 80                 2092 	ldi	#>(___str_1 + 0)
      000DB4 80 A0                 2093 	push	a
      000DB6 9C 80                 2094 	ldi	#<(___str_1 + 0)
      000DB8 80 A0                 2095 	push	a
      000DBA 49 00                 2096 	jal	_puts
      000DBC 02 94                 2097 	ads	#2
      000DBE 0A B0                 2098 	br	00257$
      000DC0                       2099 00256$:
      000DC0 84 80                 2100 	ldi	#>(___str_2 + 0)
      000DC2 80 A0                 2101 	push	a
      000DC4 A0 80                 2102 	ldi	#<(___str_2 + 0)
      000DC6 80 A0                 2103 	push	a
      000DC8 49 00                 2104 	jal	_puts
      000DCA 02 94                 2105 	ads	#2
      000DCC 80 A1 00 00           2106 	ldx	#_fails
      000DD0 00 E4                 2107 	inx	0(ix)
      000DD2                       2108 00257$:
      000DD2 00 80                 2109 	ldi	#0x00
      000DD4 80 A0                 2110 	push	a
      000DD6 18 80                 2111 	ldi	#0x18
      000DD8 80 A0                 2112 	push	a
      000DDA 6A 00                 2113 	jal	_puthex
      000DDC 02 94                 2114 	ads	#2
      000DDE 0A 80                 2115 	ldi	#0x0a
      000DE0 80 A0                 2116 	push	a
      000DE2 40 00                 2117 	jal	_putc
      000DE4 01 94                 2118 	ads	#1
                                   2119 ;	test_core.c: 57: CHECK(25, (g16 / 10) == -246 && (g16 % 10) == -1);
      000DE6 00 80                 2120 	ldi	#0x00
      000DE8 80 A0                 2121 	push	a
      000DEA 0A 80                 2122 	ldi	#0x0a
      000DEC 80 A0                 2123 	push	a
      000DEE 05 F4                 2124 	lda	_g16+1
      000DF0 80 A0                 2125 	push	a
      000DF2 04 F4                 2126 	lda	_g16
      000DF4 80 A0                 2127 	push	a
      000DF6 FE 97                 2128 	ads	#-2
      000DF8 2F 08                 2129 	jal	__divsint
      000DFA 01 F2                 2130 	ldax	1(sp)
      000DFC 0C FA                 2131 	stax	12(sp)
      000DFE 02 F2                 2132 	ldax	2(sp)
      000E00 0D FA                 2133 	stax	13(sp)
      000E02 06 94                 2134 	ads	#6
      000E04 06 F2                 2135 	ldax	6(sp)
      000E06 0A A4                 2136 	cpi	#0x0a
      000E08 03 A8                 2137 	bnz	00608$
      000E0A 07 F2                 2138 	ldax	7(sp)
      000E0C FF A4                 2139 	cpi	#0xff
      000E0E                       2140 00608$:
      000E0E 1D A8                 2141 	bnz	00262$
      000E10 00 80                 2142 	ldi	#0x00
      000E12 80 A0                 2143 	push	a
      000E14 0A 80                 2144 	ldi	#0x0a
      000E16 80 A0                 2145 	push	a
      000E18 05 F4                 2146 	lda	_g16+1
      000E1A 80 A0                 2147 	push	a
      000E1C 04 F4                 2148 	lda	_g16
      000E1E 80 A0                 2149 	push	a
      000E20 FE 97                 2150 	ads	#-2
      000E22 55 07                 2151 	jal	__modsint
      000E24 01 F2                 2152 	ldax	1(sp)
      000E26 0C FA                 2153 	stax	12(sp)
      000E28 02 F2                 2154 	ldax	2(sp)
      000E2A 0D FA                 2155 	stax	13(sp)
      000E2C 06 94                 2156 	ads	#6
      000E2E 06 F2                 2157 	ldax	6(sp)
      000E30 FF A4                 2158 	cpi	#0xff
      000E32 03 A8                 2159 	bnz	00610$
      000E34 07 F2                 2160 	ldax	7(sp)
      000E36 FF A4                 2161 	cpi	#0xff
      000E38                       2162 00610$:
      000E38 08 A8                 2163 	bnz	00262$
      000E3A 84 80                 2164 	ldi	#>(___str_1 + 0)
      000E3C 80 A0                 2165 	push	a
      000E3E 9C 80                 2166 	ldi	#<(___str_1 + 0)
      000E40 80 A0                 2167 	push	a
      000E42 49 00                 2168 	jal	_puts
      000E44 02 94                 2169 	ads	#2
      000E46 0A B0                 2170 	br	00263$
      000E48                       2171 00262$:
      000E48 84 80                 2172 	ldi	#>(___str_2 + 0)
      000E4A 80 A0                 2173 	push	a
      000E4C A0 80                 2174 	ldi	#<(___str_2 + 0)
      000E4E 80 A0                 2175 	push	a
      000E50 49 00                 2176 	jal	_puts
      000E52 02 94                 2177 	ads	#2
      000E54 80 A1 00 00           2178 	ldx	#_fails
      000E58 00 E4                 2179 	inx	0(ix)
      000E5A                       2180 00263$:
      000E5A 00 80                 2181 	ldi	#0x00
      000E5C 80 A0                 2182 	push	a
      000E5E 19 80                 2183 	ldi	#0x19
      000E60 80 A0                 2184 	push	a
      000E62 6A 00                 2185 	jal	_puthex
      000E64 02 94                 2186 	ads	#2
      000E66 0A 80                 2187 	ldi	#0x0a
      000E68 80 A0                 2188 	push	a
      000E6A 40 00                 2189 	jal	_putc
      000E6C 01 94                 2190 	ads	#1
                                   2191 ;	test_core.c: 58: DONE();
      000E6E 00 F4                 2192 	lda	_fails
      000E70 06 B8                 2193 	bz	00272$
      000E72 A6 80                 2194 	ldi	#<(___str_3 + 0)
      000E74 06 FA                 2195 	stax	6(sp)
      000E76 84 80                 2196 	ldi	#>(___str_3 + 0)
      000E78 07 FA                 2197 	stax	7(sp)
      000E7A 05 B0                 2198 	br	00273$
      000E7C                       2199 00272$:
      000E7C B3 80                 2200 	ldi	#<(___str_4 + 0)
      000E7E 06 FA                 2201 	stax	6(sp)
      000E80 84 80                 2202 	ldi	#>(___str_4 + 0)
      000E82 07 FA                 2203 	stax	7(sp)
      000E84                       2204 00273$:
      000E84 07 F2                 2205 	ldax	7(sp)
      000E86 80 A0                 2206 	push	a
      000E88 07 F2                 2207 	ldax	7(sp)
      000E8A 80 A0                 2208 	push	a
      000E8C 49 00                 2209 	jal	_puts
      000E8E 02 94                 2210 	ads	#2
                                   2211 ;	test_core.c: 59: return fails;
      000E90 00 F4                 2212 	lda	_fails
      000E92 06 FA                 2213 	stax	6(sp)
      000E94 00 80                 2214 	ldi	#0x00
      000E96 07 FA                 2215 	stax	7(sp)
      000E98 06 F2                 2216 	ldax	6(sp)
      000E9A 0B FA                 2217 	stax	11(sp)
      000E9C 07 F2                 2218 	ldax	7(sp)
      000E9E 0C FA                 2219 	stax	12(sp)
      000EA0                       2220 00270$:
                                   2221 ;	test_core.c: 60: }
      000EA0 08 94                 2222 	ads	#8
      000EA2 64 A1                 2223 	lra
      000EA4 00 8A                 2224 	ret
                                   2225 	.area CODE (CODE)
                                   2226 	.area CONST (CODE,CDATA)
                                   2227 	.area CONST (CODE,CDATA)
      001204                       2228 ___str_0:
      001204 30 80 00 8A 31 80 00  2229 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      001244 00 80 00 8A           2230 	.db 0x00
                                   2231 	.area CODE (CODE)
                                   2232 	.area CONST (CODE,CDATA)
      001248                       2233 _ctab:
      001248 0A 80 00 8A           2234 	.db #0x0a	; 10
      00124C 14 80 00 8A           2235 	.db #0x14	; 20
      001250 1E 80 00 8A           2236 	.db #0x1e	; 30
      001254 28 80 00 8A           2237 	.db #0x28	; 40
                                   2238 	.area CODE (CODE)
                                   2239 	.area CONST (CODE,CDATA)
      001258                       2240 _citab:
      001258 FF 80 00 8A FF 80 00  2241 	.dw #0xffff
             8A
      001260 2C 80 00 8A 01 80 00  2242 	.dw #0x012c
             8A
      001268 00 80 00 8A 7D 80 00  2243 	.dw #0x7d00
             8A
                                   2244 	.area CODE (CODE)
                                   2245 	.area CONST (CODE,CDATA)
      001270                       2246 ___str_1:
      001270 6F 80 00 8A 6B 80 00  2247 	.ascii "ok "
             8A 20 80 00 8A
      00127C 00 80 00 8A           2248 	.db 0x00
                                   2249 	.area CODE (CODE)
                                   2250 	.area CONST (CODE,CDATA)
      001280                       2251 ___str_2:
      001280 46 80 00 8A 41 80 00  2252 	.ascii "FAIL "
             8A 49 80 00 8A 4C 80
             00 8A 20 80 00 8A
      001294 00 80 00 8A           2253 	.db 0x00
                                   2254 	.area CODE (CODE)
                                   2255 	.area CONST (CODE,CDATA)
      001298                       2256 ___str_3:
      001298 53 80 00 8A 4F 80 00  2257 	.ascii "SOME FAILED"
             8A 4D 80 00 8A 45 80
             00 8A 20 80 00 8A 46
             80 00 8A 41 80 00 8A
             49 80 00 8A 4C 80 00
             8A 45 80 00 8A 44 80
             00 8A
      0012C4 0A 80 00 8A           2258 	.db 0x0a
      0012C8 00 80 00 8A           2259 	.db 0x00
                                   2260 	.area CODE (CODE)
                                   2261 	.area CONST (CODE,CDATA)
      0012CC                       2262 ___str_4:
      0012CC 41 80 00 8A 4C 80 00  2263 	.ascii "ALL PASSED"
             8A 4C 80 00 8A 20 80
             00 8A 50 80 00 8A 41
             80 00 8A 53 80 00 8A
             53 80 00 8A 45 80 00
             8A 44 80 00 8A
      0012F4 0A 80 00 8A           2264 	.db 0x0a
      0012F8 00 80 00 8A           2265 	.db 0x00
                                   2266 	.area CODE (CODE)
                                   2267 	.area INITIALIZER (CODE,CDATA)
      0012FC                       2268 __xinit__g8:
      0012FC 07 80 00 8A           2269 	.db #0x07	; 7
      001300                       2270 __xinit__g16:
      001300 2E 80 00 8A FB 80 00  2271 	.dw #0xfb2e
             8A
      001308                       2272 __xinit__g32:
      001308 78 80 00 8A 56 80 00  2273 	.byte #0x78, #0x56, #0x34, #0x12	;  305419896
             8A 34 80 00 8A 12 80
             00 8A
      001318                       2274 __xinit__arr:
      001318 01 80 00 8A           2275 	.db #0x01	; 1
      00131C 02 80 00 8A           2276 	.db #0x02	; 2
      001320 03 80 00 8A           2277 	.db #0x03	; 3
      001324 04 80 00 8A           2278 	.db #0x04	; 4
                                   2279 	.area CABS (ABS,CODE,CDATA)
