                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_digits
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
                                     17 	.globl _UART_STATUS
                                     18 	.globl _UART_TX
                                     19 	.globl _d3
                                     20 	.globl _d2
                                     21 	.globl _d1
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
      000001                         35 _d1::
      000001                         36 	.ds 1
      000002                         37 _d2::
      000002                         38 	.ds 1
      000003                         39 _d3::
      000003                         40 	.ds 1
                                     41 ;--------------------------------------------------------
                                     42 ; ram data
                                     43 ;--------------------------------------------------------
                                     44 	.area INITIALIZED
                                     45 ;--------------------------------------------------------
                                     46 ; overlayable items in ram
                                     47 ;--------------------------------------------------------
                                     48 ;--------------------------------------------------------
                                     49 ; Stack segment in internal ram
                                     50 ;--------------------------------------------------------
                                     51 	.area SSEG
      000004                         52 __start__stack:
      000004                         53 	.ds	1
                                     54 
                                     55 ;--------------------------------------------------------
                                     56 ; absolute external ram data
                                     57 ;--------------------------------------------------------
                                     58 	.area DABS (ABS)
                                     59 ;--------------------------------------------------------
                                     60 ; interrupt vector
                                     61 ;--------------------------------------------------------
                                     62 	.area HOME (CODE)
      000000                         63 __interrupt_vect:
      000000 0C 00                   64 	jal	__sdcc_gsinit_startup
      000002 40 8B                   65 	rets
      000004 40 8B                   66 	rets
      000006 40 8B                   67 	rets
      000008 40 8B                   68 	rets
      00000A 40 8B                   69 	rets
      00000C 40 8B                   70 	rets
      00000E 40 8B                   71 	rets
      000010 40 8B                   72 	rets
      000012 40 8B                   73 	rets
                                     74 ;--------------------------------------------------------
                                     75 ; global & static initialisations
                                     76 ;--------------------------------------------------------
                                     77 	.area HOME (CODE)
                                     78 	.area GSINIT (CODE)
                                     79 	.area GSFINAL (CODE)
                                     80 	.area GSINIT (CODE)
                                     81 	.area GSINIT (CODE)
      000018                         82 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             83 	ldx	#0x007f
      00001C C8 8A                   84 	xchg	sp
      00001E 41 A1                   85 	amode	1
      000020 21 03                   86 	jal	___sdcc_external_startup
      000022 00 A4                   87 	cpi	#0
      000024 01 A2                   88 	if	ne
      000026 0A 00                   89 	jal	__sdcc_program_startup
      000028 00 80                   90 	ldi	#>l_DATA
      00002A 80 A0                   91 	push	a
      00002C 04 80                   92 	ldi	#<l_DATA
      00002E 80 A0                   93 	push	a
      000030 80 A1 00 00             94 	ldx	#s_DATA
      000034                         95 00001$:
      000034 01 F2                   96 	ldax	1(sp)
      000036 02 DA                   97 	or	2(sp)
      000038 08 B8                   98 	bz	00002$
      00003A 00 80                   99 	ldi	#0
      00003C 00 F8                  100 	stax	0(ix)
      00003E 01 98                  101 	adx	#1
      000040 01 9E                  102 	dcx	1(sp)
      000042 03 A2                  103 	if	c
      000044 02 9E                  104 	dcx	2(sp)
      000046 F7 B7                  105 	br	00001$
      000048                        106 00002$:
      000048 00 80                  107 	ldi	#>s_INITIALIZED
      00004A 80 A0                  108 	push	a
      00004C 04 80                  109 	ldi	#<s_INITIALIZED
      00004E 80 A0                  110 	push	a
      000050 00 80                  111 	ldi	#>l_INITIALIZED
      000052 04 FA                  112 	stax	4(sp)
      000054 00 80                  113 	ldi	#<l_INITIALIZED
      000056 03 FA                  114 	stax	3(sp)
      000058 80 A1 8C 83            115 	ldx	#s_INITIALIZER
      00005C                        116 00003$:
      00005C 03 F2                  117 	ldax	3(sp)
      00005E 04 DA                  118 	or	4(sp)
      000060 0E B8                  119 	bz	00004$
      000062 80 8A                  120 	call	ix
      000064 01 98                  121 	adx	#1
      000066 68 A1                  122 	push	ix
      000068 03 CC                  123 	ldxx	3(sp)
      00006A 00 F8                  124 	stax	0(ix)
      00006C 03 E6                  125 	inx	3(sp)
      00006E 03 A2                  126 	if	c
      000070 04 E6                  127 	inx	4(sp)
      000072 6C A1                  128 	pop	ix
      000074 03 9E                  129 	dcx	3(sp)
      000076 03 A2                  130 	if	c
      000078 04 9E                  131 	dcx	4(sp)
      00007A F1 B7                  132 	br	00003$
      00007C                        133 00004$:
      00007C 04 94                  134 	ads	#4
                                    135 	.area GSFINAL (CODE)
      00007E 0A 00                  136 	jal	__sdcc_program_startup
                                    137 ;--------------------------------------------------------
                                    138 ; Home
                                    139 ;--------------------------------------------------------
                                    140 	.area HOME (CODE)
                                    141 	.area HOME (CODE)
      000014                        142 __sdcc_program_startup:
      000014 0E 02                  143 	jal	_main
      000016                        144 00001$:
      000016 00 B0                  145 	br	00001$
                                    146 ;	return from main will return to caller
                                    147 ;--------------------------------------------------------
                                    148 ; code
                                    149 ;--------------------------------------------------------
                                    150 	.area CODE (CODE)
                                    151 ;	harness.h: 15: static void putc(char c)
                                    152 ;	---------------------------------
                                    153 ;	 Function putc
                                    154 ;	---------------------------------
      000080                        155 _putc:
      000080 FF 97                  156 	ads	#-1
                                    157 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        158 00101$:
      000082 11 F6                  159 	lda	_UART_STATUS
      000084 01 FA                  160 	stax	1(sp)
      000086 02 D4                  161 	andi	#0x02
      000088 FD BF                  162 	bz	00101$
                                    163 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  164 	ldax	2(sp)
      00008C 10 FE                  165 	sta	_UART_TX
      00008E                        166 00104$:
                                    167 ;	harness.h: 20: }
      00008E 01 94                  168 	ads	#1
      000090 00 8A                  169 	ret
                                    170 ;	harness.h: 22: static void puts(const char *s)
                                    171 ;	---------------------------------
                                    172 ;	 Function puts
                                    173 ;	---------------------------------
      000092                        174 _puts:
      000092 60 A1                  175 	sra
      000094 FD 97                  176 	ads	#-3
                                    177 ;	harness.h: 24: while (*s)
      000096 06 F2                  178 	ldax	6(sp)
      000098 02 FA                  179 	stax	2(sp)
      00009A 07 F2                  180 	ldax	7(sp)
      00009C 03 FA                  181 	stax	3(sp)
      00009E                        182 00101$:
      00009E 02 CC                  183 	ldxx	2(sp)
      0000A0 18 A0                  184 	txau
      0000A2 47 A0                  185 	btst	7
      0000A4 04 B8                  186 	bz	00119$
      0000A6 00 F0                  187 	ldax	0(ix)
      0000A8 01 FA                  188 	stax	1(sp)
      0000AA 08 B0                  189 	br	00120$
      0000AC                        190 00119$:
      0000AC 18 A0                  191 	txau
      0000AE 7F D4                  192 	andi	#0x7f
      0000B0 A1 A1                  193 	addaxu
      0000B2 10 A0                  194 	txa
      0000B4 A0 A1                  195 	addax
      0000B6 80 8A                  196 	call	ix
      0000B8 01 FA                  197 	stax	1(sp)
      0000BA                        198 00120$:
      0000BA 01 F2                  199 	ldax	1(sp)
      0000BC 09 B8                  200 	bz	00104$
                                    201 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  202 	inx	2(sp)
      0000C0 03 A2                  203 	if	c
      0000C2 03 E6                  204 	inx.p	3(sp)
      0000C4 01 F2                  205 	ldax	1(sp)
      0000C6 80 A0                  206 	push	a
      0000C8 40 00                  207 	jal	_putc
      0000CA 01 94                  208 	ads	#1
      0000CC E9 B7                  209 	br	00101$
      0000CE                        210 00104$:
                                    211 ;	harness.h: 26: }
      0000CE 03 94                  212 	ads	#3
      0000D0 64 A1                  213 	lra
      0000D2 00 8A                  214 	ret
                                    215 ;	harness.h: 28: static void puthex(unsigned int v)
                                    216 ;	---------------------------------
                                    217 ;	 Function puthex
                                    218 ;	---------------------------------
      0000D4                        219 _puthex:
      0000D4 60 A1                  220 	sra
      0000D6 FD 97                  221 	ads	#-3
                                    222 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    223 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  224 	ldax	7(sp)
      0000DA 02 FA                  225 	stax	2(sp)
      0000DC 00 80                  226 	ldi	#0x00
      0000DE 03 FA                  227 	stax	3(sp)
      0000E0 08 A0                  228 	ldc	#0
      0000E2 04 A0                  229 	shr
      0000E4 03 FA                  230 	stax	3(sp)
      0000E6 02 F2                  231 	ldax	2(sp)
      0000E8 04 A0                  232 	shr
      0000EA 02 FA                  233 	stax	2(sp)
      0000EC 03 F2                  234 	ldax	3(sp)
      0000EE 08 A0                  235 	ldc	#0
      0000F0 04 A0                  236 	shr
      0000F2 03 FA                  237 	stax	3(sp)
      0000F4 02 F2                  238 	ldax	2(sp)
      0000F6 04 A0                  239 	shr
      0000F8 02 FA                  240 	stax	2(sp)
      0000FA 03 F2                  241 	ldax	3(sp)
      0000FC 08 A0                  242 	ldc	#0
      0000FE 04 A0                  243 	shr
      000100 03 FA                  244 	stax	3(sp)
      000102 02 F2                  245 	ldax	2(sp)
      000104 04 A0                  246 	shr
      000106 02 FA                  247 	stax	2(sp)
      000108 03 F2                  248 	ldax	3(sp)
      00010A 08 A0                  249 	ldc	#0
      00010C 04 A0                  250 	shr
      00010E 03 FA                  251 	stax	3(sp)
      000110 02 F2                  252 	ldax	2(sp)
      000112 04 A0                  253 	shr
      000114 02 FA                  254 	stax	2(sp)
      000116 0F D4                  255 	andi	#0x0f
      000118 02 FA                  256 	stax	2(sp)
      00011A 00 80                  257 	ldi	#0x00
      00011C 03 FA                  258 	stax	3(sp)
      00011E 02 F2                  259 	ldax	2(sp)
      000120 08 A0                  260 	ldc	#0
      000122 92 90                  261 	adc	#<(___str_0 + 0)
      000124 02 FA                  262 	stax	2(sp)
      000126 03 F2                  263 	ldax	3(sp)
      000128 81 90                  264 	adc	#>(___str_0 + 0)
      00012A 03 FA                  265 	stax	3(sp)
      00012C 02 CC                  266 	ldxx	2(sp)
      00012E 18 A0                  267 	txau
      000130 47 A0                  268 	btst	7
      000132 04 B8                  269 	bz	00103$
      000134 00 F0                  270 	ldax	0(ix)
      000136 01 FA                  271 	stax	1(sp)
      000138 08 B0                  272 	br	00104$
      00013A                        273 00103$:
      00013A 18 A0                  274 	txau
      00013C 7F D4                  275 	andi	#0x7f
      00013E A1 A1                  276 	addaxu
      000140 10 A0                  277 	txa
      000142 A0 A1                  278 	addax
      000144 80 8A                  279 	call	ix
      000146 01 FA                  280 	stax	1(sp)
      000148                        281 00104$:
      000148 01 F2                  282 	ldax	1(sp)
      00014A 80 A0                  283 	push	a
      00014C 40 00                  284 	jal	_putc
      00014E 01 94                  285 	ads	#1
                                    286 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  287 	ldax	7(sp)
      000152 02 FA                  288 	stax	2(sp)
      000154 00 80                  289 	ldi	#0x00
      000156 03 FA                  290 	stax	3(sp)
      000158 02 F2                  291 	ldax	2(sp)
      00015A 0F D4                  292 	andi	#0x0f
      00015C 02 FA                  293 	stax	2(sp)
      00015E 00 80                  294 	ldi	#0x00
      000160 03 FA                  295 	stax	3(sp)
      000162 02 F2                  296 	ldax	2(sp)
      000164 08 A0                  297 	ldc	#0
      000166 92 90                  298 	adc	#<(___str_0 + 0)
      000168 02 FA                  299 	stax	2(sp)
      00016A 03 F2                  300 	ldax	3(sp)
      00016C 81 90                  301 	adc	#>(___str_0 + 0)
      00016E 03 FA                  302 	stax	3(sp)
      000170 02 CC                  303 	ldxx	2(sp)
      000172 18 A0                  304 	txau
      000174 47 A0                  305 	btst	7
      000176 04 B8                  306 	bz	00105$
      000178 00 F0                  307 	ldax	0(ix)
      00017A 01 FA                  308 	stax	1(sp)
      00017C 08 B0                  309 	br	00106$
      00017E                        310 00105$:
      00017E 18 A0                  311 	txau
      000180 7F D4                  312 	andi	#0x7f
      000182 A1 A1                  313 	addaxu
      000184 10 A0                  314 	txa
      000186 A0 A1                  315 	addax
      000188 80 8A                  316 	call	ix
      00018A 01 FA                  317 	stax	1(sp)
      00018C                        318 00106$:
      00018C 01 F2                  319 	ldax	1(sp)
      00018E 80 A0                  320 	push	a
      000190 40 00                  321 	jal	_putc
      000192 01 94                  322 	ads	#1
                                    323 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  324 	ldax	6(sp)
      000196 02 FA                  325 	stax	2(sp)
      000198 07 F2                  326 	ldax	7(sp)
      00019A 03 FA                  327 	stax	3(sp)
      00019C 08 A0                  328 	ldc	#0
      00019E 04 A0                  329 	shr
      0001A0 03 FA                  330 	stax	3(sp)
      0001A2 02 F2                  331 	ldax	2(sp)
      0001A4 04 A0                  332 	shr
      0001A6 02 FA                  333 	stax	2(sp)
      0001A8 03 F2                  334 	ldax	3(sp)
      0001AA 08 A0                  335 	ldc	#0
      0001AC 04 A0                  336 	shr
      0001AE 03 FA                  337 	stax	3(sp)
      0001B0 02 F2                  338 	ldax	2(sp)
      0001B2 04 A0                  339 	shr
      0001B4 02 FA                  340 	stax	2(sp)
      0001B6 03 F2                  341 	ldax	3(sp)
      0001B8 08 A0                  342 	ldc	#0
      0001BA 04 A0                  343 	shr
      0001BC 03 FA                  344 	stax	3(sp)
      0001BE 02 F2                  345 	ldax	2(sp)
      0001C0 04 A0                  346 	shr
      0001C2 02 FA                  347 	stax	2(sp)
      0001C4 03 F2                  348 	ldax	3(sp)
      0001C6 08 A0                  349 	ldc	#0
      0001C8 04 A0                  350 	shr
      0001CA 03 FA                  351 	stax	3(sp)
      0001CC 02 F2                  352 	ldax	2(sp)
      0001CE 04 A0                  353 	shr
      0001D0 02 FA                  354 	stax	2(sp)
      0001D2 0F D4                  355 	andi	#0x0f
      0001D4 02 FA                  356 	stax	2(sp)
      0001D6 00 80                  357 	ldi	#0x00
      0001D8 03 FA                  358 	stax	3(sp)
      0001DA 02 F2                  359 	ldax	2(sp)
      0001DC 08 A0                  360 	ldc	#0
      0001DE 92 90                  361 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  362 	stax	2(sp)
      0001E2 03 F2                  363 	ldax	3(sp)
      0001E4 81 90                  364 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  365 	stax	3(sp)
      0001E8 02 CC                  366 	ldxx	2(sp)
      0001EA 18 A0                  367 	txau
      0001EC 47 A0                  368 	btst	7
      0001EE 04 B8                  369 	bz	00107$
      0001F0 00 F0                  370 	ldax	0(ix)
      0001F2 01 FA                  371 	stax	1(sp)
      0001F4 08 B0                  372 	br	00108$
      0001F6                        373 00107$:
      0001F6 18 A0                  374 	txau
      0001F8 7F D4                  375 	andi	#0x7f
      0001FA A1 A1                  376 	addaxu
      0001FC 10 A0                  377 	txa
      0001FE A0 A1                  378 	addax
      000200 80 8A                  379 	call	ix
      000202 01 FA                  380 	stax	1(sp)
      000204                        381 00108$:
      000204 01 F2                  382 	ldax	1(sp)
      000206 80 A0                  383 	push	a
      000208 40 00                  384 	jal	_putc
      00020A 01 94                  385 	ads	#1
                                    386 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  387 	ldax	6(sp)
      00020E 0F D4                  388 	andi	#0x0f
      000210 02 FA                  389 	stax	2(sp)
      000212 00 80                  390 	ldi	#0x00
      000214 03 FA                  391 	stax	3(sp)
      000216 02 F2                  392 	ldax	2(sp)
      000218 08 A0                  393 	ldc	#0
      00021A 92 90                  394 	adc	#<(___str_0 + 0)
      00021C 02 FA                  395 	stax	2(sp)
      00021E 03 F2                  396 	ldax	3(sp)
      000220 81 90                  397 	adc	#>(___str_0 + 0)
      000222 03 FA                  398 	stax	3(sp)
      000224 02 CC                  399 	ldxx	2(sp)
      000226 18 A0                  400 	txau
      000228 47 A0                  401 	btst	7
      00022A 04 B8                  402 	bz	00109$
      00022C 00 F0                  403 	ldax	0(ix)
      00022E 01 FA                  404 	stax	1(sp)
      000230 08 B0                  405 	br	00110$
      000232                        406 00109$:
      000232 18 A0                  407 	txau
      000234 7F D4                  408 	andi	#0x7f
      000236 A1 A1                  409 	addaxu
      000238 10 A0                  410 	txa
      00023A A0 A1                  411 	addax
      00023C 80 8A                  412 	call	ix
      00023E 01 FA                  413 	stax	1(sp)
      000240                        414 00110$:
      000240 01 F2                  415 	ldax	1(sp)
      000242 80 A0                  416 	push	a
      000244 40 00                  417 	jal	_putc
      000246 01 94                  418 	ads	#1
      000248                        419 00101$:
                                    420 ;	harness.h: 35: }
      000248 03 94                  421 	ads	#3
      00024A 64 A1                  422 	lra
      00024C 00 8A                  423 	ret
                                    424 ;	test_digits.c: 3: static void calculate_digit (value_t *value, unsigned char radix) {
                                    425 ;	---------------------------------
                                    426 ;	 Function calculate_digit
                                    427 ;	---------------------------------
      00024E                        428 _calculate_digit:
      00024E 60 A1                  429 	sra
      000250 F1 97                  430 	ads	#-15
                                    431 ;	test_digits.c: 4: unsigned long ul = value->ul;
      000252 12 F2                  432 	ldax	18(sp)
      000254 0E FA                  433 	stax	14(sp)
      000256 13 F2                  434 	ldax	19(sp)
      000258 0F FA                  435 	stax	15(sp)
      00025A 0E F2                  436 	ldax	14(sp)
      00025C 0C FA                  437 	stax	12(sp)
      00025E 0F F2                  438 	ldax	15(sp)
      000260 0D FA                  439 	stax	13(sp)
      000262 0C CC                  440 	ldxx	12(sp)
      000264 18 A0                  441 	txau
      000266 47 A0                  442 	btst	7
      000268 0A B8                  443 	bz	00127$
      00026A 00 F0                  444 	ldax	0(ix)
      00026C 08 FA                  445 	stax	8(sp)
      00026E 01 F0                  446 	ldax	1(ix)
      000270 09 FA                  447 	stax	9(sp)
      000272 02 F0                  448 	ldax	2(ix)
      000274 0A FA                  449 	stax	10(sp)
      000276 03 F0                  450 	ldax	3(ix)
      000278 0B FA                  451 	stax	11(sp)
      00027A 11 B0                  452 	br	00128$
      00027C                        453 00127$:
      00027C 18 A0                  454 	txau
      00027E 7F D4                  455 	andi	#0x7f
      000280 A1 A1                  456 	addaxu
      000282 10 A0                  457 	txa
      000284 A0 A1                  458 	addax
      000286 80 8A                  459 	call	ix
      000288 01 98                  460 	adx	#1
      00028A 08 FA                  461 	stax	8(sp)
      00028C 80 8A                  462 	call	ix
      00028E 01 98                  463 	adx	#1
      000290 09 FA                  464 	stax	9(sp)
      000292 80 8A                  465 	call	ix
      000294 01 98                  466 	adx	#1
      000296 0A FA                  467 	stax	10(sp)
      000298 80 8A                  468 	call	ix
      00029A 0B FA                  469 	stax	11(sp)
      00029C                        470 00128$:
                                    471 ;	test_digits.c: 5: unsigned char *pb4 = &value->byte[4];
      00029C 0E F2                  472 	ldax	14(sp)
      00029E 08 A0                  473 	ldc	#0
      0002A0 04 90                  474 	adc	#0x04
      0002A2 0E FA                  475 	stax	14(sp)
      0002A4 0F F2                  476 	ldax	15(sp)
      0002A6 00 90                  477 	adc	#0x00
      0002A8 0F FA                  478 	stax	15(sp)
                                    479 ;	test_digits.c: 7: do {
      0002AA 20 80                  480 	ldi	#0x20
      0002AC 07 FA                  481 	stax	7(sp)
      0002AE                        482 00103$:
                                    483 ;	test_digits.c: 8: *pb4 = (*pb4 << 1) | ((ul >> 31) & 0x01);
      0002AE 0E CC                  484 	ldxx	14(sp)
      0002B0 18 A0                  485 	txau
      0002B2 47 A0                  486 	btst	7
      0002B4 04 B8                  487 	bz	00129$
      0002B6 00 F0                  488 	ldax	0(ix)
      0002B8 06 FA                  489 	stax	6(sp)
      0002BA 08 B0                  490 	br	00130$
      0002BC                        491 00129$:
      0002BC 18 A0                  492 	txau
      0002BE 7F D4                  493 	andi	#0x7f
      0002C0 A1 A1                  494 	addaxu
      0002C2 10 A0                  495 	txa
      0002C4 A0 A1                  496 	addax
      0002C6 80 8A                  497 	call	ix
      0002C8 06 FA                  498 	stax	6(sp)
      0002CA                        499 00130$:
      0002CA 06 F2                  500 	ldax	6(sp)
      0002CC 08 A0                  501 	ldc	#0
      0002CE 00 A0                  502 	shl
      0002D0 06 FA                  503 	stax	6(sp)
      0002D2 0B F2                  504 	ldax	11(sp)
      0002D4 02 FA                  505 	stax	2(sp)
      0002D6 00 80                  506 	ldi	#0x00
      0002D8 03 FA                  507 	stax	3(sp)
      0002DA 00 80                  508 	ldi	#0x00
      0002DC 04 FA                  509 	stax	4(sp)
      0002DE 00 80                  510 	ldi	#0x00
      0002E0 05 FA                  511 	stax	5(sp)
      0002E2 08 A0                  512 	ldc	#0
      0002E4 04 A0                  513 	shr
      0002E6 05 FA                  514 	stax	5(sp)
      0002E8 04 F2                  515 	ldax	4(sp)
      0002EA 04 A0                  516 	shr
      0002EC 04 FA                  517 	stax	4(sp)
      0002EE 03 F2                  518 	ldax	3(sp)
      0002F0 04 A0                  519 	shr
      0002F2 03 FA                  520 	stax	3(sp)
      0002F4 02 F2                  521 	ldax	2(sp)
      0002F6 04 A0                  522 	shr
      0002F8 02 FA                  523 	stax	2(sp)
      0002FA 05 F2                  524 	ldax	5(sp)
      0002FC 08 A0                  525 	ldc	#0
      0002FE 04 A0                  526 	shr
      000300 05 FA                  527 	stax	5(sp)
      000302 04 F2                  528 	ldax	4(sp)
      000304 04 A0                  529 	shr
      000306 04 FA                  530 	stax	4(sp)
      000308 03 F2                  531 	ldax	3(sp)
      00030A 04 A0                  532 	shr
      00030C 03 FA                  533 	stax	3(sp)
      00030E 02 F2                  534 	ldax	2(sp)
      000310 04 A0                  535 	shr
      000312 02 FA                  536 	stax	2(sp)
      000314 05 F2                  537 	ldax	5(sp)
      000316 08 A0                  538 	ldc	#0
      000318 04 A0                  539 	shr
      00031A 05 FA                  540 	stax	5(sp)
      00031C 04 F2                  541 	ldax	4(sp)
      00031E 04 A0                  542 	shr
      000320 04 FA                  543 	stax	4(sp)
      000322 03 F2                  544 	ldax	3(sp)
      000324 04 A0                  545 	shr
      000326 03 FA                  546 	stax	3(sp)
      000328 02 F2                  547 	ldax	2(sp)
      00032A 04 A0                  548 	shr
      00032C 02 FA                  549 	stax	2(sp)
      00032E 05 F2                  550 	ldax	5(sp)
      000330 08 A0                  551 	ldc	#0
      000332 04 A0                  552 	shr
      000334 05 FA                  553 	stax	5(sp)
      000336 04 F2                  554 	ldax	4(sp)
      000338 04 A0                  555 	shr
      00033A 04 FA                  556 	stax	4(sp)
      00033C 03 F2                  557 	ldax	3(sp)
      00033E 04 A0                  558 	shr
      000340 03 FA                  559 	stax	3(sp)
      000342 02 F2                  560 	ldax	2(sp)
      000344 04 A0                  561 	shr
      000346 02 FA                  562 	stax	2(sp)
      000348 05 F2                  563 	ldax	5(sp)
      00034A 08 A0                  564 	ldc	#0
      00034C 04 A0                  565 	shr
      00034E 05 FA                  566 	stax	5(sp)
      000350 04 F2                  567 	ldax	4(sp)
      000352 04 A0                  568 	shr
      000354 04 FA                  569 	stax	4(sp)
      000356 03 F2                  570 	ldax	3(sp)
      000358 04 A0                  571 	shr
      00035A 03 FA                  572 	stax	3(sp)
      00035C 02 F2                  573 	ldax	2(sp)
      00035E 04 A0                  574 	shr
      000360 02 FA                  575 	stax	2(sp)
      000362 05 F2                  576 	ldax	5(sp)
      000364 08 A0                  577 	ldc	#0
      000366 04 A0                  578 	shr
      000368 05 FA                  579 	stax	5(sp)
      00036A 04 F2                  580 	ldax	4(sp)
      00036C 04 A0                  581 	shr
      00036E 04 FA                  582 	stax	4(sp)
      000370 03 F2                  583 	ldax	3(sp)
      000372 04 A0                  584 	shr
      000374 03 FA                  585 	stax	3(sp)
      000376 02 F2                  586 	ldax	2(sp)
      000378 04 A0                  587 	shr
      00037A 02 FA                  588 	stax	2(sp)
      00037C 05 F2                  589 	ldax	5(sp)
      00037E 08 A0                  590 	ldc	#0
      000380 04 A0                  591 	shr
      000382 05 FA                  592 	stax	5(sp)
      000384 04 F2                  593 	ldax	4(sp)
      000386 04 A0                  594 	shr
      000388 04 FA                  595 	stax	4(sp)
      00038A 03 F2                  596 	ldax	3(sp)
      00038C 04 A0                  597 	shr
      00038E 03 FA                  598 	stax	3(sp)
      000390 02 F2                  599 	ldax	2(sp)
      000392 04 A0                  600 	shr
      000394 02 FA                  601 	stax	2(sp)
      000396 01 FA                  602 	stax	1(sp)
      000398 01 D4                  603 	andi	#0x01
      00039A 01 FA                  604 	stax	1(sp)
      00039C 06 F2                  605 	ldax	6(sp)
      00039E 01 DA                  606 	or	1(sp)
      0003A0 01 FA                  607 	stax	1(sp)
      0003A2 0E CC                  608 	ldxx	14(sp)
      0003A4 01 F2                  609 	ldax	1(sp)
      0003A6 00 F8                  610 	stax	0(ix)
                                    611 ;	test_digits.c: 9: ul <<= 1;
      0003A8 08 F2                  612 	ldax	8(sp)
      0003AA 08 A0                  613 	ldc	#0
      0003AC 08 C2                  614 	add	8(sp)
      0003AE 08 FA                  615 	stax	8(sp)
      0003B0 09 F2                  616 	ldax	9(sp)
      0003B2 09 C2                  617 	add	9(sp)
      0003B4 09 FA                  618 	stax	9(sp)
      0003B6 0A F2                  619 	ldax	10(sp)
      0003B8 0A C2                  620 	add	10(sp)
      0003BA 0A FA                  621 	stax	10(sp)
      0003BC 0B F2                  622 	ldax	11(sp)
      0003BE 0B C2                  623 	add	11(sp)
      0003C0 0B FA                  624 	stax	11(sp)
                                    625 ;	test_digits.c: 10: if (radix <= *pb4) { *pb4 -= radix; ul |= 1; }
      0003C2 14 F2                  626 	ldax	20(sp)
      0003C4 01 EA                  627 	cmp	1(sp)
      0003C6 04 A2                  628 	if	gt
      0003C8 1B B0                  629 	br.p	00104$
      0003CA 0E CC                  630 	ldxx	14(sp)
      0003CC 18 A0                  631 	txau
      0003CE 47 A0                  632 	btst	7
      0003D0 04 B8                  633 	bz	00134$
      0003D2 00 F0                  634 	ldax	0(ix)
      0003D4 01 FA                  635 	stax	1(sp)
      0003D6 08 B0                  636 	br	00135$
      0003D8                        637 00134$:
      0003D8 18 A0                  638 	txau
      0003DA 7F D4                  639 	andi	#0x7f
      0003DC A1 A1                  640 	addaxu
      0003DE 10 A0                  641 	txa
      0003E0 A0 A1                  642 	addax
      0003E2 80 8A                  643 	call	ix
      0003E4 01 FA                  644 	stax	1(sp)
      0003E6                        645 00135$:
      0003E6 01 F2                  646 	ldax	1(sp)
      0003E8 08 A0                  647 	ldc	#0
      0003EA 14 CA                  648 	sub	20(sp)
      0003EC 01 FA                  649 	stax	1(sp)
      0003EE 0E CC                  650 	ldxx	14(sp)
      0003F0 01 F2                  651 	ldax	1(sp)
      0003F2 00 F8                  652 	stax	0(ix)
      0003F4 08 F2                  653 	ldax	8(sp)
      0003F6 FE D4                  654 	andi	#0xfe
      0003F8 08 A0                  655 	ldc	#0
      0003FA 01 90                  656 	adc	#0x01
      0003FC 08 FA                  657 	stax	8(sp)
      0003FE                        658 00104$:
                                    659 ;	test_digits.c: 11: } while (--i);
      0003FE 07 9E                  660 	dcx	7(sp)
      000400 07 F2                  661 	ldax	7(sp)
      000402 56 AF                  662 	bnz	00103$
                                    663 ;	test_digits.c: 12: value->ul = ul;
      000404 0C CC                  664 	ldxx	12(sp)
      000406 08 F2                  665 	ldax	8(sp)
      000408 00 F8                  666 	stax	0(ix)
      00040A 09 F2                  667 	ldax	9(sp)
      00040C 01 F8                  668 	stax	1(ix)
      00040E 0A F2                  669 	ldax	10(sp)
      000410 02 F8                  670 	stax	2(ix)
      000412 0B F2                  671 	ldax	11(sp)
      000414 03 F8                  672 	stax	3(ix)
      000416                        673 00106$:
                                    674 ;	test_digits.c: 13: }
      000416 0F 94                  675 	ads	#15
      000418 64 A1                  676 	lra
      00041A 00 8A                  677 	ret
                                    678 ;	test_digits.c: 15: int main(void) {
                                    679 ;	---------------------------------
                                    680 ;	 Function main
                                    681 ;	---------------------------------
      00041C                        682 _main:
      00041C 60 A1                  683 	sra
      00041E F5 97                  684 	ads	#-11
                                    685 ;	test_digits.c: 17: value.ul = 123456; value.byte[4] = 0;
      000420 CC 8A                  686 	spix
      000422 01 98                  687 	adx	#1
      000424 40 80                  688 	ldi	#0x40
      000426 00 F8                  689 	stax	0(ix)
      000428 E2 80                  690 	ldi	#0xe2
      00042A 01 F8                  691 	stax	1(ix)
      00042C 01 80                  692 	ldi	#0x01
      00042E 02 F8                  693 	stax	2(ix)
      000430 00 80                  694 	ldi	#0x00
      000432 03 F8                  695 	stax	3(ix)
      000434 CC 8A                  696 	spix
      000436 05 98                  697 	adx	#5
      000438 00 80                  698 	ldi	#0x00
      00043A 00 F8                  699 	stax	0(ix)
                                    700 ;	test_digits.c: 18: calculate_digit(&value, 10); d1 = value.byte[4];
      00043C 0A 80                  701 	ldi	#0x0a
      00043E 80 A0                  702 	push	a
      000440 CC 8A                  703 	spix
      000442 10 A0                  704 	txa
      000444 08 A0                  705 	ldc	#0
      000446 02 90                  706 	adc	#0x02
      000448 18 A0                  707 	txau
      00044A 7F D4                  708 	andi	#0x7f
      00044C 00 90                  709 	adc	#0x00
      00044E 80 A0                  710 	push	a
      000450 CC 8A                  711 	spix
      000452 10 A0                  712 	txa
      000454 08 A0                  713 	ldc	#0
      000456 03 90                  714 	adc	#0x03
      000458 80 A0                  715 	push	a
      00045A 27 01                  716 	jal	_calculate_digit
      00045C 03 94                  717 	ads	#3
      00045E CC 8A                  718 	spix
      000460 05 98                  719 	adx	#5
      000462 00 F0                  720 	ldax	0(ix)
      000464 01 FC                  721 	sta	_d1
                                    722 ;	test_digits.c: 19: value.byte[4] = 0; calculate_digit(&value, 10); d2 = value.byte[4];
      000466 00 80                  723 	ldi	#0x00
      000468 00 F8                  724 	stax	0(ix)
      00046A 0A 80                  725 	ldi	#0x0a
      00046C 80 A0                  726 	push	a
      00046E CC 8A                  727 	spix
      000470 10 A0                  728 	txa
      000472 08 A0                  729 	ldc	#0
      000474 02 90                  730 	adc	#0x02
      000476 18 A0                  731 	txau
      000478 7F D4                  732 	andi	#0x7f
      00047A 00 90                  733 	adc	#0x00
      00047C 80 A0                  734 	push	a
      00047E CC 8A                  735 	spix
      000480 10 A0                  736 	txa
      000482 08 A0                  737 	ldc	#0
      000484 03 90                  738 	adc	#0x03
      000486 80 A0                  739 	push	a
      000488 27 01                  740 	jal	_calculate_digit
      00048A 03 94                  741 	ads	#3
      00048C CC 8A                  742 	spix
      00048E 05 98                  743 	adx	#5
      000490 00 F0                  744 	ldax	0(ix)
      000492 02 FC                  745 	sta	_d2
                                    746 ;	test_digits.c: 20: CHECK(1, d1 == 6);
      000494 01 F4                  747 	lda	_d1
      000496 06 A4                  748 	cpi	#0x06
      000498 08 A8                  749 	bnz	00102$
      00049A 81 80                  750 	ldi	#>(___str_1 + 0)
      00049C 80 A0                  751 	push	a
      00049E A3 80                  752 	ldi	#<(___str_1 + 0)
      0004A0 80 A0                  753 	push	a
      0004A2 49 00                  754 	jal	_puts
      0004A4 02 94                  755 	ads	#2
      0004A6 0A B0                  756 	br	00103$
      0004A8                        757 00102$:
      0004A8 81 80                  758 	ldi	#>(___str_2 + 0)
      0004AA 80 A0                  759 	push	a
      0004AC A7 80                  760 	ldi	#<(___str_2 + 0)
      0004AE 80 A0                  761 	push	a
      0004B0 49 00                  762 	jal	_puts
      0004B2 02 94                  763 	ads	#2
      0004B4 80 A1 00 00            764 	ldx	#_fails
      0004B8 00 E4                  765 	inx	0(ix)
      0004BA                        766 00103$:
      0004BA 00 80                  767 	ldi	#0x00
      0004BC 80 A0                  768 	push	a
      0004BE 01 80                  769 	ldi	#0x01
      0004C0 80 A0                  770 	push	a
      0004C2 6A 00                  771 	jal	_puthex
      0004C4 02 94                  772 	ads	#2
      0004C6 0A 80                  773 	ldi	#0x0a
      0004C8 80 A0                  774 	push	a
      0004CA 40 00                  775 	jal	_putc
      0004CC 01 94                  776 	ads	#1
                                    777 ;	test_digits.c: 21: CHECK(2, d2 == 5);
      0004CE 02 F4                  778 	lda	_d2
      0004D0 05 A4                  779 	cpi	#0x05
      0004D2 08 A8                  780 	bnz	00108$
      0004D4 81 80                  781 	ldi	#>(___str_1 + 0)
      0004D6 80 A0                  782 	push	a
      0004D8 A3 80                  783 	ldi	#<(___str_1 + 0)
      0004DA 80 A0                  784 	push	a
      0004DC 49 00                  785 	jal	_puts
      0004DE 02 94                  786 	ads	#2
      0004E0 0A B0                  787 	br	00109$
      0004E2                        788 00108$:
      0004E2 81 80                  789 	ldi	#>(___str_2 + 0)
      0004E4 80 A0                  790 	push	a
      0004E6 A7 80                  791 	ldi	#<(___str_2 + 0)
      0004E8 80 A0                  792 	push	a
      0004EA 49 00                  793 	jal	_puts
      0004EC 02 94                  794 	ads	#2
      0004EE 80 A1 00 00            795 	ldx	#_fails
      0004F2 00 E4                  796 	inx	0(ix)
      0004F4                        797 00109$:
      0004F4 00 80                  798 	ldi	#0x00
      0004F6 80 A0                  799 	push	a
      0004F8 02 80                  800 	ldi	#0x02
      0004FA 80 A0                  801 	push	a
      0004FC 6A 00                  802 	jal	_puthex
      0004FE 02 94                  803 	ads	#2
      000500 0A 80                  804 	ldi	#0x0a
      000502 80 A0                  805 	push	a
      000504 40 00                  806 	jal	_putc
      000506 01 94                  807 	ads	#1
                                    808 ;	test_digits.c: 22: CHECK(3, value.ul == 1234);
      000508 CC 8A                  809 	spix
      00050A 01 98                  810 	adx	#1
      00050C 00 F0                  811 	ldax	0(ix)
      00050E 08 FA                  812 	stax	8(sp)
      000510 01 F0                  813 	ldax	1(ix)
      000512 09 FA                  814 	stax	9(sp)
      000514 02 F0                  815 	ldax	2(ix)
      000516 0A FA                  816 	stax	10(sp)
      000518 03 F0                  817 	ldax	3(ix)
      00051A 0B FA                  818 	stax	11(sp)
      00051C 08 F2                  819 	ldax	8(sp)
      00051E D2 A4                  820 	cpi	#0xd2
      000520 09 A8                  821 	bnz	00170$
      000522 09 F2                  822 	ldax	9(sp)
      000524 04 A4                  823 	cpi	#0x04
      000526 06 A8                  824 	bnz	00170$
      000528 0A F2                  825 	ldax	10(sp)
      00052A 00 A4                  826 	cpi	#0x00
      00052C 03 A8                  827 	bnz	00170$
      00052E 0B F2                  828 	ldax	11(sp)
      000530 00 A4                  829 	cpi	#0x00
      000532                        830 00170$:
      000532 08 A8                  831 	bnz	00114$
      000534 81 80                  832 	ldi	#>(___str_1 + 0)
      000536 80 A0                  833 	push	a
      000538 A3 80                  834 	ldi	#<(___str_1 + 0)
      00053A 80 A0                  835 	push	a
      00053C 49 00                  836 	jal	_puts
      00053E 02 94                  837 	ads	#2
      000540 0A B0                  838 	br	00115$
      000542                        839 00114$:
      000542 81 80                  840 	ldi	#>(___str_2 + 0)
      000544 80 A0                  841 	push	a
      000546 A7 80                  842 	ldi	#<(___str_2 + 0)
      000548 80 A0                  843 	push	a
      00054A 49 00                  844 	jal	_puts
      00054C 02 94                  845 	ads	#2
      00054E 80 A1 00 00            846 	ldx	#_fails
      000552 00 E4                  847 	inx	0(ix)
      000554                        848 00115$:
      000554 00 80                  849 	ldi	#0x00
      000556 80 A0                  850 	push	a
      000558 03 80                  851 	ldi	#0x03
      00055A 80 A0                  852 	push	a
      00055C 6A 00                  853 	jal	_puthex
      00055E 02 94                  854 	ads	#2
      000560 0A 80                  855 	ldi	#0x0a
      000562 80 A0                  856 	push	a
      000564 40 00                  857 	jal	_putc
      000566 01 94                  858 	ads	#1
                                    859 ;	test_digits.c: 23: value.ul = 42; value.byte[4] = 0;
      000568 CC 8A                  860 	spix
      00056A 01 98                  861 	adx	#1
      00056C 2A 80                  862 	ldi	#0x2a
      00056E 00 F8                  863 	stax	0(ix)
      000570 00 80                  864 	ldi	#0x00
      000572 01 F8                  865 	stax	1(ix)
      000574 00 80                  866 	ldi	#0x00
      000576 02 F8                  867 	stax	2(ix)
      000578 00 80                  868 	ldi	#0x00
      00057A 03 F8                  869 	stax	3(ix)
      00057C CC 8A                  870 	spix
      00057E 05 98                  871 	adx	#5
      000580 00 80                  872 	ldi	#0x00
      000582 00 F8                  873 	stax	0(ix)
                                    874 ;	test_digits.c: 24: calculate_digit(&value, 10); d3 = value.byte[4];
      000584 0A 80                  875 	ldi	#0x0a
      000586 80 A0                  876 	push	a
      000588 CC 8A                  877 	spix
      00058A 10 A0                  878 	txa
      00058C 08 A0                  879 	ldc	#0
      00058E 02 90                  880 	adc	#0x02
      000590 18 A0                  881 	txau
      000592 7F D4                  882 	andi	#0x7f
      000594 00 90                  883 	adc	#0x00
      000596 80 A0                  884 	push	a
      000598 CC 8A                  885 	spix
      00059A 10 A0                  886 	txa
      00059C 08 A0                  887 	ldc	#0
      00059E 03 90                  888 	adc	#0x03
      0005A0 80 A0                  889 	push	a
      0005A2 27 01                  890 	jal	_calculate_digit
      0005A4 03 94                  891 	ads	#3
      0005A6 CC 8A                  892 	spix
      0005A8 05 98                  893 	adx	#5
      0005AA 00 F0                  894 	ldax	0(ix)
      0005AC 03 FC                  895 	sta	_d3
                                    896 ;	test_digits.c: 25: CHECK(4, d3 == 2 && value.ul == 4);
      0005AE 03 F4                  897 	lda	_d3
      0005B0 02 A4                  898 	cpi	#0x02
      0005B2 1E A8                  899 	bnz	00120$
      0005B4 CC 8A                  900 	spix
      0005B6 01 98                  901 	adx	#1
      0005B8 00 F0                  902 	ldax	0(ix)
      0005BA 08 FA                  903 	stax	8(sp)
      0005BC 01 F0                  904 	ldax	1(ix)
      0005BE 09 FA                  905 	stax	9(sp)
      0005C0 02 F0                  906 	ldax	2(ix)
      0005C2 0A FA                  907 	stax	10(sp)
      0005C4 03 F0                  908 	ldax	3(ix)
      0005C6 0B FA                  909 	stax	11(sp)
      0005C8 08 F2                  910 	ldax	8(sp)
      0005CA 04 A4                  911 	cpi	#0x04
      0005CC 09 A8                  912 	bnz	00174$
      0005CE 09 F2                  913 	ldax	9(sp)
      0005D0 00 A4                  914 	cpi	#0x00
      0005D2 06 A8                  915 	bnz	00174$
      0005D4 0A F2                  916 	ldax	10(sp)
      0005D6 00 A4                  917 	cpi	#0x00
      0005D8 03 A8                  918 	bnz	00174$
      0005DA 0B F2                  919 	ldax	11(sp)
      0005DC 00 A4                  920 	cpi	#0x00
      0005DE                        921 00174$:
      0005DE 08 A8                  922 	bnz	00120$
      0005E0 81 80                  923 	ldi	#>(___str_1 + 0)
      0005E2 80 A0                  924 	push	a
      0005E4 A3 80                  925 	ldi	#<(___str_1 + 0)
      0005E6 80 A0                  926 	push	a
      0005E8 49 00                  927 	jal	_puts
      0005EA 02 94                  928 	ads	#2
      0005EC 0A B0                  929 	br	00121$
      0005EE                        930 00120$:
      0005EE 81 80                  931 	ldi	#>(___str_2 + 0)
      0005F0 80 A0                  932 	push	a
      0005F2 A7 80                  933 	ldi	#<(___str_2 + 0)
      0005F4 80 A0                  934 	push	a
      0005F6 49 00                  935 	jal	_puts
      0005F8 02 94                  936 	ads	#2
      0005FA 80 A1 00 00            937 	ldx	#_fails
      0005FE 00 E4                  938 	inx	0(ix)
      000600                        939 00121$:
      000600 00 80                  940 	ldi	#0x00
      000602 80 A0                  941 	push	a
      000604 04 80                  942 	ldi	#0x04
      000606 80 A0                  943 	push	a
      000608 6A 00                  944 	jal	_puthex
      00060A 02 94                  945 	ads	#2
      00060C 0A 80                  946 	ldi	#0x0a
      00060E 80 A0                  947 	push	a
      000610 40 00                  948 	jal	_putc
      000612 01 94                  949 	ads	#1
                                    950 ;	test_digits.c: 26: DONE();
      000614 00 F4                  951 	lda	_fails
      000616 06 B8                  952 	bz	00128$
      000618 AD 80                  953 	ldi	#<(___str_3 + 0)
      00061A 06 FA                  954 	stax	6(sp)
      00061C 81 80                  955 	ldi	#>(___str_3 + 0)
      00061E 07 FA                  956 	stax	7(sp)
      000620 05 B0                  957 	br	00129$
      000622                        958 00128$:
      000622 BA 80                  959 	ldi	#<(___str_4 + 0)
      000624 06 FA                  960 	stax	6(sp)
      000626 81 80                  961 	ldi	#>(___str_4 + 0)
      000628 07 FA                  962 	stax	7(sp)
      00062A                        963 00129$:
      00062A 07 F2                  964 	ldax	7(sp)
      00062C 80 A0                  965 	push	a
      00062E 07 F2                  966 	ldax	7(sp)
      000630 80 A0                  967 	push	a
      000632 49 00                  968 	jal	_puts
      000634 02 94                  969 	ads	#2
                                    970 ;	test_digits.c: 27: return 0;
      000636 00 80                  971 	ldi	#0x00
      000638 0E FA                  972 	stax	14(sp)
      00063A 0F FA                  973 	stax	15(sp)
      00063C                        974 00126$:
                                    975 ;	test_digits.c: 28: }
      00063C 0B 94                  976 	ads	#11
      00063E 64 A1                  977 	lra
      000640 00 8A                  978 	ret
                                    979 	.area CODE (CODE)
                                    980 	.area CONST (CODE,CDATA)
                                    981 	.area CONST (CODE,CDATA)
      000648                        982 ___str_0:
      000648 30 80 00 8A 31 80 00   983 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      000688 00 80 00 8A            984 	.db 0x00
                                    985 	.area CODE (CODE)
                                    986 	.area CONST (CODE,CDATA)
      00068C                        987 ___str_1:
      00068C 6F 80 00 8A 6B 80 00   988 	.ascii "ok "
             8A 20 80 00 8A
      000698 00 80 00 8A            989 	.db 0x00
                                    990 	.area CODE (CODE)
                                    991 	.area CONST (CODE,CDATA)
      00069C                        992 ___str_2:
      00069C 46 80 00 8A 41 80 00   993 	.ascii "FAIL "
             8A 49 80 00 8A 4C 80
             00 8A 20 80 00 8A
      0006B0 00 80 00 8A            994 	.db 0x00
                                    995 	.area CODE (CODE)
                                    996 	.area CONST (CODE,CDATA)
      0006B4                        997 ___str_3:
      0006B4 53 80 00 8A 4F 80 00   998 	.ascii "SOME FAILED"
             8A 4D 80 00 8A 45 80
             00 8A 20 80 00 8A 46
             80 00 8A 41 80 00 8A
             49 80 00 8A 4C 80 00
             8A 45 80 00 8A 44 80
             00 8A
      0006E0 0A 80 00 8A            999 	.db 0x0a
      0006E4 00 80 00 8A           1000 	.db 0x00
                                   1001 	.area CODE (CODE)
                                   1002 	.area CONST (CODE,CDATA)
      0006E8                       1003 ___str_4:
      0006E8 41 80 00 8A 4C 80 00  1004 	.ascii "ALL PASSED"
             8A 4C 80 00 8A 20 80
             00 8A 50 80 00 8A 41
             80 00 8A 53 80 00 8A
             53 80 00 8A 45 80 00
             8A 44 80 00 8A
      000710 0A 80 00 8A           1005 	.db 0x0a
      000714 00 80 00 8A           1006 	.db 0x00
                                   1007 	.area CODE (CODE)
                                   1008 	.area INITIALIZER (CODE,CDATA)
                                   1009 	.area CABS (ABS,CODE,CDATA)
