/* test_huge5.c - code bank 5 of test_huge.c (sdcc --model-huge --codeseg BANK5) */
#include "test_huge.h"

unsigned char h5_byte(unsigned char x) { return x + 5; }
unsigned int  h5_sum(unsigned int a, unsigned int b) { return a + b; }
long          h5_mul(long a, long b) { return a * b; }
unsigned char h5_depth(unsigned char n) { return n ? h6_depth(n - 1) + 1 : 0; }
unsigned char h5_apply(fn_t f, unsigned char x) { return f(x) + 5; }
unsigned char h5_apply_nb(nbfn_t f, unsigned char x) { return f(x) + 5; }
