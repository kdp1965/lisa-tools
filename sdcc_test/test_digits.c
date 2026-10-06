#include "harness.h"
typedef union { unsigned char byte[5]; long l; unsigned long ul; } value_t;
static void calculate_digit (value_t *value, unsigned char radix) {
  unsigned long ul = value->ul;
  unsigned char *pb4 = &value->byte[4];
  unsigned char i = 32;
  do {
    *pb4 = (*pb4 << 1) | ((ul >> 31) & 0x01);
    ul <<= 1;
    if (radix <= *pb4) { *pb4 -= radix; ul |= 1; }
  } while (--i);
  value->ul = ul;
}
unsigned char d1, d2, d3;
int main(void) {
  value_t value;
  value.ul = 123456; value.byte[4] = 0;
  calculate_digit(&value, 10); d1 = value.byte[4];
  value.byte[4] = 0; calculate_digit(&value, 10); d2 = value.byte[4];
  CHECK(1, d1 == 6);
  CHECK(2, d2 == 5);
  CHECK(3, value.ul == 1234);
  value.ul = 42; value.byte[4] = 0;
  calculate_digit(&value, 10); d3 = value.byte[4];
  CHECK(4, d3 == 2 && value.ul == 4);
  DONE();
  return 0;
}
