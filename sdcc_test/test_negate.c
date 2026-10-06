#include "harness.h"
int neg16(int x) { return -x; }
long neg32(long x) { return -x; }
signed char neg8(signed char x) { return -x; }
unsigned long ul(unsigned long x) { return x < 10 ? x : x - 7; }
int main(void) {
  CHECK(1, neg16(42) == -42);
  CHECK(2, neg16(-300) == 300);
  CHECK(3, neg32(123456) == -123456);
  CHECK(4, neg8(5) == -5 && neg8(-128) == -128);
  CHECK(5, neg16(0) == 0 && neg16(256) == -256);
  CHECK(6, ul(100000) == 99993);
  DONE();
  return 0;
}
