/* strings and pointer tables in code space, strlen, printf %s */
#include "harness.h"
#include <string.h>
int printf(const char *fmt, ...);
int putchar(int c) { putc(c); return c; }
static const char *const tab[] = { "one", "three", "fifteen", 0 };
const char msg[] = "hello";
int main(void)
{
  const char *const *p;
  unsigned char n = 0;
  CHECK(1, strlen(msg) == 5);
  CHECK(2, strlen(tab[0]) == 3 && strlen(tab[1]) == 5 && strlen(tab[2]) == 7);
  for (p = tab; *p; p++) n++;
  CHECK(3, n == 3);
  CHECK(4, tab[1][2] == 'r' && *tab[2] == 'f');
  puts("["); puts(tab[1]); puts("]\n");
  printf("[%s]\n", msg);
  printf("[%s]\n", tab[2]);
  printf("[%5s|%-5s]\n", "ab", "cd");
  for (p = tab; *p; p++) printf("%s\n", *p);
  DONE();
  return 0;
}
