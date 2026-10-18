/* test_bank.h - the __banked functions of test_bank2.c (code bank 2) and
 * test_bank3.c (code bank 3), see test_bank.c */
unsigned char b2_byte(unsigned char x) __banked;
unsigned int  b2_sum(unsigned int a, unsigned int b) __banked;
long          b2_mul(long a, long b) __banked;
unsigned char b2_depth(unsigned char n) __banked;
typedef unsigned char (*bfn_t)(unsigned char) __banked;
typedef unsigned char (*fn_t)(unsigned char);
unsigned char b2_apply(bfn_t f, unsigned char x) __banked;
unsigned char b2_apply_common(fn_t f, unsigned char x) __banked;
unsigned char b3_byte(unsigned char x) __banked;
unsigned char b3_depth(unsigned char n) __banked;
unsigned int  b3_sum3(unsigned char a, unsigned int b, unsigned char c) __banked;
unsigned char b40_byte(unsigned char x) __banked;
