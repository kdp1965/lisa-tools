                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module hello
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
                                     16 	.globl _msg
                                     17 	.globl _main
                                     18 	.globl _UART_STATUS
                                     19 	.globl _UART_TX
                                     20 	.globl _counter
                                     21 	.globl _buf
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
      000001                         35 _buf::
      000001                         36 	.ds 8
      000009                         37 _counter::
      000009                         38 	.ds 2
                                     39 ;--------------------------------------------------------
                                     40 ; ram data
                                     41 ;--------------------------------------------------------
                                     42 	.area INITIALIZED
                                     43 ;--------------------------------------------------------
                                     44 ; overlayable items in ram
                                     45 ;--------------------------------------------------------
                                     46 ;--------------------------------------------------------
                                     47 ; Stack segment in internal ram
                                     48 ;--------------------------------------------------------
                                     49 	.area SSEG
      00000B                         50 __start__stack:
      00000B                         51 	.ds	1
                                     52 
                                     53 ;--------------------------------------------------------
                                     54 ; absolute external ram data
                                     55 ;--------------------------------------------------------
                                     56 	.area DABS (ABS)
                                     57 ;--------------------------------------------------------
                                     58 ; interrupt vector
                                     59 ;--------------------------------------------------------
                                     60 	.area HOME (CODE)
      000000                         61 __interrupt_vect:
      000000 0C 00                   62 	jal	__sdcc_gsinit_startup
      000002 40 8B                   63 	rets
      000004 40 8B                   64 	rets
      000006 40 8B                   65 	rets
      000008 40 8B                   66 	rets
      00000A 40 8B                   67 	rets
      00000C 40 8B                   68 	rets
      00000E 40 8B                   69 	rets
      000010 40 8B                   70 	rets
      000012 40 8B                   71 	rets
                                     72 ;--------------------------------------------------------
                                     73 ; global & static initialisations
                                     74 ;--------------------------------------------------------
                                     75 	.area HOME (CODE)
                                     76 	.area GSINIT (CODE)
                                     77 	.area GSFINAL (CODE)
                                     78 	.area GSINIT (CODE)
                                     79 	.area GSINIT (CODE)
      000018                         80 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             81 	ldx	#0x007f
      00001C C8 8A                   82 	xchg	sp
      00001E 41 A1                   83 	amode	1
      000020 75 01                   84 	jal	___sdcc_external_startup
      000022 00 A4                   85 	cpi	#0
      000024 01 A2                   86 	if	ne
      000026 0A 00                   87 	jal	__sdcc_program_startup
      000028 00 80                   88 	ldi	#>l_DATA
      00002A 80 A0                   89 	push	a
      00002C 0B 80                   90 	ldi	#<l_DATA
      00002E 80 A0                   91 	push	a
      000030 80 A1 00 00             92 	ldx	#s_DATA
      000034                         93 00001$:
      000034 01 F2                   94 	ldax	1(sp)
      000036 02 DA                   95 	or	2(sp)
      000038 08 B8                   96 	bz	00002$
      00003A 00 80                   97 	ldi	#0
      00003C 00 F8                   98 	stax	0(ix)
      00003E 01 98                   99 	adx	#1
      000040 01 9E                  100 	dcx	1(sp)
      000042 03 A2                  101 	if	c
      000044 02 9E                  102 	dcx	2(sp)
      000046 F7 B7                  103 	br	00001$
      000048                        104 00002$:
      000048 00 80                  105 	ldi	#>s_INITIALIZED
      00004A 80 A0                  106 	push	a
      00004C 0B 80                  107 	ldi	#<s_INITIALIZED
      00004E 80 A0                  108 	push	a
      000050 00 80                  109 	ldi	#>l_INITIALIZED
      000052 04 FA                  110 	stax	4(sp)
      000054 00 80                  111 	ldi	#<l_INITIALIZED
      000056 03 FA                  112 	stax	3(sp)
      000058 80 A1 D8 81            113 	ldx	#s_INITIALIZER
      00005C                        114 00003$:
      00005C 03 F2                  115 	ldax	3(sp)
      00005E 04 DA                  116 	or	4(sp)
      000060 0E B8                  117 	bz	00004$
      000062 80 8A                  118 	call	ix
      000064 01 98                  119 	adx	#1
      000066 68 A1                  120 	push	ix
      000068 03 CC                  121 	ldxx	3(sp)
      00006A 00 F8                  122 	stax	0(ix)
      00006C 03 E6                  123 	inx	3(sp)
      00006E 03 A2                  124 	if	c
      000070 04 E6                  125 	inx	4(sp)
      000072 6C A1                  126 	pop	ix
      000074 03 9E                  127 	dcx	3(sp)
      000076 03 A2                  128 	if	c
      000078 04 9E                  129 	dcx	4(sp)
      00007A F1 B7                  130 	br	00003$
      00007C                        131 00004$:
      00007C 04 94                  132 	ads	#4
                                    133 	.area GSFINAL (CODE)
      00007E 0A 00                  134 	jal	__sdcc_program_startup
                                    135 ;--------------------------------------------------------
                                    136 ; Home
                                    137 ;--------------------------------------------------------
                                    138 	.area HOME (CODE)
                                    139 	.area HOME (CODE)
      000014                        140 __sdcc_program_startup:
      000014 27 01                  141 	jal	_main
      000016                        142 00001$:
      000016 00 B0                  143 	br	00001$
                                    144 ;	return from main will return to caller
                                    145 ;--------------------------------------------------------
                                    146 ; code
                                    147 ;--------------------------------------------------------
                                    148 	.area CODE (CODE)
                                    149 ;	harness.h: 15: static void putc(char c)
                                    150 ;	---------------------------------
                                    151 ;	 Function putc
                                    152 ;	---------------------------------
      000080                        153 _putc:
      000080 FF 97                  154 	ads	#-1
                                    155 ;	harness.h: 17: while (!(UART_STATUS & 2))
      000082                        156 00101$:
      000082 11 F6                  157 	lda	_UART_STATUS
      000084 01 FA                  158 	stax	1(sp)
      000086 02 D4                  159 	andi	#0x02
      000088 FD BF                  160 	bz	00101$
                                    161 ;	harness.h: 19: UART_TX = c;
      00008A 02 F2                  162 	ldax	2(sp)
      00008C 10 FE                  163 	sta	_UART_TX
      00008E                        164 00104$:
                                    165 ;	harness.h: 20: }
      00008E 01 94                  166 	ads	#1
      000090 00 8A                  167 	ret
                                    168 ;	harness.h: 22: static void puts(const char *s)
                                    169 ;	---------------------------------
                                    170 ;	 Function puts
                                    171 ;	---------------------------------
      000092                        172 _puts:
      000092 60 A1                  173 	sra
      000094 FD 97                  174 	ads	#-3
                                    175 ;	harness.h: 24: while (*s)
      000096 06 F2                  176 	ldax	6(sp)
      000098 02 FA                  177 	stax	2(sp)
      00009A 07 F2                  178 	ldax	7(sp)
      00009C 03 FA                  179 	stax	3(sp)
      00009E                        180 00101$:
      00009E 02 CC                  181 	ldxx	2(sp)
      0000A0 18 A0                  182 	txau
      0000A2 47 A0                  183 	btst	7
      0000A4 04 B8                  184 	bz	00119$
      0000A6 00 F0                  185 	ldax	0(ix)
      0000A8 01 FA                  186 	stax	1(sp)
      0000AA 08 B0                  187 	br	00120$
      0000AC                        188 00119$:
      0000AC 18 A0                  189 	txau
      0000AE 7F D4                  190 	andi	#0x7f
      0000B0 A1 A1                  191 	addaxu
      0000B2 10 A0                  192 	txa
      0000B4 A0 A1                  193 	addax
      0000B6 80 8A                  194 	call	ix
      0000B8 01 FA                  195 	stax	1(sp)
      0000BA                        196 00120$:
      0000BA 01 F2                  197 	ldax	1(sp)
      0000BC 09 B8                  198 	bz	00104$
                                    199 ;	harness.h: 25: putc(*s++);
      0000BE 02 E6                  200 	inx	2(sp)
      0000C0 03 A2                  201 	if	c
      0000C2 03 E6                  202 	inx.p	3(sp)
      0000C4 01 F2                  203 	ldax	1(sp)
      0000C6 80 A0                  204 	push	a
      0000C8 40 00                  205 	jal	_putc
      0000CA 01 94                  206 	ads	#1
      0000CC E9 B7                  207 	br	00101$
      0000CE                        208 00104$:
                                    209 ;	harness.h: 26: }
      0000CE 03 94                  210 	ads	#3
      0000D0 64 A1                  211 	lra
      0000D2 00 8A                  212 	ret
                                    213 ;	harness.h: 28: static void puthex(unsigned int v)
                                    214 ;	---------------------------------
                                    215 ;	 Function puthex
                                    216 ;	---------------------------------
      0000D4                        217 _puthex:
      0000D4 60 A1                  218 	sra
      0000D6 FD 97                  219 	ads	#-3
                                    220 ;	harness.h: 30: const char *h = "0123456789abcdef";
                                    221 ;	harness.h: 31: putc(h[(v >> 12) & 15]);
      0000D8 07 F2                  222 	ldax	7(sp)
      0000DA 02 FA                  223 	stax	2(sp)
      0000DC 00 80                  224 	ldi	#0x00
      0000DE 03 FA                  225 	stax	3(sp)
      0000E0 08 A0                  226 	ldc	#0
      0000E2 04 A0                  227 	shr
      0000E4 03 FA                  228 	stax	3(sp)
      0000E6 02 F2                  229 	ldax	2(sp)
      0000E8 04 A0                  230 	shr
      0000EA 02 FA                  231 	stax	2(sp)
      0000EC 03 F2                  232 	ldax	3(sp)
      0000EE 08 A0                  233 	ldc	#0
      0000F0 04 A0                  234 	shr
      0000F2 03 FA                  235 	stax	3(sp)
      0000F4 02 F2                  236 	ldax	2(sp)
      0000F6 04 A0                  237 	shr
      0000F8 02 FA                  238 	stax	2(sp)
      0000FA 03 F2                  239 	ldax	3(sp)
      0000FC 08 A0                  240 	ldc	#0
      0000FE 04 A0                  241 	shr
      000100 03 FA                  242 	stax	3(sp)
      000102 02 F2                  243 	ldax	2(sp)
      000104 04 A0                  244 	shr
      000106 02 FA                  245 	stax	2(sp)
      000108 03 F2                  246 	ldax	3(sp)
      00010A 08 A0                  247 	ldc	#0
      00010C 04 A0                  248 	shr
      00010E 03 FA                  249 	stax	3(sp)
      000110 02 F2                  250 	ldax	2(sp)
      000112 04 A0                  251 	shr
      000114 02 FA                  252 	stax	2(sp)
      000116 0F D4                  253 	andi	#0x0f
      000118 02 FA                  254 	stax	2(sp)
      00011A 00 80                  255 	ldi	#0x00
      00011C 03 FA                  256 	stax	3(sp)
      00011E 02 F2                  257 	ldax	2(sp)
      000120 08 A0                  258 	ldc	#0
      000122 BC 90                  259 	adc	#<(___str_0 + 0)
      000124 02 FA                  260 	stax	2(sp)
      000126 03 F2                  261 	ldax	3(sp)
      000128 80 90                  262 	adc	#>(___str_0 + 0)
      00012A 03 FA                  263 	stax	3(sp)
      00012C 02 CC                  264 	ldxx	2(sp)
      00012E 18 A0                  265 	txau
      000130 47 A0                  266 	btst	7
      000132 04 B8                  267 	bz	00103$
      000134 00 F0                  268 	ldax	0(ix)
      000136 01 FA                  269 	stax	1(sp)
      000138 08 B0                  270 	br	00104$
      00013A                        271 00103$:
      00013A 18 A0                  272 	txau
      00013C 7F D4                  273 	andi	#0x7f
      00013E A1 A1                  274 	addaxu
      000140 10 A0                  275 	txa
      000142 A0 A1                  276 	addax
      000144 80 8A                  277 	call	ix
      000146 01 FA                  278 	stax	1(sp)
      000148                        279 00104$:
      000148 01 F2                  280 	ldax	1(sp)
      00014A 80 A0                  281 	push	a
      00014C 40 00                  282 	jal	_putc
      00014E 01 94                  283 	ads	#1
                                    284 ;	harness.h: 32: putc(h[(v >> 8) & 15]);
      000150 07 F2                  285 	ldax	7(sp)
      000152 02 FA                  286 	stax	2(sp)
      000154 00 80                  287 	ldi	#0x00
      000156 03 FA                  288 	stax	3(sp)
      000158 02 F2                  289 	ldax	2(sp)
      00015A 0F D4                  290 	andi	#0x0f
      00015C 02 FA                  291 	stax	2(sp)
      00015E 00 80                  292 	ldi	#0x00
      000160 03 FA                  293 	stax	3(sp)
      000162 02 F2                  294 	ldax	2(sp)
      000164 08 A0                  295 	ldc	#0
      000166 BC 90                  296 	adc	#<(___str_0 + 0)
      000168 02 FA                  297 	stax	2(sp)
      00016A 03 F2                  298 	ldax	3(sp)
      00016C 80 90                  299 	adc	#>(___str_0 + 0)
      00016E 03 FA                  300 	stax	3(sp)
      000170 02 CC                  301 	ldxx	2(sp)
      000172 18 A0                  302 	txau
      000174 47 A0                  303 	btst	7
      000176 04 B8                  304 	bz	00105$
      000178 00 F0                  305 	ldax	0(ix)
      00017A 01 FA                  306 	stax	1(sp)
      00017C 08 B0                  307 	br	00106$
      00017E                        308 00105$:
      00017E 18 A0                  309 	txau
      000180 7F D4                  310 	andi	#0x7f
      000182 A1 A1                  311 	addaxu
      000184 10 A0                  312 	txa
      000186 A0 A1                  313 	addax
      000188 80 8A                  314 	call	ix
      00018A 01 FA                  315 	stax	1(sp)
      00018C                        316 00106$:
      00018C 01 F2                  317 	ldax	1(sp)
      00018E 80 A0                  318 	push	a
      000190 40 00                  319 	jal	_putc
      000192 01 94                  320 	ads	#1
                                    321 ;	harness.h: 33: putc(h[(v >> 4) & 15]);
      000194 06 F2                  322 	ldax	6(sp)
      000196 02 FA                  323 	stax	2(sp)
      000198 07 F2                  324 	ldax	7(sp)
      00019A 03 FA                  325 	stax	3(sp)
      00019C 08 A0                  326 	ldc	#0
      00019E 04 A0                  327 	shr
      0001A0 03 FA                  328 	stax	3(sp)
      0001A2 02 F2                  329 	ldax	2(sp)
      0001A4 04 A0                  330 	shr
      0001A6 02 FA                  331 	stax	2(sp)
      0001A8 03 F2                  332 	ldax	3(sp)
      0001AA 08 A0                  333 	ldc	#0
      0001AC 04 A0                  334 	shr
      0001AE 03 FA                  335 	stax	3(sp)
      0001B0 02 F2                  336 	ldax	2(sp)
      0001B2 04 A0                  337 	shr
      0001B4 02 FA                  338 	stax	2(sp)
      0001B6 03 F2                  339 	ldax	3(sp)
      0001B8 08 A0                  340 	ldc	#0
      0001BA 04 A0                  341 	shr
      0001BC 03 FA                  342 	stax	3(sp)
      0001BE 02 F2                  343 	ldax	2(sp)
      0001C0 04 A0                  344 	shr
      0001C2 02 FA                  345 	stax	2(sp)
      0001C4 03 F2                  346 	ldax	3(sp)
      0001C6 08 A0                  347 	ldc	#0
      0001C8 04 A0                  348 	shr
      0001CA 03 FA                  349 	stax	3(sp)
      0001CC 02 F2                  350 	ldax	2(sp)
      0001CE 04 A0                  351 	shr
      0001D0 02 FA                  352 	stax	2(sp)
      0001D2 0F D4                  353 	andi	#0x0f
      0001D4 02 FA                  354 	stax	2(sp)
      0001D6 00 80                  355 	ldi	#0x00
      0001D8 03 FA                  356 	stax	3(sp)
      0001DA 02 F2                  357 	ldax	2(sp)
      0001DC 08 A0                  358 	ldc	#0
      0001DE BC 90                  359 	adc	#<(___str_0 + 0)
      0001E0 02 FA                  360 	stax	2(sp)
      0001E2 03 F2                  361 	ldax	3(sp)
      0001E4 80 90                  362 	adc	#>(___str_0 + 0)
      0001E6 03 FA                  363 	stax	3(sp)
      0001E8 02 CC                  364 	ldxx	2(sp)
      0001EA 18 A0                  365 	txau
      0001EC 47 A0                  366 	btst	7
      0001EE 04 B8                  367 	bz	00107$
      0001F0 00 F0                  368 	ldax	0(ix)
      0001F2 01 FA                  369 	stax	1(sp)
      0001F4 08 B0                  370 	br	00108$
      0001F6                        371 00107$:
      0001F6 18 A0                  372 	txau
      0001F8 7F D4                  373 	andi	#0x7f
      0001FA A1 A1                  374 	addaxu
      0001FC 10 A0                  375 	txa
      0001FE A0 A1                  376 	addax
      000200 80 8A                  377 	call	ix
      000202 01 FA                  378 	stax	1(sp)
      000204                        379 00108$:
      000204 01 F2                  380 	ldax	1(sp)
      000206 80 A0                  381 	push	a
      000208 40 00                  382 	jal	_putc
      00020A 01 94                  383 	ads	#1
                                    384 ;	harness.h: 34: putc(h[v & 15]);
      00020C 06 F2                  385 	ldax	6(sp)
      00020E 0F D4                  386 	andi	#0x0f
      000210 02 FA                  387 	stax	2(sp)
      000212 00 80                  388 	ldi	#0x00
      000214 03 FA                  389 	stax	3(sp)
      000216 02 F2                  390 	ldax	2(sp)
      000218 08 A0                  391 	ldc	#0
      00021A BC 90                  392 	adc	#<(___str_0 + 0)
      00021C 02 FA                  393 	stax	2(sp)
      00021E 03 F2                  394 	ldax	3(sp)
      000220 80 90                  395 	adc	#>(___str_0 + 0)
      000222 03 FA                  396 	stax	3(sp)
      000224 02 CC                  397 	ldxx	2(sp)
      000226 18 A0                  398 	txau
      000228 47 A0                  399 	btst	7
      00022A 04 B8                  400 	bz	00109$
      00022C 00 F0                  401 	ldax	0(ix)
      00022E 01 FA                  402 	stax	1(sp)
      000230 08 B0                  403 	br	00110$
      000232                        404 00109$:
      000232 18 A0                  405 	txau
      000234 7F D4                  406 	andi	#0x7f
      000236 A1 A1                  407 	addaxu
      000238 10 A0                  408 	txa
      00023A A0 A1                  409 	addax
      00023C 80 8A                  410 	call	ix
      00023E 01 FA                  411 	stax	1(sp)
      000240                        412 00110$:
      000240 01 F2                  413 	ldax	1(sp)
      000242 80 A0                  414 	push	a
      000244 40 00                  415 	jal	_putc
      000246 01 94                  416 	ads	#1
      000248                        417 00101$:
                                    418 ;	harness.h: 35: }
      000248 03 94                  419 	ads	#3
      00024A 64 A1                  420 	lra
      00024C 00 8A                  421 	ret
                                    422 ;	hello.c: 8: int main(void)
                                    423 ;	---------------------------------
                                    424 ;	 Function main
                                    425 ;	---------------------------------
      00024E                        426 _main:
      00024E 60 A1                  427 	sra
      000250 FC 97                  428 	ads	#-4
                                    429 ;	hello.c: 12: puts(msg);
      000252 80 80                  430 	ldi	#>(_msg + 0)
      000254 80 A0                  431 	push	a
      000256 CD 80                  432 	ldi	#<(_msg + 0)
      000258 80 A0                  433 	push	a
      00025A 49 00                  434 	jal	_puts
      00025C 02 94                  435 	ads	#2
                                    436 ;	hello.c: 13: for (i = 0; i < 8; i++)
      00025E 00 80                  437 	ldi	#0x00
      000260 04 FA                  438 	stax	4(sp)
      000262                        439 00103$:
                                    440 ;	hello.c: 14: buf[i] = i * 3;
      000262 04 F2                  441 	ldax	4(sp)
      000264 08 A0                  442 	ldc	#0
      000266 01 90                  443 	adc	#<(_buf + 0)
      000268 02 FA                  444 	stax	2(sp)
      00026A 00 80                  445 	ldi	#0x00
      00026C 00 90                  446 	adc	#>(_buf + 0)
      00026E 03 FA                  447 	stax	3(sp)
      000270 04 F2                  448 	ldax	4(sp)
      000272 80 A0                  449 	push	a
      000274 03 80                  450 	ldi	#0x03
      000276 01 EE                  451 	swap	1(sp)
      000278 01 C6                  452 	mul	1(sp)
      00027A 01 94                  453 	ads	#1
      00027C 01 FA                  454 	stax	1(sp)
      00027E 02 CC                  455 	ldxx	2(sp)
      000280 01 F2                  456 	ldax	1(sp)
      000282 00 F8                  457 	stax	0(ix)
                                    458 ;	hello.c: 13: for (i = 0; i < 8; i++)
      000284 04 E6                  459 	inx	4(sp)
      000286 04 F2                  460 	ldax	4(sp)
      000288 08 A4                  461 	cpi	#0x08
      00028A 05 A2                  462 	if	lt
      00028C EB B7                  463 	br.p	00103$
                                    464 ;	hello.c: 15: for (i = 0; i < 8; i++)
      00028E 00 80                  465 	ldi	#0x00
      000290 01 FA                  466 	stax	1(sp)
      000292                        467 00105$:
                                    468 ;	hello.c: 16: counter += buf[i];
      000292 01 F2                  469 	ldax	1(sp)
      000294 08 A0                  470 	ldc	#0
      000296 01 90                  471 	adc	#<(_buf + 0)
      000298 02 FA                  472 	stax	2(sp)
      00029A 00 80                  473 	ldi	#0x00
      00029C 00 90                  474 	adc	#>(_buf + 0)
      00029E 03 FA                  475 	stax	3(sp)
      0002A0 02 CC                  476 	ldxx	2(sp)
      0002A2 00 F0                  477 	ldax	0(ix)
      0002A4 04 FA                  478 	stax	4(sp)
      0002A6 09 F4                  479 	lda	_counter
      0002A8 08 A0                  480 	ldc	#0
      0002AA 04 C2                  481 	add	4(sp)
      0002AC 09 FC                  482 	sta	_counter
      0002AE 0A F4                  483 	lda	_counter+1
      0002B0 00 90                  484 	adc	#0x00
      0002B2 0A FC                  485 	sta	_counter+1
                                    486 ;	hello.c: 15: for (i = 0; i < 8; i++)
      0002B4 01 E6                  487 	inx	1(sp)
      0002B6 01 F2                  488 	ldax	1(sp)
      0002B8 08 A4                  489 	cpi	#0x08
      0002BA 05 A2                  490 	if	lt
      0002BC EB B7                  491 	br.p	00105$
                                    492 ;	hello.c: 17: puts("sum=");
      0002BE 80 80                  493 	ldi	#>(___str_1 + 0)
      0002C0 80 A0                  494 	push	a
      0002C2 E7 80                  495 	ldi	#<(___str_1 + 0)
      0002C4 80 A0                  496 	push	a
      0002C6 49 00                  497 	jal	_puts
      0002C8 02 94                  498 	ads	#2
                                    499 ;	hello.c: 18: puthex(counter);
      0002CA 0A F4                  500 	lda	_counter+1
      0002CC 80 A0                  501 	push	a
      0002CE 09 F4                  502 	lda	_counter
      0002D0 80 A0                  503 	push	a
      0002D2 6A 00                  504 	jal	_puthex
      0002D4 02 94                  505 	ads	#2
                                    506 ;	hello.c: 19: putc('\n');
      0002D6 0A 80                  507 	ldi	#0x0a
      0002D8 80 A0                  508 	push	a
      0002DA 40 00                  509 	jal	_putc
      0002DC 01 94                  510 	ads	#1
                                    511 ;	hello.c: 20: return 0;
      0002DE 00 80                  512 	ldi	#0x00
      0002E0 07 FA                  513 	stax	7(sp)
      0002E2 08 FA                  514 	stax	8(sp)
      0002E4                        515 00107$:
                                    516 ;	hello.c: 21: }
      0002E4 04 94                  517 	ads	#4
      0002E6 64 A1                  518 	lra
      0002E8 00 8A                  519 	ret
                                    520 	.area CODE (CODE)
                                    521 	.area CONST (CODE,CDATA)
                                    522 	.area CONST (CODE,CDATA)
      0002F0                        523 ___str_0:
      0002F0 30 80 00 8A 31 80 00   524 	.ascii "0123456789abcdef"
             8A 32 80 00 8A 33 80
             00 8A 34 80 00 8A 35
             80 00 8A 36 80 00 8A
             37 80 00 8A 38 80 00
             8A 39 80 00 8A 61 80
             00 8A 62 80 00 8A 63
             80 00 8A 64 80 00 8A
             65 80 00 8A 66 80 00
             8A
      000330 00 80 00 8A            525 	.db 0x00
                                    526 	.area CODE (CODE)
                                    527 	.area CONST (CODE,CDATA)
      000334                        528 _msg:
      000334 48 80 00 8A 65 80 00   529 	.ascii "Hello from LISA via SDCC"
             8A 6C 80 00 8A 6C 80
             00 8A 6F 80 00 8A 20
             80 00 8A 66 80 00 8A
             72 80 00 8A 6F 80 00
             8A 6D 80 00 8A 20 80
             00 8A 4C 80 00 8A 49
             80 00 8A 53 80 00 8A
             41 80 00 8A 20 80 00
             8A 76 80 00 8A 69 80
             00 8A 61 80 00 8A 20
             80 00 8A 53 80 00 8A
             44 80 00 8A 43 80 00
             8A 43 80 00 8A
      000394 0A 80 00 8A            530 	.db 0x0a
      000398 00 80 00 8A            531 	.db 0x00
                                    532 	.area CODE (CODE)
                                    533 	.area CONST (CODE,CDATA)
      00039C                        534 ___str_1:
      00039C 73 80 00 8A 75 80 00   535 	.ascii "sum="
             8A 6D 80 00 8A 3D 80
             00 8A
      0003AC 00 80 00 8A            536 	.db 0x00
                                    537 	.area CODE (CODE)
                                    538 	.area INITIALIZER (CODE,CDATA)
                                    539 	.area CABS (ABS,CODE,CDATA)
