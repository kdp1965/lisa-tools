#include "harness.h"
int printf(const char *fmt, ...);
int putchar(int c) { putc(c); return c; }
static const char *const owl[] = {
  "     .{{{}}}}}}.",
  "    {{{{{}}}}}}}.",
  "}}}}}}\\  '='  /}}}}}",
  "  ''\"''':   :''''''",
  "  jgs    `@` ",
  0,
};
int main(void)
{
  const char *const *p;
  for (p = owl; *p; p++) { puts(*p); putc('\n'); }
  for (p = owl; *p; p++) printf("%s\n", *p);
  DONE();
  return 0;
}
