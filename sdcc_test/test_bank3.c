/* test_bank3.c - code bank 3 of test_bank.c */
#pragma bank 3
#include "test_bank.h"

unsigned char b3_byte(unsigned char x) __banked { return b2_byte(x) + 3; }
unsigned char b3_depth(unsigned char n) __banked { return n ? b2_depth(n - 1) + 1 : 0; }
unsigned int  b3_sum3(unsigned char a, unsigned int b, unsigned char c) __banked { return a + b + c; }
