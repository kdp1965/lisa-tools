/* test_huge6.c - code bank 6 of test_huge.c (sdcc --model-huge --codeseg BANK6) */
#include "test_huge.h"

unsigned char h6_byte(unsigned char x) { return h5_byte(x) + 6; }
unsigned char h6_depth(unsigned char n) { return n ? h5_depth(n - 1) + 1 : 0; }
unsigned int  h6_sum3(unsigned char a, unsigned int b, unsigned char c) { return common_add(a) + b + common_add_nb(c); }
