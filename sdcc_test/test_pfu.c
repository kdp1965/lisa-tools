/* printf/sprintf %u on the chip: the sdcc_hello demo printed sum: 1 */
#include "harness.h"
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
#include <string.h>
int putchar(int c) { putc(c); return c; }
volatile unsigned int vs = 178, vc = 337;
char buf[16];
int main(void)
{
  unsigned int sum = vs, count = vc;
  sprintf(buf, "%u", sum);
  CHECK(1, strcmp(buf, "178") == 0);
  sprintf(buf, "%u %u", count, sum);
  CHECK(2, strcmp(buf, "337 178") == 0);
  sprintf(buf, "%d", -5);
  CHECK(3, strcmp(buf, "-5") == 0);
  CHECK(4, sum / 10 == 17 && sum % 10 == 8);
  printf("\nCount: %u sum: %u\n", count, sum);
  DONE();
  return 0;
}
