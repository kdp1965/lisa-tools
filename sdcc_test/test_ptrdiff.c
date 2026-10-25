/* test_ptrdiff.c - pointer differences: their value and type.  With the
 * third chip's 24-bit pointers (--rev3) a difference is a long (sdcc's
 * newPtrDiffLink); the iCode typed it int, and as a variadic argument it
 * was pushed as two bytes while the call counted four (sdcc-lisa crashed:
 * "Stack offset below SP", found in NuttX's i2c_hexdump.c).
 *
 *  1-3  forwards, backwards, element sizes 1, 2 and 6 (a struct)
 *  4    the difference's size is a ptrdiff's (long on --rev3, int before)
 *  5-6  a difference through a variadic function, then an int after it
 *  7    assigned to an int, compared with a literal
 *  8    across 64 KB (--rev3: the 24-bit space; skipped before) */
#include "harness.h"
#include <stdarg.h>
#include <stddef.h>

struct six { unsigned char b[6]; };

static unsigned char bytes[40];
static unsigned int words[20];
static struct six sixes[10];

static long va_got;
static int va_after;

static void take(const char *fmt, ...)
{
  va_list ap;
  (void) fmt;
  va_start(ap, fmt);
  va_got = (long) va_arg(ap, ptrdiff_t);
  va_after = va_arg(ap, int);
  va_end(ap);
}

static ptrdiff_t diff(unsigned char *p, void *q)
{
  return p - (unsigned char *) q;
}

void main(void)
{
  unsigned char *p = &bytes[30], *q = &bytes[5];
  unsigned int *wp = &words[17], *wq = &words[2];
  struct six *sp = &sixes[9], *sq = &sixes[1];
  int i;

  CHECK(1, p - q == 25 && q - p == -25);
  CHECK(2, wp - wq == 15 && wq - wp == -15);
  CHECK(3, sp - sq == 8 && sq - sp == -8);
  CHECK(4, sizeof (p - q) == sizeof (ptrdiff_t));

  take("x", p - (unsigned char *) q, 0x1234);
  CHECK(5, va_got == 25);
  CHECK(6, va_after == 0x1234);

  i = (int)(q - p);
  CHECK(7, i == -25 && diff(p, q) == 25);

#ifdef __SDCC_LISA_REV3
  {
    unsigned char *far1 = (unsigned char *) 0x10000;
    unsigned char *far2 = (unsigned char *) 0x30010;
    CHECK(8, far2 - far1 == 0x20010L && far1 - far2 == -0x20010L);
  }
#else
  CHECK(8, 1);
#endif
  DONE();
}
