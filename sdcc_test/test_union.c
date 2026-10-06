#include "harness.h"
typedef union { unsigned char byte[5]; long l; unsigned long ul; } value_t;
long f(long v) {
  value_t value;
  value.l = v;
  if (value.l < 0) value.l = -value.l;
  return value.l;
}
long g(long v) { long x = v; if (x < 0) x = -x; return x; }
int main(void) {
  CHECK(1, f(-42) == 42);
  CHECK(2, f(-123456) == 123456);
  CHECK(3, g(-42) == 42);
  CHECK(4, f(77) == 77);
  DONE();
  return 0;
}
