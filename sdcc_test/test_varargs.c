#include "harness.h"
#include <stdarg.h>
typedef union { unsigned char byte[5]; long l; unsigned long ul; } value_t;
long f2(int x) { long l = x; if (l < 0) l = -l; return l; }
long va(int n, ...) {
  va_list ap; value_t value; long r;
  va_start(ap, n);
  value.l = va_arg(ap, int);
  if (value.l < 0) value.l = -value.l;
  r = value.l;
  va_end(ap);
  return r;
}
unsigned char dig(unsigned long ul) {
  value_t value; unsigned char i = 32; unsigned char *pb4 = &value.byte[4];
  value.ul = ul; *pb4 = 0;
  do { *pb4 = (*pb4 << 1) | ((ul >> 31) & 0x01); ul <<= 1; if (10 <= *pb4) { *pb4 -= 10; ul |= 1; } } while (--i);
  value.ul = ul;
  return *pb4;
}
int main(void) {
  CHECK(1, f2(-42) == 42);
  CHECK(2, va(1, -42) == 42);
  CHECK(3, va(1, 42) == 42);
  CHECK(4, dig(42) == 2);
  CHECK(5, dig(123456) == 6);
  DONE();
  return 0;
}
