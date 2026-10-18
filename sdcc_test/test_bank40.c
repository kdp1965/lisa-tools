/* test_bank40.c - code bank 40 of test_bank.c: a bank above 15 (the code
 * bank register is 6 bits) */
#pragma bank 40
#include "test_bank.h"

unsigned char b40_byte(unsigned char x) __banked { return b3_byte(x) + 40; }
