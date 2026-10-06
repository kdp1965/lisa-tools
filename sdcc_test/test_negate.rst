                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_negate
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
                                     17 	.globl _ul
                                     18 	.globl _neg8
                                     19 	.globl _neg32
                                     20 	.globl _neg16
                                     21 	.globl _UART_STATUS
                                     22 	.globl _UART_TX
                                     23 ;--------------------------------------------------------
                                     24 ; special function registers
                                     25 ;--------------------------------------------------------
                                     26 	.area RSEG (ABS)
      000000                         27 	.org 0x0000
                           000210    28 _UART_TX	=	0x0210
                           000211    29 _UART_STATUS	=	0x0211
                                     30 ;--------------------------------------------------------
                                     31 ; ram data
                                     32 ;--------------------------------------------------------
                                     33 	.area DATA
      000000                         34 _fails:
      000000                         35 	.ds 1
                                     36 ;--------------------------------------------------------
                                     37 ; ram data
                                     38 ;--------------------------------------------------------
                                     39 	.area INITIALIZED
                                     40 ;--------------------------------------------------------
                                     41 ; overlayable items in ram
                                     42 ;--------------------------------------------------------
                                     43 ;--------------------------------------------------------
                                     44 ; Stack segment in internal ram
                                     45 ;--------------------------------------------------------
                                     46 	.area SSEG
      000001                         47 __start__stack:
      000001                         48 	.ds	1
                                     49 
                                     50 ;--------------------------------------------------------
                                     51 ; absolute external ram data
                                     52 ;--------------------------------------------------------
                                     53 	.area DABS (ABS)
                                     54 ;--------------------------------------------------------
                                     55 ; interrupt vector
                                     56 ;--------------------------------------------------------
                                     57 	.area HOME (CODE)
      000000                         58 __interrupt_vect:
      000000 0C 00                   59 	jal	__sdcc_gsinit_startup
      000002 40 8B                   60 	rets
      000004 40 8B                   61 	rets
      000006 40 8B                   62 	rets
      000008 40 8B                   63 	rets
      00000A 40 8B                   64 	rets
      00000C 40 8B                   65 	rets
      00000E 40 8B                   66 	rets
      000010 40 8B                   67 	rets
      000012 40 8B                   68 	rets
                                     69 ;--------------------------------------------------------
                                     70 ; global & static initialisations
                                     71 ;--------------------------------------------------------
                                     72 	.area HOME (CODE)
                                     73 	.area GSINIT (CODE)
                                     74 	.area GSFINAL (CODE)
                                     75 	.area GSINIT (CODE)
                                     76 	.area GSINIT (CODE)
      000018                         77 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             78 	ldx	#0x007f
      00001C C8 8A                   79 	xchg	sp
      00001E 41 A1                   80 	amode	1
      000020 D5 02                   81 	jal	___sdcc_external_startup
      000022 00 A4                   82 	cpi	#0
      000024 01 A2                   83 	if	ne
      000026 0A 00                   84 	jal	__sdcc_program_startup
      000028 00 80                   85 	ldi	#>l_DATA
      00002A 80 A0                   86 	push	a
      00002C 01 80                   87 	ldi	#<l_DATA
      00002E 80 A0                   88 	push	a
      000030 80 A1 00 00             89 	ldx	#s_DATA
      000034                         90 00001$:
      000034 01 F2                   91 	ldax	1(sp)
      000036 02 DA                   92 	or	2(sp)
      000038 08 B8                   93 	bz	00002$
      00003A 00 80                   94 	ldi	#0
      00003C 00 F8                   95 	stax	0(ix)
      00003E 01 98                   96 	adx	#1
      000040 01 9E                   97 	dcx	1(sp)
      000042 03 A2                   98 	if	c
      000044 02 9E                   99 	dcx	2(sp)
      000046 F7 B7                  100 	br	00001$
      000048                        101 00002$:
      000048 00 80                  102 	ldi	#>s_INITIALIZED
      00004A 80 A0                  103 	push	a
      00004C 01 80                  104 	ldi	#<s_INITIALIZED
      00004E 80 A0                  105 	push	a
      000050 00 80                  106 	ldi	#>l_INITIALIZED
      000052 04 FA                  107 	stax	4(sp)
      000054 00 80                  108 	ldi	#<l_INITIALIZED
      000056 03 FA                  109 	stax	3(sp)
      000058 80 A1 40 83            110 	ldx	#s_INITIALIZER
      00005C                        111 00003$:
      00005C 03 F2                  112 	ldax	3(sp)
      00005E 04 DA                  113 	or	4(sp)
      000060 0E B8                  114 	bz	00004$
      000062 80 8A                  115 	call	ix
      000064 01 98                  116 	adx	#1
      000066 68 A1                  117 	push	ix
      000068 03 CC                  118 	ldxx	3(sp)
      00006A 00 F8                  119 	stax	0(ix)
      00006C 03 E6                  120 	inx	3(sp)
      00006E 03 A2                  121 	if	c
      000070 04 E6                  122 	inx	4(sp)
      000072 6C A1                  123 	pop	ix
      000074 03 9E                  124 	dcx	3(sp)
      000076 03 A2                  125 	if	c
      000078 04 9E                  126 	dcx	4(sp)
      00007A F1 B7                  127 	br	00003$
      00007C                        128 00004$:
      00007C 04 94                  129 	ads	#4
                                    130 	.area GSFINAL (CODE)
      00007E 0A 00                  131 	jal	__sdcc_program_startup
                                    132 ;--------------------------------------------------------
                                    133 ; Home
                                    134 ;--------------------------------------------------------
                                    135 	.area HOME (CODE)
                                    136 	.area HOME (CODE)
      000014                        137 __sdcc_program_startup:
      000014 91 01                  138 	jal	_main
      000016                        139 00001$:
      000016 00 B0                  140 	br	00001$
                                    141 ;	return from main will return to caller
                                    142 ;--------------------------------------------------------
                                    143 ; code
                                    144 ;--------------------------------------------------------
                                    145 	.area CODE (CODE)
                                    146 ;	harness.h: 15: static void putc(char c)
                                    147 ;	---------------------------------
                                    148 ;	 Function putc
                                    149 ;	---------------------------------
      000080                        150 _putc:
      000080 FF 97                  151 	ads	#-1
                                    152 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        153 00101$:
      000082 11 F6                  154 	lda	_UART_STATUS
      000084 01 FA                  155 	stax	1(sp)
      000086 02 D4                  156 	andi	#0x02
      000088 FD BF                  157 	bz	00101$
                                    158 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  159 	ldax	2(sp)
      00008C 10 FE                  160 	sta	_UART_TX
      00008E                        161 00104$:
                                    162 ;	harness.h: 20: }
      00008E 01 94                  163 	ads	#1
      000090 00 8A                  164 	ret
                                    165 ;	harness.h: 22: static void puts(const char *s)
                                    166 ;	---------------------------------
                                    167 ;	 Function puts
                                    168 ;	---------------------------------
      000092                        169 _puts:
      000092 60 A1                  170 	sra
      000094 FD 97                  171 	ads	#-3
                                    172 ;	harness.h: 24: while (*s)
      000096 06 F2                  173 	ldax	6(sp)
      000098 02 FA                  174 	stax	2(sp)
      00009A 07 F2                  175 	ldax	7(sp)
      00009C 03 FA                  176 	stax	3(sp)
      00009E                        177 00101$:
      00009E 02 CC                  178 	ldxx	2(sp)
      0000A0 18 A0                  179 	txau
      0000A2 47 A0                  180 	btst	7
      0000A4 04 B8                  181 	bz	00119$
      0000A6 00 F0                  182 	ldax	0(ix)
      0000A8 01 FA                  183 	stax	1(sp)
      0000AA 08 B0                  184 	br	00120$
      0000AC                        185 00119$:
      0000AC 18 A0                  186 	txau
      0000AE 7F D4                  187 	andi	#0x7f
      0000B0 A1 A1                  188 	addaxu
      0000B2 10 A0                  189 	txa
      0000B4 A0 A1                  190 	addax
      0000B6 80 8A                  191 	call	ix
      0000B8 01 FA                  192 	stax	1(sp)
      0000BA                        193 00120$:
      0000BA 01 F2                  194 	ldax	1(sp)
      0000BC 09 B8                  195 	bz	00104$
                                    196 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  197 	inx	2(sp)
      0000C0 03 A2                  198 	if	c
      0000C2 03 E6                  199 	inx.p	3(sp)
      0000C4 01 F2                  200 	ldax	1(sp)
      0000C6 80 A0                  201 	push	a
      0000C8 40 00                  202 	jal	_putc
      0000CA 01 94                  203 	ads	#1
      0000CC E9 B7                  204 	br	00101$
      0000CE                        205 00104$:
                                    206 ;	harness.h: 26: }
      0000CE 03 94                  207 	ads	#3
      0000D0 64 A1                  208 	lra
      0000D2 00 8A                  209 	ret
                                    210 ;	harness.h: 28: static void puthex(unsigned int v)
                                    211 ;	---------------------------------
                                    212 ;	 Function puthex
                                    213 ;	---------------------------------
      0000D4                        214 _puthex:
      0000D4 60 A1                  215 	sra
      0000D6 FD 97                  216 	ads	#-3
                                    217 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    218 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  219 	ldax	7(sp)
      0000DA 02 FA                  220 	stax	2(sp)
      0000DC 00 80                  221 	ldi	#0x00
      0000DE 03 FA                  222 	stax	3(sp)
      0000E0 08 A0                  223 	ldc	#0
      0000E2 04 A0                  224 	shr
      0000E4 03 FA                  225 	stax	3(sp)
      0000E6 02 F2                  226 	ldax	2(sp)
      0000E8 04 A0                  227 	shr
      0000EA 02 FA                  228 	stax	2(sp)
      0000EC 03 F2                  229 	ldax	3(sp)
      0000EE 08 A0                  230 	ldc	#0
      0000F0 04 A0                  231 	shr
      0000F2 03 FA                  232 	stax	3(sp)
      0000F4 02 F2                  233 	ldax	2(sp)
      0000F6 04 A0                  234 	shr
      0000F8 02 FA                  235 	stax	2(sp)
      0000FA 03 F2                  236 	ldax	3(sp)
      0000FC 08 A0                  237 	ldc	#0
      0000FE 04 A0                  238 	shr
      000100 03 FA                  239 	stax	3(sp)
      000102 02 F2                  240 	ldax	2(sp)
      000104 04 A0                  241 	shr
      000106 02 FA                  242 	stax	2(sp)
      000108 03 F2                  243 	ldax	3(sp)
      00010A 08 A0                  244 	ldc	#0
      00010C 04 A0                  245 	shr
      00010E 03 FA                  246 	stax	3(sp)
      000110 02 F2                  247 	ldax	2(sp)
      000112 04 A0                  248 	shr
      000114 02 FA                  249 	stax	2(sp)
      000116 0F D4                  250 	andi	#0x0f
      000118 02 FA                  251 	stax	2(sp)
      00011A 00 80                  252 	ldi	#0x00
      00011C 03 FA                  253 	stax	3(sp)
      00011E 02 F2                  254 	ldax	2(sp)
      000120 08 A0                  255 	ldc	#0
      000122 6C 90                  256 	adc	#<(___str_0 + 0)
      000124 02 FA                  257 	stax	2(sp)
      000126 03 F2                  258 	ldax	3(sp)
      000128 81 90                  259 	adc	#>(___str_0 + 0)
      00012A 03 FA                  260 	stax	3(sp)
      00012C 02 CC                  261 	ldxx	2(sp)
      00012E 18 A0                  262 	txau
      000130 47 A0                  263 	btst	7
      000132 04 B8                  264 	bz	00103$
      000134 00 F0                  265 	ldax	0(ix)
      000136 01 FA                  266 	stax	1(sp)
      000138 08 B0                  267 	br	00104$
      00013A                        268 00103$:
      00013A 18 A0                  269 	txau
      00013C 7F D4                  270 	andi	#0x7f
      00013E A1 A1                  271 	addaxu
      000140 10 A0                  272 	txa
      000142 A0 A1                  273 	addax
      000144 80 8A                  274 	call	ix
      000146 01 FA                  275 	stax	1(sp)
      000148                        276 00104$:
      000148 01 F2                  277 	ldax	1(sp)
      00014A 80 A0                  278 	push	a
      00014C 40 00                  279 	jal	_putc
      00014E 01 94                  280 	ads	#1
                                    281 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  282 	ldax	7(sp)
      000152 02 FA                  283 	stax	2(sp)
      000154 00 80                  284 	ldi	#0x00
      000156 03 FA                  285 	stax	3(sp)
      000158 02 F2                  286 	ldax	2(sp)
      00015A 0F D4                  287 	andi	#0x0f
      00015C 02 FA                  288 	stax	2(sp)
      00015E 00 80                  289 	ldi	#0x00
      000160 03 FA                  290 	stax	3(sp)
      000162 02 F2                  291 	ldax	2(sp)
      000164 08 A0                  292 	ldc	#0
      000166 6C 90                  293 	adc	#<(___str_0 + 0)
      000168 02 FA                  294 	stax	2(sp)
      00016A 03 F2                  295 	ldax	3(sp)
      00016C 81 90                  296 	adc	#>(___str_0 + 0)
      00016E 03 FA                  297 	stax	3(sp)
      000170 02 CC                  298 	ldxx	2(sp)
      000172 18 A0                  299 	txau
      000174 47 A0                  300 	btst	7
      000176 04 B8                  301 	bz	00105$
      000178 00 F0                  302 	ldax	0(ix)
      00017A 01 FA                  303 	stax	1(sp)
      00017C 08 B0                  304 	br	00106$
      00017E                        305 00105$:
      00017E 18 A0                  306 	txau
      000180 7F D4                  307 	andi	#0x7f
      000182 A1 A1                  308 	addaxu
      000184 10 A0                  309 	txa
      000186 A0 A1                  310 	addax
      000188 80 8A                  311 	call	ix
      00018A 01 FA                  312 	stax	1(sp)
      00018C                        313 00106$:
      00018C 01 F2                  314 	ldax	1(sp)
      00018E 80 A0                  315 	push	a
      000190 40 00                  316 	jal	_putc
      000192 01 94                  317 	ads	#1
                                    318 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  319 	ldax	6(sp)
      000196 02 FA                  320 	stax	2(sp)
      000198 07 F2                  321 	ldax	7(sp)
      00019A 03 FA                  322 	stax	3(sp)
      00019C 08 A0                  323 	ldc	#0
      00019E 04 A0                  324 	shr
      0001A0 03 FA                  325 	stax	3(sp)
      0001A2 02 F2                  326 	ldax	2(sp)
      0001A4 04 A0                  327 	shr
      0001A6 02 FA                  328 	stax	2(sp)
      0001A8 03 F2                  329 	ldax	3(sp)
      0001AA 08 A0                  330 	ldc	#0
      0001AC 04 A0                  331 	shr
      0001AE 03 FA                  332 	stax	3(sp)
      0001B0 02 F2                  333 	ldax	2(sp)
      0001B2 04 A0                  334 	shr
      0001B4 02 FA                  335 	stax	2(sp)
      0001B6 03 F2                  336 	ldax	3(sp)
      0001B8 08 A0                  337 	ldc	#0
      0001BA 04 A0                  338 	shr
      0001BC 03 FA                  339 	stax	3(sp)
      0001BE 02 F2                  340 	ldax	2(sp)
      0001C0 04 A0                  341 	shr
      0001C2 02 FA                  342 	stax	2(sp)
      0001C4 03 F2                  343 	ldax	3(sp)
      0001C6 08 A0                  344 	ldc	#0
      0001C8 04 A0                  345 	shr
      0001CA 03 FA                  346 	stax	3(sp)
      0001CC 02 F2                  347 	ldax	2(sp)
      0001CE 04 A0                  348 	shr
      0001D0 02 FA                  349 	stax	2(sp)
      0001D2 0F D4                  350 	andi	#0x0f
      0001D4 02 FA                  351 	stax	2(sp)
      0001D6 00 80                  352 	ldi	#0x00
      0001D8 03 FA                  353 	stax	3(sp)
      0001DA 02 F2                  354 	ldax	2(sp)
      0001DC 08 A0                  355 	ldc	#0
      0001DE 6C 90                  356 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  357 	stax	2(sp)
      0001E2 03 F2                  358 	ldax	3(sp)
      0001E4 81 90                  359 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  360 	stax	3(sp)
      0001E8 02 CC                  361 	ldxx	2(sp)
      0001EA 18 A0                  362 	txau
      0001EC 47 A0                  363 	btst	7
      0001EE 04 B8                  364 	bz	00107$
      0001F0 00 F0                  365 	ldax	0(ix)
      0001F2 01 FA                  366 	stax	1(sp)
      0001F4 08 B0                  367 	br	00108$
      0001F6                        368 00107$:
      0001F6 18 A0                  369 	txau
      0001F8 7F D4                  370 	andi	#0x7f
      0001FA A1 A1                  371 	addaxu
      0001FC 10 A0                  372 	txa
      0001FE A0 A1                  373 	addax
      000200 80 8A                  374 	call	ix
      000202 01 FA                  375 	stax	1(sp)
      000204                        376 00108$:
      000204 01 F2                  377 	ldax	1(sp)
      000206 80 A0                  378 	push	a
      000208 40 00                  379 	jal	_putc
      00020A 01 94                  380 	ads	#1
                                    381 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  382 	ldax	6(sp)
      00020E 0F D4                  383 	andi	#0x0f
      000210 02 FA                  384 	stax	2(sp)
      000212 00 80                  385 	ldi	#0x00
      000214 03 FA                  386 	stax	3(sp)
      000216 02 F2                  387 	ldax	2(sp)
      000218 08 A0                  388 	ldc	#0
      00021A 6C 90                  389 	adc	#<(___str_0 + 0)
      00021C 02 FA                  390 	stax	2(sp)
      00021E 03 F2                  391 	ldax	3(sp)
      000220 81 90                  392 	adc	#>(___str_0 + 0)
      000222 03 FA                  393 	stax	3(sp)
      000224 02 CC                  394 	ldxx	2(sp)
      000226 18 A0                  395 	txau
      000228 47 A0                  396 	btst	7
      00022A 04 B8                  397 	bz	00109$
      00022C 00 F0                  398 	ldax	0(ix)
      00022E 01 FA                  399 	stax	1(sp)
      000230 08 B0                  400 	br	00110$
      000232                        401 00109$:
      000232 18 A0                  402 	txau
      000234 7F D4                  403 	andi	#0x7f
      000236 A1 A1                  404 	addaxu
      000238 10 A0                  405 	txa
      00023A A0 A1                  406 	addax
      00023C 80 8A                  407 	call	ix
      00023E 01 FA                  408 	stax	1(sp)
      000240                        409 00110$:
      000240 01 F2                  410 	ldax	1(sp)
      000242 80 A0                  411 	push	a
      000244 40 00                  412 	jal	_putc
      000246 01 94                  413 	ads	#1
      000248                        414 00101$:
                                    415 ;	harness.h: 35: }
      000248 03 94                  416 	ads	#3
      00024A 64 A1                  417 	lra
      00024C 00 8A                  418 	ret
                                    419 ;	test_negate.c: 2: int neg16(int x) { return -x; }
                                    420 ;	---------------------------------
                                    421 ;	 Function neg16
                                    422 ;	---------------------------------
      00024E                        423 _neg16:
      00024E FE 97                  424 	ads	#-2
      000250 00 80                  425 	ldi	#0x00
      000252 05 CA                  426 	sub	5(sp)
      000254 01 FA                  427 	stax	1(sp)
      000256 78 A1                  428 	savec
      000258 00 80                  429 	ldi	#0x00
      00025A 7C A1                  430 	restc
      00025C 06 CA                  431 	sub	6(sp)
      00025E 02 FA                  432 	stax	2(sp)
      000260 01 F2                  433 	ldax	1(sp)
      000262 03 FA                  434 	stax	3(sp)
      000264 02 F2                  435 	ldax	2(sp)
      000266 04 FA                  436 	stax	4(sp)
      000268                        437 00101$:
      000268 02 94                  438 	ads	#2
      00026A 00 8A                  439 	ret
                                    440 ;	test_negate.c: 3: long neg32(long x) { return -x; }
                                    441 ;	---------------------------------
                                    442 ;	 Function neg32
                                    443 ;	---------------------------------
      00026C                        444 _neg32:
      00026C FC 97                  445 	ads	#-4
      00026E 00 80                  446 	ldi	#0x00
      000270 09 CA                  447 	sub	9(sp)
      000272 01 FA                  448 	stax	1(sp)
      000274 78 A1                  449 	savec
      000276 00 80                  450 	ldi	#0x00
      000278 7C A1                  451 	restc
      00027A 0A CA                  452 	sub	10(sp)
      00027C 02 FA                  453 	stax	2(sp)
      00027E 78 A1                  454 	savec
      000280 00 80                  455 	ldi	#0x00
      000282 7C A1                  456 	restc
      000284 0B CA                  457 	sub	11(sp)
      000286 03 FA                  458 	stax	3(sp)
      000288 78 A1                  459 	savec
      00028A 00 80                  460 	ldi	#0x00
      00028C 7C A1                  461 	restc
      00028E 0C CA                  462 	sub	12(sp)
      000290 04 FA                  463 	stax	4(sp)
      000292 01 F2                  464 	ldax	1(sp)
      000294 05 FA                  465 	stax	5(sp)
      000296 02 F2                  466 	ldax	2(sp)
      000298 06 FA                  467 	stax	6(sp)
      00029A 03 F2                  468 	ldax	3(sp)
      00029C 07 FA                  469 	stax	7(sp)
      00029E 04 F2                  470 	ldax	4(sp)
      0002A0 08 FA                  471 	stax	8(sp)
      0002A2                        472 00101$:
      0002A2 04 94                  473 	ads	#4
      0002A4 00 8A                  474 	ret
                                    475 ;	test_negate.c: 4: signed char neg8(signed char x) { return -x; }
                                    476 ;	---------------------------------
                                    477 ;	 Function neg8
                                    478 ;	---------------------------------
      0002A6                        479 _neg8:
      0002A6 FF 97                  480 	ads	#-1
      0002A8 00 80                  481 	ldi	#0x00
      0002AA 02 CA                  482 	sub	2(sp)
      0002AC 01 FA                  483 	stax	1(sp)
      0002AE                        484 00101$:
      0002AE 01 94                  485 	ads	#1
      0002B0 00 8A                  486 	ret
                                    487 ;	test_negate.c: 5: unsigned long ul(unsigned long x) { return x < 10 ? x : x - 7; }
                                    488 ;	---------------------------------
                                    489 ;	 Function ul
                                    490 ;	---------------------------------
      0002B2                        491 _ul:
      0002B2 FC 97                  492 	ads	#-4
      0002B4 00 80                  493 	ldi	#0x00
      0002B6 0C EA                  494 	cmp	12(sp)
      0002B8 04 A2                  495 	if	gt
      0002BA 14 B0                  496 	br.p	00111$
      0002BC 05 A2                  497 	if	lt
      0002BE 11 B0                  498 	br.p	00112$
      0002C0 00 80                  499 	ldi	#0x00
      0002C2 0B EA                  500 	cmp	11(sp)
      0002C4 04 A2                  501 	if	gt
      0002C6 0E B0                  502 	br.p	00111$
      0002C8 05 A2                  503 	if	lt
      0002CA 0B B0                  504 	br.p	00112$
      0002CC 00 80                  505 	ldi	#0x00
      0002CE 0A EA                  506 	cmp	10(sp)
      0002D0 04 A2                  507 	if	gt
      0002D2 08 B0                  508 	br.p	00111$
      0002D4 05 A2                  509 	if	lt
      0002D6 05 B0                  510 	br.p	00112$
      0002D8 0A 80                  511 	ldi	#0x0a
      0002DA 09 EA                  512 	cmp	9(sp)
      0002DC 04 A2                  513 	if	gt
      0002DE 02 B0                  514 	br.p	00111$
      0002E0                        515 00112$:
      0002E0 0A B0                  516 	br	00103$
      0002E2                        517 00111$:
      0002E2 09 F2                  518 	ldax	9(sp)
      0002E4 01 FA                  519 	stax	1(sp)
      0002E6 0A F2                  520 	ldax	10(sp)
      0002E8 02 FA                  521 	stax	2(sp)
      0002EA 0B F2                  522 	ldax	11(sp)
      0002EC 03 FA                  523 	stax	3(sp)
      0002EE 0C F2                  524 	ldax	12(sp)
      0002F0 04 FA                  525 	stax	4(sp)
      0002F2 0E B0                  526 	br	00104$
      0002F4                        527 00103$:
      0002F4 09 F2                  528 	ldax	9(sp)
      0002F6 09 A0                  529 	ldc	#1
      0002F8 F8 90                  530 	adc	#0xf8
      0002FA 01 FA                  531 	stax	1(sp)
      0002FC 0A F2                  532 	ldax	10(sp)
      0002FE FF 90                  533 	adc	#0xff
      000300 02 FA                  534 	stax	2(sp)
      000302 0B F2                  535 	ldax	11(sp)
      000304 FF 90                  536 	adc	#0xff
      000306 03 FA                  537 	stax	3(sp)
      000308 0C F2                  538 	ldax	12(sp)
      00030A FF 90                  539 	adc	#0xff
      00030C 04 FA                  540 	stax	4(sp)
      00030E                        541 00104$:
      00030E 01 F2                  542 	ldax	1(sp)
      000310 05 FA                  543 	stax	5(sp)
      000312 02 F2                  544 	ldax	2(sp)
      000314 06 FA                  545 	stax	6(sp)
      000316 03 F2                  546 	ldax	3(sp)
      000318 07 FA                  547 	stax	7(sp)
      00031A 04 F2                  548 	ldax	4(sp)
      00031C 08 FA                  549 	stax	8(sp)
      00031E                        550 00101$:
      00031E 04 94                  551 	ads	#4
      000320 00 8A                  552 	ret
                                    553 ;	test_negate.c: 6: int main(void) {
                                    554 ;	---------------------------------
                                    555 ;	 Function main
                                    556 ;	---------------------------------
      000322                        557 _main:
      000322 60 A1                  558 	sra
      000324 F9 97                  559 	ads	#-7
                                    560 ;	test_negate.c: 7: CHECK(1, neg16(42) == -42);
      000326 00 80                  561 	ldi	#0x00
      000328 80 A0                  562 	push	a
      00032A 2A 80                  563 	ldi	#0x2a
      00032C 80 A0                  564 	push	a
      00032E FE 97                  565 	ads	#-2
      000330 27 01                  566 	jal	_neg16
      000332 01 F2                  567 	ldax	1(sp)
      000334 0A FA                  568 	stax	10(sp)
      000336 02 F2                  569 	ldax	2(sp)
      000338 0B FA                  570 	stax	11(sp)
      00033A 04 94                  571 	ads	#4
      00033C 06 F2                  572 	ldax	6(sp)
      00033E D6 A4                  573 	cpi	#0xd6
      000340 03 A8                  574 	bnz	00197$
      000342 07 F2                  575 	ldax	7(sp)
      000344 FF A4                  576 	cpi	#0xff
      000346                        577 00197$:
      000346 08 A8                  578 	bnz	00102$
      000348 81 80                  579 	ldi	#>(___str_1 + 0)
      00034A 80 A0                  580 	push	a
      00034C 7D 80                  581 	ldi	#<(___str_1 + 0)
      00034E 80 A0                  582 	push	a
      000350 49 00                  583 	jal	_puts
      000352 02 94                  584 	ads	#2
      000354 0A B0                  585 	br	00103$
      000356                        586 00102$:
      000356 81 80                  587 	ldi	#>(___str_2 + 0)
      000358 80 A0                  588 	push	a
      00035A 81 80                  589 	ldi	#<(___str_2 + 0)
      00035C 80 A0                  590 	push	a
      00035E 49 00                  591 	jal	_puts
      000360 02 94                  592 	ads	#2
      000362 80 A1 00 00            593 	ldx	#_fails
      000366 00 E4                  594 	inx	0(ix)
      000368                        595 00103$:
      000368 00 80                  596 	ldi	#0x00
      00036A 80 A0                  597 	push	a
      00036C 01 80                  598 	ldi	#0x01
      00036E 80 A0                  599 	push	a
      000370 6A 00                  600 	jal	_puthex
      000372 02 94                  601 	ads	#2
      000374 0A 80                  602 	ldi	#0x0a
      000376 80 A0                  603 	push	a
      000378 40 00                  604 	jal	_putc
      00037A 01 94                  605 	ads	#1
                                    606 ;	test_negate.c: 8: CHECK(2, neg16(-300) == 300);
      00037C FE 80                  607 	ldi	#0xfe
      00037E 80 A0                  608 	push	a
      000380 D4 80                  609 	ldi	#0xd4
      000382 80 A0                  610 	push	a
      000384 FE 97                  611 	ads	#-2
      000386 27 01                  612 	jal	_neg16
      000388 01 F2                  613 	ldax	1(sp)
      00038A 0A FA                  614 	stax	10(sp)
      00038C 02 F2                  615 	ldax	2(sp)
      00038E 0B FA                  616 	stax	11(sp)
      000390 04 94                  617 	ads	#4
      000392 06 F2                  618 	ldax	6(sp)
      000394 2C A4                  619 	cpi	#0x2c
      000396 03 A8                  620 	bnz	00199$
      000398 07 F2                  621 	ldax	7(sp)
      00039A 01 A4                  622 	cpi	#0x01
      00039C                        623 00199$:
      00039C 08 A8                  624 	bnz	00108$
      00039E 81 80                  625 	ldi	#>(___str_1 + 0)
      0003A0 80 A0                  626 	push	a
      0003A2 7D 80                  627 	ldi	#<(___str_1 + 0)
      0003A4 80 A0                  628 	push	a
      0003A6 49 00                  629 	jal	_puts
      0003A8 02 94                  630 	ads	#2
      0003AA 0A B0                  631 	br	00109$
      0003AC                        632 00108$:
      0003AC 81 80                  633 	ldi	#>(___str_2 + 0)
      0003AE 80 A0                  634 	push	a
      0003B0 81 80                  635 	ldi	#<(___str_2 + 0)
      0003B2 80 A0                  636 	push	a
      0003B4 49 00                  637 	jal	_puts
      0003B6 02 94                  638 	ads	#2
      0003B8 80 A1 00 00            639 	ldx	#_fails
      0003BC 00 E4                  640 	inx	0(ix)
      0003BE                        641 00109$:
      0003BE 00 80                  642 	ldi	#0x00
      0003C0 80 A0                  643 	push	a
      0003C2 02 80                  644 	ldi	#0x02
      0003C4 80 A0                  645 	push	a
      0003C6 6A 00                  646 	jal	_puthex
      0003C8 02 94                  647 	ads	#2
      0003CA 0A 80                  648 	ldi	#0x0a
      0003CC 80 A0                  649 	push	a
      0003CE 40 00                  650 	jal	_putc
      0003D0 01 94                  651 	ads	#1
                                    652 ;	test_negate.c: 9: CHECK(3, neg32(123456) == -123456);
      0003D2 00 80                  653 	ldi	#0x00
      0003D4 80 A0                  654 	push	a
      0003D6 01 80                  655 	ldi	#0x01
      0003D8 80 A0                  656 	push	a
      0003DA E2 80                  657 	ldi	#0xe2
      0003DC 80 A0                  658 	push	a
      0003DE 40 80                  659 	ldi	#0x40
      0003E0 80 A0                  660 	push	a
      0003E2 FC 97                  661 	ads	#-4
      0003E4 36 01                  662 	jal	_neg32
      0003E6 01 F2                  663 	ldax	1(sp)
      0003E8 0A FA                  664 	stax	10(sp)
      0003EA 02 F2                  665 	ldax	2(sp)
      0003EC 0B FA                  666 	stax	11(sp)
      0003EE 03 F2                  667 	ldax	3(sp)
      0003F0 0C FA                  668 	stax	12(sp)
      0003F2 04 F2                  669 	ldax	4(sp)
      0003F4 0D FA                  670 	stax	13(sp)
      0003F6 08 94                  671 	ads	#8
      0003F8 02 F2                  672 	ldax	2(sp)
      0003FA C0 A4                  673 	cpi	#0xc0
      0003FC 09 A8                  674 	bnz	00201$
      0003FE 03 F2                  675 	ldax	3(sp)
      000400 1D A4                  676 	cpi	#0x1d
      000402 06 A8                  677 	bnz	00201$
      000404 04 F2                  678 	ldax	4(sp)
      000406 FE A4                  679 	cpi	#0xfe
      000408 03 A8                  680 	bnz	00201$
      00040A 05 F2                  681 	ldax	5(sp)
      00040C FF A4                  682 	cpi	#0xff
      00040E                        683 00201$:
      00040E 08 A8                  684 	bnz	00114$
      000410 81 80                  685 	ldi	#>(___str_1 + 0)
      000412 80 A0                  686 	push	a
      000414 7D 80                  687 	ldi	#<(___str_1 + 0)
      000416 80 A0                  688 	push	a
      000418 49 00                  689 	jal	_puts
      00041A 02 94                  690 	ads	#2
      00041C 0A B0                  691 	br	00115$
      00041E                        692 00114$:
      00041E 81 80                  693 	ldi	#>(___str_2 + 0)
      000420 80 A0                  694 	push	a
      000422 81 80                  695 	ldi	#<(___str_2 + 0)
      000424 80 A0                  696 	push	a
      000426 49 00                  697 	jal	_puts
      000428 02 94                  698 	ads	#2
      00042A 80 A1 00 00            699 	ldx	#_fails
      00042E 00 E4                  700 	inx	0(ix)
      000430                        701 00115$:
      000430 00 80                  702 	ldi	#0x00
      000432 80 A0                  703 	push	a
      000434 03 80                  704 	ldi	#0x03
      000436 80 A0                  705 	push	a
      000438 6A 00                  706 	jal	_puthex
      00043A 02 94                  707 	ads	#2
      00043C 0A 80                  708 	ldi	#0x0a
      00043E 80 A0                  709 	push	a
      000440 40 00                  710 	jal	_putc
      000442 01 94                  711 	ads	#1
                                    712 ;	test_negate.c: 10: CHECK(4, neg8(5) == -5 && neg8(-128) == -128);
      000444 05 80                  713 	ldi	#0x05
      000446 80 A0                  714 	push	a
      000448 53 01                  715 	jal	_neg8
      00044A 02 FA                  716 	stax	2(sp)
      00044C 01 94                  717 	ads	#1
      00044E 01 F2                  718 	ldax	1(sp)
      000450 FB A4                  719 	cpi	#0xfb
      000452 10 A8                  720 	bnz	00120$
      000454 80 80                  721 	ldi	#0x80
      000456 80 A0                  722 	push	a
      000458 53 01                  723 	jal	_neg8
      00045A 02 FA                  724 	stax	2(sp)
      00045C 01 94                  725 	ads	#1
      00045E 01 F2                  726 	ldax	1(sp)
      000460 80 A4                  727 	cpi	#0x80
      000462 08 A8                  728 	bnz	00120$
      000464 81 80                  729 	ldi	#>(___str_1 + 0)
      000466 80 A0                  730 	push	a
      000468 7D 80                  731 	ldi	#<(___str_1 + 0)
      00046A 80 A0                  732 	push	a
      00046C 49 00                  733 	jal	_puts
      00046E 02 94                  734 	ads	#2
      000470 0A B0                  735 	br	00121$
      000472                        736 00120$:
      000472 81 80                  737 	ldi	#>(___str_2 + 0)
      000474 80 A0                  738 	push	a
      000476 81 80                  739 	ldi	#<(___str_2 + 0)
      000478 80 A0                  740 	push	a
      00047A 49 00                  741 	jal	_puts
      00047C 02 94                  742 	ads	#2
      00047E 80 A1 00 00            743 	ldx	#_fails
      000482 00 E4                  744 	inx	0(ix)
      000484                        745 00121$:
      000484 00 80                  746 	ldi	#0x00
      000486 80 A0                  747 	push	a
      000488 04 80                  748 	ldi	#0x04
      00048A 80 A0                  749 	push	a
      00048C 6A 00                  750 	jal	_puthex
      00048E 02 94                  751 	ads	#2
      000490 0A 80                  752 	ldi	#0x0a
      000492 80 A0                  753 	push	a
      000494 40 00                  754 	jal	_putc
      000496 01 94                  755 	ads	#1
                                    756 ;	test_negate.c: 11: CHECK(5, neg16(0) == 0 && neg16(256) == -256);
      000498 00 80                  757 	ldi	#0x00
      00049A 80 A0                  758 	push	a
      00049C 00 80                  759 	ldi	#0x00
      00049E 80 A0                  760 	push	a
      0004A0 FE 97                  761 	ads	#-2
      0004A2 27 01                  762 	jal	_neg16
      0004A4 01 F2                  763 	ldax	1(sp)
      0004A6 0A FA                  764 	stax	10(sp)
      0004A8 02 F2                  765 	ldax	2(sp)
      0004AA 0B FA                  766 	stax	11(sp)
      0004AC 04 94                  767 	ads	#4
      0004AE 06 F2                  768 	ldax	6(sp)
      0004B0 07 DA                  769 	or	7(sp)
      0004B2 19 A8                  770 	bnz	00127$
      0004B4 01 80                  771 	ldi	#0x01
      0004B6 80 A0                  772 	push	a
      0004B8 00 80                  773 	ldi	#0x00
      0004BA 80 A0                  774 	push	a
      0004BC FE 97                  775 	ads	#-2
      0004BE 27 01                  776 	jal	_neg16
      0004C0 01 F2                  777 	ldax	1(sp)
      0004C2 0A FA                  778 	stax	10(sp)
      0004C4 02 F2                  779 	ldax	2(sp)
      0004C6 0B FA                  780 	stax	11(sp)
      0004C8 04 94                  781 	ads	#4
      0004CA 06 F2                  782 	ldax	6(sp)
      0004CC 00 A4                  783 	cpi	#0x00
      0004CE 03 A8                  784 	bnz	00207$
      0004D0 07 F2                  785 	ldax	7(sp)
      0004D2 FF A4                  786 	cpi	#0xff
      0004D4                        787 00207$:
      0004D4 08 A8                  788 	bnz	00127$
      0004D6 81 80                  789 	ldi	#>(___str_1 + 0)
      0004D8 80 A0                  790 	push	a
      0004DA 7D 80                  791 	ldi	#<(___str_1 + 0)
      0004DC 80 A0                  792 	push	a
      0004DE 49 00                  793 	jal	_puts
      0004E0 02 94                  794 	ads	#2
      0004E2 0A B0                  795 	br	00128$
      0004E4                        796 00127$:
      0004E4 81 80                  797 	ldi	#>(___str_2 + 0)
      0004E6 80 A0                  798 	push	a
      0004E8 81 80                  799 	ldi	#<(___str_2 + 0)
      0004EA 80 A0                  800 	push	a
      0004EC 49 00                  801 	jal	_puts
      0004EE 02 94                  802 	ads	#2
      0004F0 80 A1 00 00            803 	ldx	#_fails
      0004F4 00 E4                  804 	inx	0(ix)
      0004F6                        805 00128$:
      0004F6 00 80                  806 	ldi	#0x00
      0004F8 80 A0                  807 	push	a
      0004FA 05 80                  808 	ldi	#0x05
      0004FC 80 A0                  809 	push	a
      0004FE 6A 00                  810 	jal	_puthex
      000500 02 94                  811 	ads	#2
      000502 0A 80                  812 	ldi	#0x0a
      000504 80 A0                  813 	push	a
      000506 40 00                  814 	jal	_putc
      000508 01 94                  815 	ads	#1
                                    816 ;	test_negate.c: 12: CHECK(6, ul(100000) == 99993);
      00050A 00 80                  817 	ldi	#0x00
      00050C 80 A0                  818 	push	a
      00050E 01 80                  819 	ldi	#0x01
      000510 80 A0                  820 	push	a
      000512 86 80                  821 	ldi	#0x86
      000514 80 A0                  822 	push	a
      000516 A0 80                  823 	ldi	#0xa0
      000518 80 A0                  824 	push	a
      00051A FC 97                  825 	ads	#-4
      00051C 59 01                  826 	jal	_ul
      00051E 01 F2                  827 	ldax	1(sp)
      000520 0A FA                  828 	stax	10(sp)
      000522 02 F2                  829 	ldax	2(sp)
      000524 0B FA                  830 	stax	11(sp)
      000526 03 F2                  831 	ldax	3(sp)
      000528 0C FA                  832 	stax	12(sp)
      00052A 04 F2                  833 	ldax	4(sp)
      00052C 0D FA                  834 	stax	13(sp)
      00052E 08 94                  835 	ads	#8
      000530 02 F2                  836 	ldax	2(sp)
      000532 99 A4                  837 	cpi	#0x99
      000534 09 A8                  838 	bnz	00209$
      000536 03 F2                  839 	ldax	3(sp)
      000538 86 A4                  840 	cpi	#0x86
      00053A 06 A8                  841 	bnz	00209$
      00053C 04 F2                  842 	ldax	4(sp)
      00053E 01 A4                  843 	cpi	#0x01
      000540 03 A8                  844 	bnz	00209$
      000542 05 F2                  845 	ldax	5(sp)
      000544 00 A4                  846 	cpi	#0x00
      000546                        847 00209$:
      000546 08 A8                  848 	bnz	00134$
      000548 81 80                  849 	ldi	#>(___str_1 + 0)
      00054A 80 A0                  850 	push	a
      00054C 7D 80                  851 	ldi	#<(___str_1 + 0)
      00054E 80 A0                  852 	push	a
      000550 49 00                  853 	jal	_puts
      000552 02 94                  854 	ads	#2
      000554 0A B0                  855 	br	00135$
      000556                        856 00134$:
      000556 81 80                  857 	ldi	#>(___str_2 + 0)
      000558 80 A0                  858 	push	a
      00055A 81 80                  859 	ldi	#<(___str_2 + 0)
      00055C 80 A0                  860 	push	a
      00055E 49 00                  861 	jal	_puts
      000560 02 94                  862 	ads	#2
      000562 80 A1 00 00            863 	ldx	#_fails
      000566 00 E4                  864 	inx	0(ix)
      000568                        865 00135$:
      000568 00 80                  866 	ldi	#0x00
      00056A 80 A0                  867 	push	a
      00056C 06 80                  868 	ldi	#0x06
      00056E 80 A0                  869 	push	a
      000570 6A 00                  870 	jal	_puthex
      000572 02 94                  871 	ads	#2
      000574 0A 80                  872 	ldi	#0x0a
      000576 80 A0                  873 	push	a
      000578 40 00                  874 	jal	_putc
      00057A 01 94                  875 	ads	#1
                                    876 ;	test_negate.c: 13: DONE();
      00057C 00 F4                  877 	lda	_fails
      00057E 06 B8                  878 	bz	00141$
      000580 87 80                  879 	ldi	#<(___str_3 + 0)
      000582 06 FA                  880 	stax	6(sp)
      000584 81 80                  881 	ldi	#>(___str_3 + 0)
      000586 07 FA                  882 	stax	7(sp)
      000588 05 B0                  883 	br	00142$
      00058A                        884 00141$:
      00058A 94 80                  885 	ldi	#<(___str_4 + 0)
      00058C 06 FA                  886 	stax	6(sp)
      00058E 81 80                  887 	ldi	#>(___str_4 + 0)
      000590 07 FA                  888 	stax	7(sp)
      000592                        889 00142$:
      000592 07 F2                  890 	ldax	7(sp)
      000594 80 A0                  891 	push	a
      000596 07 F2                  892 	ldax	7(sp)
      000598 80 A0                  893 	push	a
      00059A 49 00                  894 	jal	_puts
      00059C 02 94                  895 	ads	#2
                                    896 ;	test_negate.c: 14: return 0;
      00059E 00 80                  897 	ldi	#0x00
      0005A0 0A FA                  898 	stax	10(sp)
      0005A2 0B FA                  899 	stax	11(sp)
      0005A4                        900 00139$:
                                    901 ;	test_negate.c: 15: }
      0005A4 07 94                  902 	ads	#7
      0005A6 64 A1                  903 	lra
      0005A8 00 8A                  904 	ret
                                    905 	.area CODE (CODE)
                                    906 	.area CONST (CODE,CDATA)
                                    907 	.area CONST (CODE,CDATA)
      0005B0                        908 ___str_0:
      0005B0 30 80 00 8A 31 80 00   909 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      0005F0 00 80 00 8A            910 	.db 0x00
                                    911 	.area CODE (CODE)
                                    912 	.area CONST (CODE,CDATA)
      0005F4                        913 ___str_1:
      0005F4 6F 80 00 8A 6B 80 00   914 	.ascii "ok "
             8A 20 80 00 8A
      000600 00 80 00 8A            915 	.db 0x00
                                    916 	.area CODE (CODE)
                                    917 	.area CONST (CODE,CDATA)
      000604                        918 ___str_2:
      000604 46 80 00 8A 41 80 00   919 	.ascii "FAIL "
             8A 49 80 00 8A 4C 80
             00 8A 20 80 00 8A
      000618 00 80 00 8A            920 	.db 0x00
                                    921 	.area CODE (CODE)
                                    922 	.area CONST (CODE,CDATA)
      00061C                        923 ___str_3:
      00061C 53 80 00 8A 4F 80 00   924 	.ascii "SOME FAILED"
             8A 4D 80 00 8A 45 80
             00 8A 20 80 00 8A 46
             80 00 8A 41 80 00 8A
             49 80 00 8A 4C 80 00
             8A 45 80 00 8A 44 80
             00 8A
      000648 0A 80 00 8A            925 	.db 0x0a
      00064C 00 80 00 8A            926 	.db 0x00
                                    927 	.area CODE (CODE)
                                    928 	.area CONST (CODE,CDATA)
      000650                        929 ___str_4:
      000650 41 80 00 8A 4C 80 00   930 	.ascii "ALL PASSED"
             8A 4C 80 00 8A 20 80
             00 8A 50 80 00 8A 41
             80 00 8A 53 80 00 8A
             53 80 00 8A 45 80 00
             8A 44 80 00 8A
      000678 0A 80 00 8A            931 	.db 0x0a
      00067C 00 80 00 8A            932 	.db 0x00
                                    933 	.area CODE (CODE)
                                    934 	.area INITIALIZER (CODE,CDATA)
                                    935 	.area CABS (ABS,CODE,CDATA)
