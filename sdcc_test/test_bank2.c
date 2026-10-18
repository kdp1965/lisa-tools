/* test_bank2.c - code bank 2 of test_bank.c */
#pragma bank 2
#include "test_bank.h"

unsigned char b2_byte(unsigned char x) __banked { return x + 2; }
unsigned int  b2_sum(unsigned int a, unsigned int b) __banked { return a + b; }
long          b2_mul(long a, long b) __banked { return a * b; }
unsigned char b2_depth(unsigned char n) __banked { return n ? b3_depth(n - 1) + 1 : 0; }
unsigned char b2_apply(bfn_t f, unsigned char x) __banked { return f(x) + 2; }
unsigned char b2_apply_common(fn_t f, unsigned char x) __banked { return f(x) + 2; }
