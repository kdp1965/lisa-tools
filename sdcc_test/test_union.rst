                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_union
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
                                     17 	.globl _g
                                     18 	.globl _f
                                     19 	.globl _UART_STATUS
                                     20 	.globl _UART_TX
                                     21 ;--------------------------------------------------------
                                     22 ; special function registers
                                     23 ;--------------------------------------------------------
                                     24 	.area RSEG (ABS)
      000000                         25 	.org 0x0000
                           000210    26 _UART_TX	=	0x0210
                           000211    27 _UART_STATUS	=	0x0211
                                     28 ;--------------------------------------------------------
                                     29 ; ram data
                                     30 ;--------------------------------------------------------
                                     31 	.area DATA
      000000                         32 _fails:
      000000                         33 	.ds 1
                                     34 ;--------------------------------------------------------
                                     35 ; ram data
                                     36 ;--------------------------------------------------------
                                     37 	.area INITIALIZED
                                     38 ;--------------------------------------------------------
                                     39 ; overlayable items in ram
                                     40 ;--------------------------------------------------------
                                     41 ;--------------------------------------------------------
                                     42 ; Stack segment in internal ram
                                     43 ;--------------------------------------------------------
                                     44 	.area SSEG
      000001                         45 __start__stack:
      000001                         46 	.ds	1
                                     47 
                                     48 ;--------------------------------------------------------
                                     49 ; absolute external ram data
                                     50 ;--------------------------------------------------------
                                     51 	.area DABS (ABS)
                                     52 ;--------------------------------------------------------
                                     53 ; interrupt vector
                                     54 ;--------------------------------------------------------
                                     55 	.area HOME (CODE)
      000000                         56 __interrupt_vect:
      000000 0C 00                   57 	jal	__sdcc_gsinit_startup
      000002 40 8B                   58 	rets
      000004 40 8B                   59 	rets
      000006 40 8B                   60 	rets
      000008 40 8B                   61 	rets
      00000A 40 8B                   62 	rets
      00000C 40 8B                   63 	rets
      00000E 40 8B                   64 	rets
      000010 40 8B                   65 	rets
      000012 40 8B                   66 	rets
                                     67 ;--------------------------------------------------------
                                     68 ; global & static initialisations
                                     69 ;--------------------------------------------------------
                                     70 	.area HOME (CODE)
                                     71 	.area GSINIT (CODE)
                                     72 	.area GSFINAL (CODE)
                                     73 	.area GSINIT (CODE)
                                     74 	.area GSINIT (CODE)
      000018                         75 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             76 	ldx	#0x007f
      00001C C8 8A                   77 	xchg	sp
      00001E 41 A1                   78 	amode	1
      000020 C2 02                   79 	jal	___sdcc_external_startup
      000022 00 A4                   80 	cpi	#0
      000024 01 A2                   81 	if	ne
      000026 0A 00                   82 	jal	__sdcc_program_startup
      000028 00 80                   83 	ldi	#>l_DATA
      00002A 80 A0                   84 	push	a
      00002C 01 80                   85 	ldi	#<l_DATA
      00002E 80 A0                   86 	push	a
      000030 80 A1 00 00             87 	ldx	#s_DATA
      000034                         88 00001$:
      000034 01 F2                   89 	ldax	1(sp)
      000036 02 DA                   90 	or	2(sp)
      000038 08 B8                   91 	bz	00002$
      00003A 00 80                   92 	ldi	#0
      00003C 00 F8                   93 	stax	0(ix)
      00003E 01 98                   94 	adx	#1
      000040 01 9E                   95 	dcx	1(sp)
      000042 03 A2                   96 	if	c
      000044 02 9E                   97 	dcx	2(sp)
      000046 F7 B7                   98 	br	00001$
      000048                         99 00002$:
      000048 00 80                  100 	ldi	#>s_INITIALIZED
      00004A 80 A0                  101 	push	a
      00004C 01 80                  102 	ldi	#<s_INITIALIZED
      00004E 80 A0                  103 	push	a
      000050 00 80                  104 	ldi	#>l_INITIALIZED
      000052 04 FA                  105 	stax	4(sp)
      000054 00 80                  106 	ldi	#<l_INITIALIZED
      000056 03 FA                  107 	stax	3(sp)
      000058 80 A1 2C 83            108 	ldx	#s_INITIALIZER
      00005C                        109 00003$:
      00005C 03 F2                  110 	ldax	3(sp)
      00005E 04 DA                  111 	or	4(sp)
      000060 0E B8                  112 	bz	00004$
      000062 80 8A                  113 	call	ix
      000064 01 98                  114 	adx	#1
      000066 68 A1                  115 	push	ix
      000068 03 CC                  116 	ldxx	3(sp)
      00006A 00 F8                  117 	stax	0(ix)
      00006C 03 E6                  118 	inx	3(sp)
      00006E 03 A2                  119 	if	c
      000070 04 E6                  120 	inx	4(sp)
      000072 6C A1                  121 	pop	ix
      000074 03 9E                  122 	dcx	3(sp)
      000076 03 A2                  123 	if	c
      000078 04 9E                  124 	dcx	4(sp)
      00007A F1 B7                  125 	br	00003$
      00007C                        126 00004$:
      00007C 04 94                  127 	ads	#4
                                    128 	.area GSFINAL (CODE)
      00007E 0A 00                  129 	jal	__sdcc_program_startup
                                    130 ;--------------------------------------------------------
                                    131 ; Home
                                    132 ;--------------------------------------------------------
                                    133 	.area HOME (CODE)
                                    134 	.area HOME (CODE)
      000014                        135 __sdcc_program_startup:
      000014 C5 01                  136 	jal	_main
      000016                        137 00001$:
      000016 00 B0                  138 	br	00001$
                                    139 ;	return from main will return to caller
                                    140 ;--------------------------------------------------------
                                    141 ; code
                                    142 ;--------------------------------------------------------
                                    143 	.area CODE (CODE)
                                    144 ;	harness.h: 15: static void putc(char c)
                                    145 ;	---------------------------------
                                    146 ;	 Function putc
                                    147 ;	---------------------------------
      000080                        148 _putc:
      000080 FF 97                  149 	ads	#-1
                                    150 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        151 00101$:
      000082 11 F6                  152 	lda	_UART_STATUS
      000084 01 FA                  153 	stax	1(sp)
      000086 02 D4                  154 	andi	#0x02
      000088 FD BF                  155 	bz	00101$
                                    156 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  157 	ldax	2(sp)
      00008C 10 FE                  158 	sta	_UART_TX
      00008E                        159 00104$:
                                    160 ;	harness.h: 20: }
      00008E 01 94                  161 	ads	#1
      000090 00 8A                  162 	ret
                                    163 ;	harness.h: 22: static void puts(const char *s)
                                    164 ;	---------------------------------
                                    165 ;	 Function puts
                                    166 ;	---------------------------------
      000092                        167 _puts:
      000092 60 A1                  168 	sra
      000094 FD 97                  169 	ads	#-3
                                    170 ;	harness.h: 24: while (*s)
      000096 06 F2                  171 	ldax	6(sp)
      000098 02 FA                  172 	stax	2(sp)
      00009A 07 F2                  173 	ldax	7(sp)
      00009C 03 FA                  174 	stax	3(sp)
      00009E                        175 00101$:
      00009E 02 CC                  176 	ldxx	2(sp)
      0000A0 18 A0                  177 	txau
      0000A2 47 A0                  178 	btst	7
      0000A4 04 B8                  179 	bz	00119$
      0000A6 00 F0                  180 	ldax	0(ix)
      0000A8 01 FA                  181 	stax	1(sp)
      0000AA 08 B0                  182 	br	00120$
      0000AC                        183 00119$:
      0000AC 18 A0                  184 	txau
      0000AE 7F D4                  185 	andi	#0x7f
      0000B0 A1 A1                  186 	addaxu
      0000B2 10 A0                  187 	txa
      0000B4 A0 A1                  188 	addax
      0000B6 80 8A                  189 	call	ix
      0000B8 01 FA                  190 	stax	1(sp)
      0000BA                        191 00120$:
      0000BA 01 F2                  192 	ldax	1(sp)
      0000BC 09 B8                  193 	bz	00104$
                                    194 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  195 	inx	2(sp)
      0000C0 03 A2                  196 	if	c
      0000C2 03 E6                  197 	inx.p	3(sp)
      0000C4 01 F2                  198 	ldax	1(sp)
      0000C6 80 A0                  199 	push	a
      0000C8 40 00                  200 	jal	_putc
      0000CA 01 94                  201 	ads	#1
      0000CC E9 B7                  202 	br	00101$
      0000CE                        203 00104$:
                                    204 ;	harness.h: 26: }
      0000CE 03 94                  205 	ads	#3
      0000D0 64 A1                  206 	lra
      0000D2 00 8A                  207 	ret
                                    208 ;	harness.h: 28: static void puthex(unsigned int v)
                                    209 ;	---------------------------------
                                    210 ;	 Function puthex
                                    211 ;	---------------------------------
      0000D4                        212 _puthex:
      0000D4 60 A1                  213 	sra
      0000D6 FD 97                  214 	ads	#-3
                                    215 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    216 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  217 	ldax	7(sp)
      0000DA 02 FA                  218 	stax	2(sp)
      0000DC 00 80                  219 	ldi	#0x00
      0000DE 03 FA                  220 	stax	3(sp)
      0000E0 08 A0                  221 	ldc	#0
      0000E2 04 A0                  222 	shr
      0000E4 03 FA                  223 	stax	3(sp)
      0000E6 02 F2                  224 	ldax	2(sp)
      0000E8 04 A0                  225 	shr
      0000EA 02 FA                  226 	stax	2(sp)
      0000EC 03 F2                  227 	ldax	3(sp)
      0000EE 08 A0                  228 	ldc	#0
      0000F0 04 A0                  229 	shr
      0000F2 03 FA                  230 	stax	3(sp)
      0000F4 02 F2                  231 	ldax	2(sp)
      0000F6 04 A0                  232 	shr
      0000F8 02 FA                  233 	stax	2(sp)
      0000FA 03 F2                  234 	ldax	3(sp)
      0000FC 08 A0                  235 	ldc	#0
      0000FE 04 A0                  236 	shr
      000100 03 FA                  237 	stax	3(sp)
      000102 02 F2                  238 	ldax	2(sp)
      000104 04 A0                  239 	shr
      000106 02 FA                  240 	stax	2(sp)
      000108 03 F2                  241 	ldax	3(sp)
      00010A 08 A0                  242 	ldc	#0
      00010C 04 A0                  243 	shr
      00010E 03 FA                  244 	stax	3(sp)
      000110 02 F2                  245 	ldax	2(sp)
      000112 04 A0                  246 	shr
      000114 02 FA                  247 	stax	2(sp)
      000116 0F D4                  248 	andi	#0x0f
      000118 02 FA                  249 	stax	2(sp)
      00011A 00 80                  250 	ldi	#0x00
      00011C 03 FA                  251 	stax	3(sp)
      00011E 02 F2                  252 	ldax	2(sp)
      000120 08 A0                  253 	ldc	#0
      000122 62 90                  254 	adc	#<(___str_0 + 0)
      000124 02 FA                  255 	stax	2(sp)
      000126 03 F2                  256 	ldax	3(sp)
      000128 81 90                  257 	adc	#>(___str_0 + 0)
      00012A 03 FA                  258 	stax	3(sp)
      00012C 02 CC                  259 	ldxx	2(sp)
      00012E 18 A0                  260 	txau
      000130 47 A0                  261 	btst	7
      000132 04 B8                  262 	bz	00103$
      000134 00 F0                  263 	ldax	0(ix)
      000136 01 FA                  264 	stax	1(sp)
      000138 08 B0                  265 	br	00104$
      00013A                        266 00103$:
      00013A 18 A0                  267 	txau
      00013C 7F D4                  268 	andi	#0x7f
      00013E A1 A1                  269 	addaxu
      000140 10 A0                  270 	txa
      000142 A0 A1                  271 	addax
      000144 80 8A                  272 	call	ix
      000146 01 FA                  273 	stax	1(sp)
      000148                        274 00104$:
      000148 01 F2                  275 	ldax	1(sp)
      00014A 80 A0                  276 	push	a
      00014C 40 00                  277 	jal	_putc
      00014E 01 94                  278 	ads	#1
                                    279 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  280 	ldax	7(sp)
      000152 02 FA                  281 	stax	2(sp)
      000154 00 80                  282 	ldi	#0x00
      000156 03 FA                  283 	stax	3(sp)
      000158 02 F2                  284 	ldax	2(sp)
      00015A 0F D4                  285 	andi	#0x0f
      00015C 02 FA                  286 	stax	2(sp)
      00015E 00 80                  287 	ldi	#0x00
      000160 03 FA                  288 	stax	3(sp)
      000162 02 F2                  289 	ldax	2(sp)
      000164 08 A0                  290 	ldc	#0
      000166 62 90                  291 	adc	#<(___str_0 + 0)
      000168 02 FA                  292 	stax	2(sp)
      00016A 03 F2                  293 	ldax	3(sp)
      00016C 81 90                  294 	adc	#>(___str_0 + 0)
      00016E 03 FA                  295 	stax	3(sp)
      000170 02 CC                  296 	ldxx	2(sp)
      000172 18 A0                  297 	txau
      000174 47 A0                  298 	btst	7
      000176 04 B8                  299 	bz	00105$
      000178 00 F0                  300 	ldax	0(ix)
      00017A 01 FA                  301 	stax	1(sp)
      00017C 08 B0                  302 	br	00106$
      00017E                        303 00105$:
      00017E 18 A0                  304 	txau
      000180 7F D4                  305 	andi	#0x7f
      000182 A1 A1                  306 	addaxu
      000184 10 A0                  307 	txa
      000186 A0 A1                  308 	addax
      000188 80 8A                  309 	call	ix
      00018A 01 FA                  310 	stax	1(sp)
      00018C                        311 00106$:
      00018C 01 F2                  312 	ldax	1(sp)
      00018E 80 A0                  313 	push	a
      000190 40 00                  314 	jal	_putc
      000192 01 94                  315 	ads	#1
                                    316 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  317 	ldax	6(sp)
      000196 02 FA                  318 	stax	2(sp)
      000198 07 F2                  319 	ldax	7(sp)
      00019A 03 FA                  320 	stax	3(sp)
      00019C 08 A0                  321 	ldc	#0
      00019E 04 A0                  322 	shr
      0001A0 03 FA                  323 	stax	3(sp)
      0001A2 02 F2                  324 	ldax	2(sp)
      0001A4 04 A0                  325 	shr
      0001A6 02 FA                  326 	stax	2(sp)
      0001A8 03 F2                  327 	ldax	3(sp)
      0001AA 08 A0                  328 	ldc	#0
      0001AC 04 A0                  329 	shr
      0001AE 03 FA                  330 	stax	3(sp)
      0001B0 02 F2                  331 	ldax	2(sp)
      0001B2 04 A0                  332 	shr
      0001B4 02 FA                  333 	stax	2(sp)
      0001B6 03 F2                  334 	ldax	3(sp)
      0001B8 08 A0                  335 	ldc	#0
      0001BA 04 A0                  336 	shr
      0001BC 03 FA                  337 	stax	3(sp)
      0001BE 02 F2                  338 	ldax	2(sp)
      0001C0 04 A0                  339 	shr
      0001C2 02 FA                  340 	stax	2(sp)
      0001C4 03 F2                  341 	ldax	3(sp)
      0001C6 08 A0                  342 	ldc	#0
      0001C8 04 A0                  343 	shr
      0001CA 03 FA                  344 	stax	3(sp)
      0001CC 02 F2                  345 	ldax	2(sp)
      0001CE 04 A0                  346 	shr
      0001D0 02 FA                  347 	stax	2(sp)
      0001D2 0F D4                  348 	andi	#0x0f
      0001D4 02 FA                  349 	stax	2(sp)
      0001D6 00 80                  350 	ldi	#0x00
      0001D8 03 FA                  351 	stax	3(sp)
      0001DA 02 F2                  352 	ldax	2(sp)
      0001DC 08 A0                  353 	ldc	#0
      0001DE 62 90                  354 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  355 	stax	2(sp)
      0001E2 03 F2                  356 	ldax	3(sp)
      0001E4 81 90                  357 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  358 	stax	3(sp)
      0001E8 02 CC                  359 	ldxx	2(sp)
      0001EA 18 A0                  360 	txau
      0001EC 47 A0                  361 	btst	7
      0001EE 04 B8                  362 	bz	00107$
      0001F0 00 F0                  363 	ldax	0(ix)
      0001F2 01 FA                  364 	stax	1(sp)
      0001F4 08 B0                  365 	br	00108$
      0001F6                        366 00107$:
      0001F6 18 A0                  367 	txau
      0001F8 7F D4                  368 	andi	#0x7f
      0001FA A1 A1                  369 	addaxu
      0001FC 10 A0                  370 	txa
      0001FE A0 A1                  371 	addax
      000200 80 8A                  372 	call	ix
      000202 01 FA                  373 	stax	1(sp)
      000204                        374 00108$:
      000204 01 F2                  375 	ldax	1(sp)
      000206 80 A0                  376 	push	a
      000208 40 00                  377 	jal	_putc
      00020A 01 94                  378 	ads	#1
                                    379 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  380 	ldax	6(sp)
      00020E 0F D4                  381 	andi	#0x0f
      000210 02 FA                  382 	stax	2(sp)
      000212 00 80                  383 	ldi	#0x00
      000214 03 FA                  384 	stax	3(sp)
      000216 02 F2                  385 	ldax	2(sp)
      000218 08 A0                  386 	ldc	#0
      00021A 62 90                  387 	adc	#<(___str_0 + 0)
      00021C 02 FA                  388 	stax	2(sp)
      00021E 03 F2                  389 	ldax	3(sp)
      000220 81 90                  390 	adc	#>(___str_0 + 0)
      000222 03 FA                  391 	stax	3(sp)
      000224 02 CC                  392 	ldxx	2(sp)
      000226 18 A0                  393 	txau
      000228 47 A0                  394 	btst	7
      00022A 04 B8                  395 	bz	00109$
      00022C 00 F0                  396 	ldax	0(ix)
      00022E 01 FA                  397 	stax	1(sp)
      000230 08 B0                  398 	br	00110$
      000232                        399 00109$:
      000232 18 A0                  400 	txau
      000234 7F D4                  401 	andi	#0x7f
      000236 A1 A1                  402 	addaxu
      000238 10 A0                  403 	txa
      00023A A0 A1                  404 	addax
      00023C 80 8A                  405 	call	ix
      00023E 01 FA                  406 	stax	1(sp)
      000240                        407 00110$:
      000240 01 F2                  408 	ldax	1(sp)
      000242 80 A0                  409 	push	a
      000244 40 00                  410 	jal	_putc
      000246 01 94                  411 	ads	#1
      000248                        412 00101$:
                                    413 ;	harness.h: 35: }
      000248 03 94                  414 	ads	#3
      00024A 64 A1                  415 	lra
      00024C 00 8A                  416 	ret
                                    417 ;	test_union.c: 3: long f(long v) {
                                    418 ;	---------------------------------
                                    419 ;	 Function f
                                    420 ;	---------------------------------
      00024E                        421 _f:
      00024E F7 97                  422 	ads	#-9
                                    423 ;	test_union.c: 5: value.l = v;
      000250 CC 8A                  424 	spix
      000252 01 98                  425 	adx	#1
      000254 0E F2                  426 	ldax	14(sp)
      000256 00 F8                  427 	stax	0(ix)
      000258 0F F2                  428 	ldax	15(sp)
      00025A 01 F8                  429 	stax	1(ix)
      00025C 10 F2                  430 	ldax	16(sp)
      00025E 02 F8                  431 	stax	2(ix)
      000260 11 F2                  432 	ldax	17(sp)
      000262 03 F8                  433 	stax	3(ix)
                                    434 ;	test_union.c: 6: if (value.l < 0) value.l = -value.l;
      000264 00 F0                  435 	ldax	0(ix)
      000266 06 FA                  436 	stax	6(sp)
      000268 01 F0                  437 	ldax	1(ix)
      00026A 07 FA                  438 	stax	7(sp)
      00026C 02 F0                  439 	ldax	2(ix)
      00026E 08 FA                  440 	stax	8(sp)
      000270 03 F0                  441 	ldax	3(ix)
      000272 09 FA                  442 	stax	9(sp)
      000274 00 80                  443 	ldi	#0x00
      000276 09 EA                  444 	cmp	9(sp)
      000278 24 A2                  445 	if	sgt
      00027A 14 B0                  446 	br.p	00111$
      00027C 25 A2                  447 	if	slt
      00027E 11 B0                  448 	br.p	00112$
      000280 00 80                  449 	ldi	#0x00
      000282 08 EA                  450 	cmp	8(sp)
      000284 04 A2                  451 	if	gt
      000286 0E B0                  452 	br.p	00111$
      000288 05 A2                  453 	if	lt
      00028A 0B B0                  454 	br.p	00112$
      00028C 00 80                  455 	ldi	#0x00
      00028E 07 EA                  456 	cmp	7(sp)
      000290 04 A2                  457 	if	gt
      000292 08 B0                  458 	br.p	00111$
      000294 05 A2                  459 	if	lt
      000296 05 B0                  460 	br.p	00112$
      000298 00 80                  461 	ldi	#0x00
      00029A 06 EA                  462 	cmp	6(sp)
      00029C 04 A2                  463 	if	gt
      00029E 02 B0                  464 	br.p	00111$
      0002A0                        465 00112$:
      0002A0 25 B0                  466 	br	00102$
      0002A2                        467 00111$:
      0002A2 CC 8A                  468 	spix
      0002A4 01 98                  469 	adx	#1
      0002A6 00 F0                  470 	ldax	0(ix)
      0002A8 06 FA                  471 	stax	6(sp)
      0002AA 01 F0                  472 	ldax	1(ix)
      0002AC 07 FA                  473 	stax	7(sp)
      0002AE 02 F0                  474 	ldax	2(ix)
      0002B0 08 FA                  475 	stax	8(sp)
      0002B2 03 F0                  476 	ldax	3(ix)
      0002B4 09 FA                  477 	stax	9(sp)
      0002B6 00 80                  478 	ldi	#0x00
      0002B8 06 CA                  479 	sub	6(sp)
      0002BA 06 FA                  480 	stax	6(sp)
      0002BC 78 A1                  481 	savec
      0002BE 00 80                  482 	ldi	#0x00
      0002C0 7C A1                  483 	restc
      0002C2 07 CA                  484 	sub	7(sp)
      0002C4 07 FA                  485 	stax	7(sp)
      0002C6 78 A1                  486 	savec
      0002C8 00 80                  487 	ldi	#0x00
      0002CA 7C A1                  488 	restc
      0002CC 08 CA                  489 	sub	8(sp)
      0002CE 08 FA                  490 	stax	8(sp)
      0002D0 78 A1                  491 	savec
      0002D2 00 80                  492 	ldi	#0x00
      0002D4 7C A1                  493 	restc
      0002D6 09 CA                  494 	sub	9(sp)
      0002D8 09 FA                  495 	stax	9(sp)
      0002DA 06 F2                  496 	ldax	6(sp)
      0002DC 00 F8                  497 	stax	0(ix)
      0002DE 07 F2                  498 	ldax	7(sp)
      0002E0 01 F8                  499 	stax	1(ix)
      0002E2 08 F2                  500 	ldax	8(sp)
      0002E4 02 F8                  501 	stax	2(ix)
      0002E6 09 F2                  502 	ldax	9(sp)
      0002E8 03 F8                  503 	stax	3(ix)
      0002EA                        504 00102$:
                                    505 ;	test_union.c: 7: return value.l;
      0002EA CC 8A                  506 	spix
      0002EC 01 98                  507 	adx	#1
      0002EE 00 F0                  508 	ldax	0(ix)
      0002F0 06 FA                  509 	stax	6(sp)
      0002F2 01 F0                  510 	ldax	1(ix)
      0002F4 07 FA                  511 	stax	7(sp)
      0002F6 02 F0                  512 	ldax	2(ix)
      0002F8 08 FA                  513 	stax	8(sp)
      0002FA 03 F0                  514 	ldax	3(ix)
      0002FC 09 FA                  515 	stax	9(sp)
      0002FE 06 F2                  516 	ldax	6(sp)
      000300 0A FA                  517 	stax	10(sp)
      000302 07 F2                  518 	ldax	7(sp)
      000304 0B FA                  519 	stax	11(sp)
      000306 08 F2                  520 	ldax	8(sp)
      000308 0C FA                  521 	stax	12(sp)
      00030A 09 F2                  522 	ldax	9(sp)
      00030C 0D FA                  523 	stax	13(sp)
      00030E                        524 00103$:
                                    525 ;	test_union.c: 8: }
      00030E 09 94                  526 	ads	#9
      000310 00 8A                  527 	ret
                                    528 ;	test_union.c: 9: long g(long v) { long x = v; if (x < 0) x = -x; return x; }
                                    529 ;	---------------------------------
                                    530 ;	 Function g
                                    531 ;	---------------------------------
      000312                        532 _g:
      000312 FC 97                  533 	ads	#-4
      000314 09 F2                  534 	ldax	9(sp)
      000316 01 FA                  535 	stax	1(sp)
      000318 0A F2                  536 	ldax	10(sp)
      00031A 02 FA                  537 	stax	2(sp)
      00031C 0B F2                  538 	ldax	11(sp)
      00031E 03 FA                  539 	stax	3(sp)
      000320 0C F2                  540 	ldax	12(sp)
      000322 04 FA                  541 	stax	4(sp)
      000324 00 80                  542 	ldi	#0x00
      000326 04 EA                  543 	cmp	4(sp)
      000328 24 A2                  544 	if	sgt
      00032A 14 B0                  545 	br.p	00111$
      00032C 25 A2                  546 	if	slt
      00032E 11 B0                  547 	br.p	00112$
      000330 00 80                  548 	ldi	#0x00
      000332 03 EA                  549 	cmp	3(sp)
      000334 04 A2                  550 	if	gt
      000336 0E B0                  551 	br.p	00111$
      000338 05 A2                  552 	if	lt
      00033A 0B B0                  553 	br.p	00112$
      00033C 00 80                  554 	ldi	#0x00
      00033E 02 EA                  555 	cmp	2(sp)
      000340 04 A2                  556 	if	gt
      000342 08 B0                  557 	br.p	00111$
      000344 05 A2                  558 	if	lt
      000346 05 B0                  559 	br.p	00112$
      000348 00 80                  560 	ldi	#0x00
      00034A 01 EA                  561 	cmp	1(sp)
      00034C 04 A2                  562 	if	gt
      00034E 02 B0                  563 	br.p	00111$
      000350                        564 00112$:
      000350 13 B0                  565 	br	00102$
      000352                        566 00111$:
      000352 00 80                  567 	ldi	#0x00
      000354 01 CA                  568 	sub	1(sp)
      000356 01 FA                  569 	stax	1(sp)
      000358 78 A1                  570 	savec
      00035A 00 80                  571 	ldi	#0x00
      00035C 7C A1                  572 	restc
      00035E 02 CA                  573 	sub	2(sp)
      000360 02 FA                  574 	stax	2(sp)
      000362 78 A1                  575 	savec
      000364 00 80                  576 	ldi	#0x00
      000366 7C A1                  577 	restc
      000368 03 CA                  578 	sub	3(sp)
      00036A 03 FA                  579 	stax	3(sp)
      00036C 78 A1                  580 	savec
      00036E 00 80                  581 	ldi	#0x00
      000370 7C A1                  582 	restc
      000372 04 CA                  583 	sub	4(sp)
      000374 04 FA                  584 	stax	4(sp)
      000376                        585 00102$:
      000376 01 F2                  586 	ldax	1(sp)
      000378 05 FA                  587 	stax	5(sp)
      00037A 02 F2                  588 	ldax	2(sp)
      00037C 06 FA                  589 	stax	6(sp)
      00037E 03 F2                  590 	ldax	3(sp)
      000380 07 FA                  591 	stax	7(sp)
      000382 04 F2                  592 	ldax	4(sp)
      000384 08 FA                  593 	stax	8(sp)
      000386                        594 00103$:
      000386 04 94                  595 	ads	#4
      000388 00 8A                  596 	ret
                                    597 ;	test_union.c: 10: int main(void) {
                                    598 ;	---------------------------------
                                    599 ;	 Function main
                                    600 ;	---------------------------------
      00038A                        601 _main:
      00038A 60 A1                  602 	sra
      00038C FA 97                  603 	ads	#-6
                                    604 ;	test_union.c: 11: CHECK(1, f(-42) == 42);
      00038E FF 80                  605 	ldi	#0xff
      000390 80 A0                  606 	push	a
      000392 FF 80                  607 	ldi	#0xff
      000394 80 A0                  608 	push	a
      000396 FF 80                  609 	ldi	#0xff
      000398 80 A0                  610 	push	a
      00039A D6 80                  611 	ldi	#0xd6
      00039C 80 A0                  612 	push	a
      00039E FC 97                  613 	ads	#-4
      0003A0 27 01                  614 	jal	_f
      0003A2 01 F2                  615 	ldax	1(sp)
      0003A4 0B FA                  616 	stax	11(sp)
      0003A6 02 F2                  617 	ldax	2(sp)
      0003A8 0C FA                  618 	stax	12(sp)
      0003AA 03 F2                  619 	ldax	3(sp)
      0003AC 0D FA                  620 	stax	13(sp)
      0003AE 04 F2                  621 	ldax	4(sp)
      0003B0 0E FA                  622 	stax	14(sp)
      0003B2 08 94                  623 	ads	#8
      0003B4 03 F2                  624 	ldax	3(sp)
      0003B6 2A A4                  625 	cpi	#0x2a
      0003B8 09 A8                  626 	bnz	00159$
      0003BA 04 F2                  627 	ldax	4(sp)
      0003BC 00 A4                  628 	cpi	#0x00
      0003BE 06 A8                  629 	bnz	00159$
      0003C0 05 F2                  630 	ldax	5(sp)
      0003C2 00 A4                  631 	cpi	#0x00
      0003C4 03 A8                  632 	bnz	00159$
      0003C6 06 F2                  633 	ldax	6(sp)
      0003C8 00 A4                  634 	cpi	#0x00
      0003CA                        635 00159$:
      0003CA 08 A8                  636 	bnz	00102$
      0003CC 81 80                  637 	ldi	#>(___str_1 + 0)
      0003CE 80 A0                  638 	push	a
      0003D0 73 80                  639 	ldi	#<(___str_1 + 0)
      0003D2 80 A0                  640 	push	a
      0003D4 49 00                  641 	jal	_puts
      0003D6 02 94                  642 	ads	#2
      0003D8 0A B0                  643 	br	00103$
      0003DA                        644 00102$:
      0003DA 81 80                  645 	ldi	#>(___str_2 + 0)
      0003DC 80 A0                  646 	push	a
      0003DE 77 80                  647 	ldi	#<(___str_2 + 0)
      0003E0 80 A0                  648 	push	a
      0003E2 49 00                  649 	jal	_puts
      0003E4 02 94                  650 	ads	#2
      0003E6 80 A1 00 00            651 	ldx	#_fails
      0003EA 00 E4                  652 	inx	0(ix)
      0003EC                        653 00103$:
      0003EC 00 80                  654 	ldi	#0x00
      0003EE 80 A0                  655 	push	a
      0003F0 01 80                  656 	ldi	#0x01
      0003F2 80 A0                  657 	push	a
      0003F4 6A 00                  658 	jal	_puthex
      0003F6 02 94                  659 	ads	#2
      0003F8 0A 80                  660 	ldi	#0x0a
      0003FA 80 A0                  661 	push	a
      0003FC 40 00                  662 	jal	_putc
      0003FE 01 94                  663 	ads	#1
                                    664 ;	test_union.c: 12: CHECK(2, f(-123456) == 123456);
      000400 FF 80                  665 	ldi	#0xff
      000402 80 A0                  666 	push	a
      000404 FE 80                  667 	ldi	#0xfe
      000406 80 A0                  668 	push	a
      000408 1D 80                  669 	ldi	#0x1d
      00040A 80 A0                  670 	push	a
      00040C C0 80                  671 	ldi	#0xc0
      00040E 80 A0                  672 	push	a
      000410 FC 97                  673 	ads	#-4
      000412 27 01                  674 	jal	_f
      000414 01 F2                  675 	ldax	1(sp)
      000416 0B FA                  676 	stax	11(sp)
      000418 02 F2                  677 	ldax	2(sp)
      00041A 0C FA                  678 	stax	12(sp)
      00041C 03 F2                  679 	ldax	3(sp)
      00041E 0D FA                  680 	stax	13(sp)
      000420 04 F2                  681 	ldax	4(sp)
      000422 0E FA                  682 	stax	14(sp)
      000424 08 94                  683 	ads	#8
      000426 03 F2                  684 	ldax	3(sp)
      000428 40 A4                  685 	cpi	#0x40
      00042A 09 A8                  686 	bnz	00161$
      00042C 04 F2                  687 	ldax	4(sp)
      00042E E2 A4                  688 	cpi	#0xe2
      000430 06 A8                  689 	bnz	00161$
      000432 05 F2                  690 	ldax	5(sp)
      000434 01 A4                  691 	cpi	#0x01
      000436 03 A8                  692 	bnz	00161$
      000438 06 F2                  693 	ldax	6(sp)
      00043A 00 A4                  694 	cpi	#0x00
      00043C                        695 00161$:
      00043C 08 A8                  696 	bnz	00108$
      00043E 81 80                  697 	ldi	#>(___str_1 + 0)
      000440 80 A0                  698 	push	a
      000442 73 80                  699 	ldi	#<(___str_1 + 0)
      000444 80 A0                  700 	push	a
      000446 49 00                  701 	jal	_puts
      000448 02 94                  702 	ads	#2
      00044A 0A B0                  703 	br	00109$
      00044C                        704 00108$:
      00044C 81 80                  705 	ldi	#>(___str_2 + 0)
      00044E 80 A0                  706 	push	a
      000450 77 80                  707 	ldi	#<(___str_2 + 0)
      000452 80 A0                  708 	push	a
      000454 49 00                  709 	jal	_puts
      000456 02 94                  710 	ads	#2
      000458 80 A1 00 00            711 	ldx	#_fails
      00045C 00 E4                  712 	inx	0(ix)
      00045E                        713 00109$:
      00045E 00 80                  714 	ldi	#0x00
      000460 80 A0                  715 	push	a
      000462 02 80                  716 	ldi	#0x02
      000464 80 A0                  717 	push	a
      000466 6A 00                  718 	jal	_puthex
      000468 02 94                  719 	ads	#2
      00046A 0A 80                  720 	ldi	#0x0a
      00046C 80 A0                  721 	push	a
      00046E 40 00                  722 	jal	_putc
      000470 01 94                  723 	ads	#1
                                    724 ;	test_union.c: 13: CHECK(3, g(-42) == 42);
      000472 FF 80                  725 	ldi	#0xff
      000474 80 A0                  726 	push	a
      000476 FF 80                  727 	ldi	#0xff
      000478 80 A0                  728 	push	a
      00047A FF 80                  729 	ldi	#0xff
      00047C 80 A0                  730 	push	a
      00047E D6 80                  731 	ldi	#0xd6
      000480 80 A0                  732 	push	a
      000482 FC 97                  733 	ads	#-4
      000484 89 01                  734 	jal	_g
      000486 01 F2                  735 	ldax	1(sp)
      000488 0B FA                  736 	stax	11(sp)
      00048A 02 F2                  737 	ldax	2(sp)
      00048C 0C FA                  738 	stax	12(sp)
      00048E 03 F2                  739 	ldax	3(sp)
      000490 0D FA                  740 	stax	13(sp)
      000492 04 F2                  741 	ldax	4(sp)
      000494 0E FA                  742 	stax	14(sp)
      000496 08 94                  743 	ads	#8
      000498 03 F2                  744 	ldax	3(sp)
      00049A 2A A4                  745 	cpi	#0x2a
      00049C 09 A8                  746 	bnz	00163$
      00049E 04 F2                  747 	ldax	4(sp)
      0004A0 00 A4                  748 	cpi	#0x00
      0004A2 06 A8                  749 	bnz	00163$
      0004A4 05 F2                  750 	ldax	5(sp)
      0004A6 00 A4                  751 	cpi	#0x00
      0004A8 03 A8                  752 	bnz	00163$
      0004AA 06 F2                  753 	ldax	6(sp)
      0004AC 00 A4                  754 	cpi	#0x00
      0004AE                        755 00163$:
      0004AE 08 A8                  756 	bnz	00114$
      0004B0 81 80                  757 	ldi	#>(___str_1 + 0)
      0004B2 80 A0                  758 	push	a
      0004B4 73 80                  759 	ldi	#<(___str_1 + 0)
      0004B6 80 A0                  760 	push	a
      0004B8 49 00                  761 	jal	_puts
      0004BA 02 94                  762 	ads	#2
      0004BC 0A B0                  763 	br	00115$
      0004BE                        764 00114$:
      0004BE 81 80                  765 	ldi	#>(___str_2 + 0)
      0004C0 80 A0                  766 	push	a
      0004C2 77 80                  767 	ldi	#<(___str_2 + 0)
      0004C4 80 A0                  768 	push	a
      0004C6 49 00                  769 	jal	_puts
      0004C8 02 94                  770 	ads	#2
      0004CA 80 A1 00 00            771 	ldx	#_fails
      0004CE 00 E4                  772 	inx	0(ix)
      0004D0                        773 00115$:
      0004D0 00 80                  774 	ldi	#0x00
      0004D2 80 A0                  775 	push	a
      0004D4 03 80                  776 	ldi	#0x03
      0004D6 80 A0                  777 	push	a
      0004D8 6A 00                  778 	jal	_puthex
      0004DA 02 94                  779 	ads	#2
      0004DC 0A 80                  780 	ldi	#0x0a
      0004DE 80 A0                  781 	push	a
      0004E0 40 00                  782 	jal	_putc
      0004E2 01 94                  783 	ads	#1
                                    784 ;	test_union.c: 14: CHECK(4, f(77) == 77);
      0004E4 00 80                  785 	ldi	#0x00
      0004E6 80 A0                  786 	push	a
      0004E8 00 80                  787 	ldi	#0x00
      0004EA 80 A0                  788 	push	a
      0004EC 00 80                  789 	ldi	#0x00
      0004EE 80 A0                  790 	push	a
      0004F0 4D 80                  791 	ldi	#0x4d
      0004F2 80 A0                  792 	push	a
      0004F4 FC 97                  793 	ads	#-4
      0004F6 27 01                  794 	jal	_f
      0004F8 01 F2                  795 	ldax	1(sp)
      0004FA 0B FA                  796 	stax	11(sp)
      0004FC 02 F2                  797 	ldax	2(sp)
      0004FE 0C FA                  798 	stax	12(sp)
      000500 03 F2                  799 	ldax	3(sp)
      000502 0D FA                  800 	stax	13(sp)
      000504 04 F2                  801 	ldax	4(sp)
      000506 0E FA                  802 	stax	14(sp)
      000508 08 94                  803 	ads	#8
      00050A 03 F2                  804 	ldax	3(sp)
      00050C 4D A4                  805 	cpi	#0x4d
      00050E 09 A8                  806 	bnz	00165$
      000510 04 F2                  807 	ldax	4(sp)
      000512 00 A4                  808 	cpi	#0x00
      000514 06 A8                  809 	bnz	00165$
      000516 05 F2                  810 	ldax	5(sp)
      000518 00 A4                  811 	cpi	#0x00
      00051A 03 A8                  812 	bnz	00165$
      00051C 06 F2                  813 	ldax	6(sp)
      00051E 00 A4                  814 	cpi	#0x00
      000520                        815 00165$:
      000520 08 A8                  816 	bnz	00120$
      000522 81 80                  817 	ldi	#>(___str_1 + 0)
      000524 80 A0                  818 	push	a
      000526 73 80                  819 	ldi	#<(___str_1 + 0)
      000528 80 A0                  820 	push	a
      00052A 49 00                  821 	jal	_puts
      00052C 02 94                  822 	ads	#2
      00052E 0A B0                  823 	br	00121$
      000530                        824 00120$:
      000530 81 80                  825 	ldi	#>(___str_2 + 0)
      000532 80 A0                  826 	push	a
      000534 77 80                  827 	ldi	#<(___str_2 + 0)
      000536 80 A0                  828 	push	a
      000538 49 00                  829 	jal	_puts
      00053A 02 94                  830 	ads	#2
      00053C 80 A1 00 00            831 	ldx	#_fails
      000540 00 E4                  832 	inx	0(ix)
      000542                        833 00121$:
      000542 00 80                  834 	ldi	#0x00
      000544 80 A0                  835 	push	a
      000546 04 80                  836 	ldi	#0x04
      000548 80 A0                  837 	push	a
      00054A 6A 00                  838 	jal	_puthex
      00054C 02 94                  839 	ads	#2
      00054E 0A 80                  840 	ldi	#0x0a
      000550 80 A0                  841 	push	a
      000552 40 00                  842 	jal	_putc
      000554 01 94                  843 	ads	#1
                                    844 ;	test_union.c: 15: DONE();
      000556 00 F4                  845 	lda	_fails
      000558 06 B8                  846 	bz	00127$
      00055A 7D 80                  847 	ldi	#<(___str_3 + 0)
      00055C 01 FA                  848 	stax	1(sp)
      00055E 81 80                  849 	ldi	#>(___str_3 + 0)
      000560 02 FA                  850 	stax	2(sp)
      000562 05 B0                  851 	br	00128$
      000564                        852 00127$:
      000564 8A 80                  853 	ldi	#<(___str_4 + 0)
      000566 01 FA                  854 	stax	1(sp)
      000568 81 80                  855 	ldi	#>(___str_4 + 0)
      00056A 02 FA                  856 	stax	2(sp)
      00056C                        857 00128$:
      00056C 02 F2                  858 	ldax	2(sp)
      00056E 80 A0                  859 	push	a
      000570 02 F2                  860 	ldax	2(sp)
      000572 80 A0                  861 	push	a
      000574 49 00                  862 	jal	_puts
      000576 02 94                  863 	ads	#2
                                    864 ;	test_union.c: 16: return 0;
      000578 00 80                  865 	ldi	#0x00
      00057A 09 FA                  866 	stax	9(sp)
      00057C 0A FA                  867 	stax	10(sp)
      00057E                        868 00125$:
                                    869 ;	test_union.c: 17: }
      00057E 06 94                  870 	ads	#6
      000580 64 A1                  871 	lra
      000582 00 8A                  872 	ret
                                    873 	.area CODE (CODE)
                                    874 	.area CONST (CODE,CDATA)
                                    875 	.area CONST (CODE,CDATA)
      000588                        876 ___str_0:
      000588 30 80 00 8A 31 80 00   877 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      0005C8 00 80 00 8A            878 	.db 0x00
                                    879 	.area CODE (CODE)
                                    880 	.area CONST (CODE,CDATA)
      0005CC                        881 ___str_1:
      0005CC 6F 80 00 8A 6B 80 00   882 	.ascii "ok "
             8A 20 80 00 8A
      0005D8 00 80 00 8A            883 	.db 0x00
                                    884 	.area CODE (CODE)
                                    885 	.area CONST (CODE,CDATA)
      0005DC                        886 ___str_2:
      0005DC 46 80 00 8A 41 80 00   887 	.ascii "FAIL "
             8A 49 80 00 8A 4C 80
             00 8A 20 80 00 8A
      0005F0 00 80 00 8A            888 	.db 0x00
                                    889 	.area CODE (CODE)
                                    890 	.area CONST (CODE,CDATA)
      0005F4                        891 ___str_3:
      0005F4 53 80 00 8A 4F 80 00   892 	.ascii "SOME FAILED"
             8A 4D 80 00 8A 45 80
             00 8A 20 80 00 8A 46
             80 00 8A 41 80 00 8A
             49 80 00 8A 4C 80 00
             8A 45 80 00 8A 44 80
             00 8A
      000620 0A 80 00 8A            893 	.db 0x0a
      000624 00 80 00 8A            894 	.db 0x00
                                    895 	.area CODE (CODE)
                                    896 	.area CONST (CODE,CDATA)
      000628                        897 ___str_4:
      000628 41 80 00 8A 4C 80 00   898 	.ascii "ALL PASSED"
             8A 4C 80 00 8A 20 80
             00 8A 50 80 00 8A 41
             80 00 8A 53 80 00 8A
             53 80 00 8A 45 80 00
             8A 44 80 00 8A
      000650 0A 80 00 8A            899 	.db 0x0a
      000654 00 80 00 8A            900 	.db 0x00
                                    901 	.area CODE (CODE)
                                    902 	.area INITIALIZER (CODE,CDATA)
                                    903 	.area CABS (ABS,CODE,CDATA)
