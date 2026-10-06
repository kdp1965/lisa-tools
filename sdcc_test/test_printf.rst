                                      1 ;--------------------------------------------------------
                                      2 ; File Created by SDCC : free open source ISO C Compiler
                                      3 ; Version 4.5.2 #0 (Mac OS X ppc)
                                      4 ;--------------------------------------------------------
                                      5 	.module test_printf
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
                                     17 	.globl _strlen
                                     18 	.globl _strcat
                                     19 	.globl _strcpy
                                     20 	.globl _printf
                                     21 	.globl _UART_STATUS
                                     22 	.globl _UART_TX
                                     23 	.globl _name
                                     24 	.globl _putchar
                                     25 ;--------------------------------------------------------
                                     26 ; special function registers
                                     27 ;--------------------------------------------------------
                                     28 	.area RSEG (ABS)
      000000                         29 	.org 0x0000
                           000210    30 _UART_TX	=	0x0210
                           000211    31 _UART_STATUS	=	0x0211
                                     32 ;--------------------------------------------------------
                                     33 ; ram data
                                     34 ;--------------------------------------------------------
                                     35 	.area DATA
      000000                         36 _name::
      000000                         37 	.ds 16
                                     38 ;--------------------------------------------------------
                                     39 ; ram data
                                     40 ;--------------------------------------------------------
                                     41 	.area INITIALIZED
                                     42 ;--------------------------------------------------------
                                     43 ; overlayable items in ram
                                     44 ;--------------------------------------------------------
                                     45 ;--------------------------------------------------------
                                     46 ; Stack segment in internal ram
                                     47 ;--------------------------------------------------------
                                     48 	.area SSEG
      000010                         49 __start__stack:
      000010                         50 	.ds	1
                                     51 
                                     52 ;--------------------------------------------------------
                                     53 ; absolute external ram data
                                     54 ;--------------------------------------------------------
                                     55 	.area DABS (ABS)
                                     56 ;--------------------------------------------------------
                                     57 ; interrupt vector
                                     58 ;--------------------------------------------------------
                                     59 	.area HOME (CODE)
      000000                         60 __interrupt_vect:
      000000 0C 00                   61 	jal	__sdcc_gsinit_startup
      000002 40 8B                   62 	rets
      000004 40 8B                   63 	rets
      000006 40 8B                   64 	rets
      000008 40 8B                   65 	rets
      00000A 40 8B                   66 	rets
      00000C 40 8B                   67 	rets
      00000E 40 8B                   68 	rets
      000010 40 8B                   69 	rets
      000012 40 8B                   70 	rets
                                     71 ;--------------------------------------------------------
                                     72 ; global & static initialisations
                                     73 ;--------------------------------------------------------
                                     74 	.area HOME (CODE)
                                     75 	.area GSINIT (CODE)
                                     76 	.area GSFINAL (CODE)
                                     77 	.area GSINIT (CODE)
                                     78 	.area GSINIT (CODE)
      000018                         79 __sdcc_gsinit_startup::
      000018 80 A1 7F 00             80 	ldx	#0x007f
      00001C C8 8A                   81 	xchg	sp
      00001E 41 A1                   82 	amode	1
      000020 6E 01                   83 	jal	___sdcc_external_startup
      000022 00 A4                   84 	cpi	#0
      000024 01 A2                   85 	if	ne
      000026 0A 00                   86 	jal	__sdcc_program_startup
      000028 00 80                   87 	ldi	#>l_DATA
      00002A 80 A0                   88 	push	a
      00002C 10 80                   89 	ldi	#<l_DATA
      00002E 80 A0                   90 	push	a
      000030 80 A1 00 00             91 	ldx	#s_DATA
      000034                         92 00001$:
      000034 01 F2                   93 	ldax	1(sp)
      000036 02 DA                   94 	or	2(sp)
      000038 08 B8                   95 	bz	00002$
      00003A 00 80                   96 	ldi	#0
      00003C 00 F8                   97 	stax	0(ix)
      00003E 01 98                   98 	adx	#1
      000040 01 9E                   99 	dcx	1(sp)
      000042 03 A2                  100 	if	c
      000044 02 9E                  101 	dcx	2(sp)
      000046 F7 B7                  102 	br	00001$
      000048                        103 00002$:
      000048 00 80                  104 	ldi	#>s_INITIALIZED
      00004A 80 A0                  105 	push	a
      00004C 10 80                  106 	ldi	#<s_INITIALIZED
      00004E 80 A0                  107 	push	a
      000050 00 80                  108 	ldi	#>l_INITIALIZED
      000052 04 FA                  109 	stax	4(sp)
      000054 00 80                  110 	ldi	#<l_INITIALIZED
      000056 03 FA                  111 	stax	3(sp)
      000058 80 A1 F4 8A            112 	ldx	#s_INITIALIZER
      00005C                        113 00003$:
      00005C 03 F2                  114 	ldax	3(sp)
      00005E 04 DA                  115 	or	4(sp)
      000060 0E B8                  116 	bz	00004$
      000062 80 8A                  117 	call	ix
      000064 01 98                  118 	adx	#1
      000066 68 A1                  119 	push	ix
      000068 03 CC                  120 	ldxx	3(sp)
      00006A 00 F8                  121 	stax	0(ix)
      00006C 03 E6                  122 	inx	3(sp)
      00006E 03 A2                  123 	if	c
      000070 04 E6                  124 	inx	4(sp)
      000072 6C A1                  125 	pop	ix
      000074 03 9E                  126 	dcx	3(sp)
      000076 03 A2                  127 	if	c
      000078 04 9E                  128 	dcx	4(sp)
      00007A F1 B7                  129 	br	00003$
      00007C                        130 00004$:
      00007C 04 94                  131 	ads	#4
                                    132 	.area GSFINAL (CODE)
      00007E 0A 00                  133 	jal	__sdcc_program_startup
                                    134 ;--------------------------------------------------------
                                    135 ; Home
                                    136 ;--------------------------------------------------------
                                    137 	.area HOME (CODE)
                                    138 	.area HOME (CODE)
      000014                        139 __sdcc_program_startup:
      000014 4D 00                  140 	jal	_main
      000016                        141 00001$:
      000016 00 B0                  142 	br	00001$
                                    143 ;	return from main will return to caller
                                    144 ;--------------------------------------------------------
                                    145 ; code
                                    146 ;--------------------------------------------------------
                                    147 	.area CODE (CODE)
                                    148 ;	test_printf.c: 8: int putchar(int c)
                                    149 ;	---------------------------------
                                    150 ;	 Function putchar
                                    151 ;	---------------------------------
      000080                        152 _putchar:
      000080 FF 97                  153 	ads	#-1
                                    154 ;	test_printf.c: 10: while (!(UART_STATUS & 2))
      000082                        155 00101$:
      000082 11 F6                  156 	lda	_UART_STATUS
      000084 01 FA                  157 	stax	1(sp)
      000086 02 D4                  158 	andi	#0x02
      000088 FD BF                  159 	bz	00101$
                                    160 ;	test_printf.c: 12: UART_TX = c;
      00008A 04 F2                  161 	ldax	4(sp)
      00008C 10 FE                  162 	sta	_UART_TX
                                    163 ;	test_printf.c: 13: return c;
      00008E 04 F2                  164 	ldax	4(sp)
      000090 02 FA                  165 	stax	2(sp)
      000092 05 F2                  166 	ldax	5(sp)
      000094 03 FA                  167 	stax	3(sp)
      000096                        168 00104$:
                                    169 ;	test_printf.c: 14: }
      000096 01 94                  170 	ads	#1
      000098 00 8A                  171 	ret
                                    172 ;	test_printf.c: 18: int main(void)
                                    173 ;	---------------------------------
                                    174 ;	 Function main
                                    175 ;	---------------------------------
      00009A                        176 _main:
      00009A 60 A1                  177 	sra
      00009C FA 97                  178 	ads	#-6
                                    179 ;	test_printf.c: 23: strcpy(name, "LISA");
      00009E 84 80                  180 	ldi	#>(___str_0 + 0)
      0000A0 80 A0                  181 	push	a
      0000A2 F4 80                  182 	ldi	#<(___str_0 + 0)
      0000A4 80 A0                  183 	push	a
      0000A6 00 80                  184 	ldi	#>(_name + 0)
      0000A8 80 A0                  185 	push	a
      0000AA 00 80                  186 	ldi	#<(_name + 0)
      0000AC 80 A0                  187 	push	a
      0000AE FE 97                  188 	ads	#-2
      0000B0 44 01                  189 	jal	_strcpy
      0000B2 06 94                  190 	ads	#6
                                    191 ;	test_printf.c: 24: strcat(name, "!");
      0000B4 84 80                  192 	ldi	#>(___str_1 + 0)
      0000B6 80 A0                  193 	push	a
      0000B8 F9 80                  194 	ldi	#<(___str_1 + 0)
      0000BA 80 A0                  195 	push	a
      0000BC 00 80                  196 	ldi	#>(_name + 0)
      0000BE 80 A0                  197 	push	a
      0000C0 00 80                  198 	ldi	#<(_name + 0)
      0000C2 80 A0                  199 	push	a
      0000C4 FE 97                  200 	ads	#-2
      0000C6 70 01                  201 	jal	_strcat
      0000C8 06 94                  202 	ads	#6
                                    203 ;	test_printf.c: 25: printf("Hello from %s, len %u\n", name, (unsigned)strlen(name));
      0000CA 00 80                  204 	ldi	#>(_name + 0)
      0000CC 80 A0                  205 	push	a
      0000CE 00 80                  206 	ldi	#<(_name + 0)
      0000D0 80 A0                  207 	push	a
      0000D2 FE 97                  208 	ads	#-2
      0000D4 06 02                  209 	jal	_strlen
      0000D6 01 F2                  210 	ldax	1(sp)
      0000D8 09 FA                  211 	stax	9(sp)
      0000DA 02 F2                  212 	ldax	2(sp)
      0000DC 0A FA                  213 	stax	10(sp)
      0000DE 04 94                  214 	ads	#4
      0000E0 06 F2                  215 	ldax	6(sp)
      0000E2 80 A0                  216 	push	a
      0000E4 06 F2                  217 	ldax	6(sp)
      0000E6 80 A0                  218 	push	a
      0000E8 00 80                  219 	ldi	#>(_name + 0)
      0000EA 80 A0                  220 	push	a
      0000EC 00 80                  221 	ldi	#<(_name + 0)
      0000EE 80 A0                  222 	push	a
      0000F0 84 80                  223 	ldi	#>(___str_2 + 0)
      0000F2 80 A0                  224 	push	a
      0000F4 FB 80                  225 	ldi	#<(___str_2 + 0)
      0000F6 80 A0                  226 	push	a
      0000F8 FE 97                  227 	ads	#-2
      0000FA DD 01                  228 	jal	_printf
      0000FC 08 94                  229 	ads	#8
                                    230 ;	test_printf.c: 26: for (i = 0; i < 3; i++)
      0000FE 00 80                  231 	ldi	#0x00
      000100 05 FA                  232 	stax	5(sp)
      000102 06 FA                  233 	stax	6(sp)
      000104                        234 00102$:
                                    235 ;	test_printf.c: 27: printf("i=%u sq=%u hex=%04x\n", i, i * i, i * 0x111);
      000104 01 80                  236 	ldi	#0x01
      000106 80 A0                  237 	push	a
      000108 11 80                  238 	ldi	#0x11
      00010A 80 A0                  239 	push	a
      00010C 07 F2                  240 	ldax	7(sp)
      00010E 01 86                  241 	mulu	1(sp)
      000110 80 A0                  242 	push	a
      000112 08 F2                  243 	ldax	8(sp)
      000114 03 C6                  244 	mul	3(sp)
      000116 08 A0                  245 	ldc	#0
      000118 01 C2                  246 	add	1(sp)
      00011A 01 FA                  247 	stax	1(sp)
      00011C 09 F2                  248 	ldax	9(sp)
      00011E 02 C6                  249 	mul	2(sp)
      000120 08 A0                  250 	ldc	#0
      000122 01 C2                  251 	add	1(sp)
      000124 01 FA                  252 	stax	1(sp)
      000126 08 F2                  253 	ldax	8(sp)
      000128 02 C6                  254 	mul	2(sp)
      00012A 80 A0                  255 	push	a
      00012C C0 A0                  256 	pop	a
      00012E 06 FA                  257 	stax	6(sp)
      000130 C0 A0                  258 	pop	a
      000132 06 FA                  259 	stax	6(sp)
      000134 02 94                  260 	ads	#2
      000136 05 F2                  261 	ldax	5(sp)
      000138 05 86                  262 	mulu	5(sp)
      00013A 80 A0                  263 	push	a
      00013C 06 F2                  264 	ldax	6(sp)
      00013E 07 C6                  265 	mul	7(sp)
      000140 08 A0                  266 	ldc	#0
      000142 01 C2                  267 	add	1(sp)
      000144 01 FA                  268 	stax	1(sp)
      000146 07 F2                  269 	ldax	7(sp)
      000148 06 C6                  270 	mul	6(sp)
      00014A 08 A0                  271 	ldc	#0
      00014C 01 C2                  272 	add	1(sp)
      00014E 01 FA                  273 	stax	1(sp)
      000150 06 F2                  274 	ldax	6(sp)
      000152 06 C6                  275 	mul	6(sp)
      000154 02 FA                  276 	stax	2(sp)
      000156 C0 A0                  277 	pop	a
      000158 02 FA                  278 	stax	2(sp)
      00015A 04 F2                  279 	ldax	4(sp)
      00015C 80 A0                  280 	push	a
      00015E 04 F2                  281 	ldax	4(sp)
      000160 80 A0                  282 	push	a
      000162 04 F2                  283 	ldax	4(sp)
      000164 80 A0                  284 	push	a
      000166 04 F2                  285 	ldax	4(sp)
      000168 80 A0                  286 	push	a
      00016A 0A F2                  287 	ldax	10(sp)
      00016C 80 A0                  288 	push	a
      00016E 0A F2                  289 	ldax	10(sp)
      000170 80 A0                  290 	push	a
      000172 85 80                  291 	ldi	#>(___str_3 + 0)
      000174 80 A0                  292 	push	a
      000176 12 80                  293 	ldi	#<(___str_3 + 0)
      000178 80 A0                  294 	push	a
      00017A FE 97                  295 	ads	#-2
      00017C DD 01                  296 	jal	_printf
      00017E 0A 94                  297 	ads	#10
                                    298 ;	test_printf.c: 26: for (i = 0; i < 3; i++)
      000180 05 E6                  299 	inx	5(sp)
      000182 03 A2                  300 	if	c
      000184 06 E6                  301 	inx.p	6(sp)
      000186 00 80                  302 	ldi	#0x00
      000188 06 EA                  303 	cmp	6(sp)
      00018A 04 A2                  304 	if	gt
      00018C 08 B0                  305 	br.p	00119$
      00018E 05 A2                  306 	if	lt
      000190 05 B0                  307 	br.p	00120$
      000192 03 80                  308 	ldi	#0x03
      000194 05 EA                  309 	cmp	5(sp)
      000196 04 A2                  310 	if	gt
      000198 02 B0                  311 	br.p	00119$
      00019A                        312 00120$:
      00019A 02 B0                  313 	br	00121$
      00019C                        314 00119$:
      00019C B4 B7                  315 	br	00102$
      00019E                        316 00121$:
                                    317 ;	test_printf.c: 28: printf("big=%ld char=%c %d%%\n", big, 'Z', -42);
      00019E FF 80                  318 	ldi	#0xff
      0001A0 80 A0                  319 	push	a
      0001A2 D6 80                  320 	ldi	#0xd6
      0001A4 80 A0                  321 	push	a
      0001A6 00 80                  322 	ldi	#0x00
      0001A8 80 A0                  323 	push	a
      0001AA 5A 80                  324 	ldi	#0x5a
      0001AC 80 A0                  325 	push	a
      0001AE FF 80                  326 	ldi	#0xff
      0001B0 80 A0                  327 	push	a
      0001B2 FE 80                  328 	ldi	#0xfe
      0001B4 80 A0                  329 	push	a
      0001B6 1D 80                  330 	ldi	#0x1d
      0001B8 80 A0                  331 	push	a
      0001BA C0 80                  332 	ldi	#0xc0
      0001BC 80 A0                  333 	push	a
      0001BE 85 80                  334 	ldi	#>(___str_4 + 0)
      0001C0 80 A0                  335 	push	a
      0001C2 27 80                  336 	ldi	#<(___str_4 + 0)
      0001C4 80 A0                  337 	push	a
      0001C6 FE 97                  338 	ads	#-2
      0001C8 DD 01                  339 	jal	_printf
      0001CA 0C 94                  340 	ads	#12
                                    341 ;	test_printf.c: 29: printf("%d %d %d %d\n", -1, -10, -42, -300);
      0001CC FE 80                  342 	ldi	#0xfe
      0001CE 80 A0                  343 	push	a
      0001D0 D4 80                  344 	ldi	#0xd4
      0001D2 80 A0                  345 	push	a
      0001D4 FF 80                  346 	ldi	#0xff
      0001D6 80 A0                  347 	push	a
      0001D8 D6 80                  348 	ldi	#0xd6
      0001DA 80 A0                  349 	push	a
      0001DC FF 80                  350 	ldi	#0xff
      0001DE 80 A0                  351 	push	a
      0001E0 F6 80                  352 	ldi	#0xf6
      0001E2 80 A0                  353 	push	a
      0001E4 FF 80                  354 	ldi	#0xff
      0001E6 80 A0                  355 	push	a
      0001E8 FF 80                  356 	ldi	#0xff
      0001EA 80 A0                  357 	push	a
      0001EC 85 80                  358 	ldi	#>(___str_5 + 0)
      0001EE 80 A0                  359 	push	a
      0001F0 3D 80                  360 	ldi	#<(___str_5 + 0)
      0001F2 80 A0                  361 	push	a
      0001F4 FE 97                  362 	ads	#-2
      0001F6 DD 01                  363 	jal	_printf
      0001F8 0C 94                  364 	ads	#12
                                    365 ;	test_printf.c: 30: printf("%ld %ld\n", -123456L, -1L);
      0001FA FF 80                  366 	ldi	#0xff
      0001FC 80 A0                  367 	push	a
      0001FE FF 80                  368 	ldi	#0xff
      000200 80 A0                  369 	push	a
      000202 FF 80                  370 	ldi	#0xff
      000204 80 A0                  371 	push	a
      000206 FF 80                  372 	ldi	#0xff
      000208 80 A0                  373 	push	a
      00020A FF 80                  374 	ldi	#0xff
      00020C 80 A0                  375 	push	a
      00020E FE 80                  376 	ldi	#0xfe
      000210 80 A0                  377 	push	a
      000212 1D 80                  378 	ldi	#0x1d
      000214 80 A0                  379 	push	a
      000216 C0 80                  380 	ldi	#0xc0
      000218 80 A0                  381 	push	a
      00021A 85 80                  382 	ldi	#>(___str_6 + 0)
      00021C 80 A0                  383 	push	a
      00021E 4A 80                  384 	ldi	#<(___str_6 + 0)
      000220 80 A0                  385 	push	a
      000222 FE 97                  386 	ads	#-2
      000224 DD 01                  387 	jal	_printf
      000226 0C 94                  388 	ads	#12
                                    389 ;	test_printf.c: 31: printf("%d %u %x\n", 42, 42, 42);
      000228 00 80                  390 	ldi	#0x00
      00022A 80 A0                  391 	push	a
      00022C 2A 80                  392 	ldi	#0x2a
      00022E 80 A0                  393 	push	a
      000230 00 80                  394 	ldi	#0x00
      000232 80 A0                  395 	push	a
      000234 2A 80                  396 	ldi	#0x2a
      000236 80 A0                  397 	push	a
      000238 00 80                  398 	ldi	#0x00
      00023A 80 A0                  399 	push	a
      00023C 2A 80                  400 	ldi	#0x2a
      00023E 80 A0                  401 	push	a
      000240 85 80                  402 	ldi	#>(___str_7 + 0)
      000242 80 A0                  403 	push	a
      000244 53 80                  404 	ldi	#<(___str_7 + 0)
      000246 80 A0                  405 	push	a
      000248 FE 97                  406 	ads	#-2
      00024A DD 01                  407 	jal	_printf
      00024C 0A 94                  408 	ads	#10
                                    409 ;	test_printf.c: 32: printf("%d|%5d|%-5d|%05d\n", -7, -7, -7, -7);
      00024E FF 80                  410 	ldi	#0xff
      000250 80 A0                  411 	push	a
      000252 F9 80                  412 	ldi	#0xf9
      000254 80 A0                  413 	push	a
      000256 FF 80                  414 	ldi	#0xff
      000258 80 A0                  415 	push	a
      00025A F9 80                  416 	ldi	#0xf9
      00025C 80 A0                  417 	push	a
      00025E FF 80                  418 	ldi	#0xff
      000260 80 A0                  419 	push	a
      000262 F9 80                  420 	ldi	#0xf9
      000264 80 A0                  421 	push	a
      000266 FF 80                  422 	ldi	#0xff
      000268 80 A0                  423 	push	a
      00026A F9 80                  424 	ldi	#0xf9
      00026C 80 A0                  425 	push	a
      00026E 85 80                  426 	ldi	#>(___str_8 + 0)
      000270 80 A0                  427 	push	a
      000272 5D 80                  428 	ldi	#<(___str_8 + 0)
      000274 80 A0                  429 	push	a
      000276 FE 97                  430 	ads	#-2
      000278 DD 01                  431 	jal	_printf
      00027A 0C 94                  432 	ads	#12
                                    433 ;	test_printf.c: 33: return 0;
      00027C 00 80                  434 	ldi	#0x00
      00027E 09 FA                  435 	stax	9(sp)
      000280 0A FA                  436 	stax	10(sp)
      000282                        437 00104$:
                                    438 ;	test_printf.c: 34: }
      000282 06 94                  439 	ads	#6
      000284 64 A1                  440 	lra
      000286 00 8A                  441 	ret
                                    442 	.area CODE (CODE)
                                    443 	.area CONST (CODE,CDATA)
                                    444 	.area CONST (CODE,CDATA)
      0013D0                        445 ___str_0:
      0013D0 4C 80 00 8A 49 80 00   446 	.ascii "LISA"
             8A 53 80 00 8A 41 80
             00 8A
      0013E0 00 80 00 8A            447 	.db 0x00
                                    448 	.area CODE (CODE)
                                    449 	.area CONST (CODE,CDATA)
      0013E4                        450 ___str_1:
      0013E4 21 80 00 8A            451 	.ascii "!"
      0013E8 00 80 00 8A            452 	.db 0x00
                                    453 	.area CODE (CODE)
                                    454 	.area CONST (CODE,CDATA)
      0013EC                        455 ___str_2:
      0013EC 48 80 00 8A 65 80 00   456 	.ascii "Hello from %s, len %u"
             8A 6C 80 00 8A 6C 80
             00 8A 6F 80 00 8A 20
             80 00 8A 66 80 00 8A
             72 80 00 8A 6F 80 00
             8A 6D 80 00 8A 20 80
             00 8A 25 80 00 8A 73
             80 00 8A 2C 80 00 8A
             20 80 00 8A 6C 80 00
             8A 65 80 00 8A 6E 80
             00 8A 20 80 00 8A 25
             80 00 8A 75 80 00 8A
      001440 0A 80 00 8A            457 	.db 0x0a
      001444 00 80 00 8A            458 	.db 0x00
                                    459 	.area CODE (CODE)
                                    460 	.area CONST (CODE,CDATA)
      001448                        461 ___str_3:
      001448 69 80 00 8A 3D 80 00   462 	.ascii "i=%u sq=%u hex=%04x"
             8A 25 80 00 8A 75 80
             00 8A 20 80 00 8A 73
             80 00 8A 71 80 00 8A
             3D 80 00 8A 25 80 00
             8A 75 80 00 8A 20 80
             00 8A 68 80 00 8A 65
             80 00 8A 78 80 00 8A
             3D 80 00 8A 25 80 00
             8A 30 80 00 8A 34 80
             00 8A 78 80 00 8A
      001494 0A 80 00 8A            463 	.db 0x0a
      001498 00 80 00 8A            464 	.db 0x00
                                    465 	.area CODE (CODE)
                                    466 	.area CONST (CODE,CDATA)
      00149C                        467 ___str_4:
      00149C 62 80 00 8A 69 80 00   468 	.ascii "big=%ld char=%c %d%%"
             8A 67 80 00 8A 3D 80
             00 8A 25 80 00 8A 6C
             80 00 8A 64 80 00 8A
             20 80 00 8A 63 80 00
             8A 68 80 00 8A 61 80
             00 8A 72 80 00 8A 3D
             80 00 8A 25 80 00 8A
             63 80 00 8A 20 80 00
             8A 25 80 00 8A 64 80
             00 8A 25 80 00 8A 25
             80 00 8A
      0014EC 0A 80 00 8A            469 	.db 0x0a
      0014F0 00 80 00 8A            470 	.db 0x00
                                    471 	.area CODE (CODE)
                                    472 	.area CONST (CODE,CDATA)
      0014F4                        473 ___str_5:
      0014F4 25 80 00 8A 64 80 00   474 	.ascii "%d %d %d %d"
             8A 20 80 00 8A 25 80
             00 8A 64 80 00 8A 20
             80 00 8A 25 80 00 8A
             64 80 00 8A 20 80 00
             8A 25 80 00 8A 64 80
             00 8A
      001520 0A 80 00 8A            475 	.db 0x0a
      001524 00 80 00 8A            476 	.db 0x00
                                    477 	.area CODE (CODE)
                                    478 	.area CONST (CODE,CDATA)
      001528                        479 ___str_6:
      001528 25 80 00 8A 6C 80 00   480 	.ascii "%ld %ld"
             8A 64 80 00 8A 20 80
             00 8A 25 80 00 8A 6C
             80 00 8A 64 80 00 8A
      001544 0A 80 00 8A            481 	.db 0x0a
      001548 00 80 00 8A            482 	.db 0x00
                                    483 	.area CODE (CODE)
                                    484 	.area CONST (CODE,CDATA)
      00154C                        485 ___str_7:
      00154C 25 80 00 8A 64 80 00   486 	.ascii "%d %u %x"
             8A 20 80 00 8A 25 80
             00 8A 75 80 00 8A 20
             80 00 8A 25 80 00 8A
             78 80 00 8A
      00156C 0A 80 00 8A            487 	.db 0x0a
      001570 00 80 00 8A            488 	.db 0x00
                                    489 	.area CODE (CODE)
                                    490 	.area CONST (CODE,CDATA)
      001574                        491 ___str_8:
      001574 25 80 00 8A 64 80 00   492 	.ascii "%d|%5d|%-5d|%05d"
             8A 7C 80 00 8A 25 80
             00 8A 35 80 00 8A 64
             80 00 8A 7C 80 00 8A
             25 80 00 8A 2D 80 00
             8A 35 80 00 8A 64 80
             00 8A 7C 80 00 8A 25
             80 00 8A 30 80 00 8A
             35 80 00 8A 64 80 00
             8A
      0015B4 0A 80 00 8A            493 	.db 0x0a
      0015B8 00 80 00 8A            494 	.db 0x00
                                    495 	.area CODE (CODE)
                                    496 	.area INITIALIZER (CODE,CDATA)
                                    497 	.area CABS (ABS,CODE,CDATA)
