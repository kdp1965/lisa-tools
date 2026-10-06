/*
 * test_regress.c - cases from bugs found with the SDCC regression suite
 * (support/regression, `make test-lisa`), kept small enough for the chip.
 */
#include "harness.h"
#include <string.h>
#include <setjmp.h>

/* 1. a static (direct-addressed) operand of a predicated sub needs its
      ldx hoisted ahead of the ifte (ldx is two words) */
static volatile int sl, sr, sres;
static long ll, lr, lres;

/* 2. an unsigned char operand narrower than the int result: the zero
      extension bytes must follow the borrow convention of `sub M` */
unsigned char uc(unsigned char x) { return x; }
int isub(int a, unsigned char b) { return a - b; }
long lsub(long a, unsigned char b) { return a - b; }

/* 3. struct by value */
struct pair { unsigned char lo; int hi; };
int sum_pair(unsigned char k, struct pair p) { return p.lo + p.hi + k; }

/* 4. bit fields */
struct bits { unsigned char a:3; unsigned char b:2; unsigned char c:3; };
struct wide { unsigned int x:12; signed int y:4; };
volatile struct bits vb;
struct wide vw;

/* 5. the shift count sharing a stack slot with the result */
static void set31(unsigned long *l) { *l = 31; }

/* 6. literal address as a generic pointer: data, not code (0x40 is
      reserved with __at so neither DATA nor the stack lands on it) */
static volatile unsigned char __at(0x40) scratch;
#define ABS(a) (*(volatile unsigned char *)(a))

/* 7. the A register allocator: an 8-bit division helper returns int into
      a one-byte result in A (only its low byte may be read), and a byte in
      A that an earlier compare tested needs its own zero test before the
      branch (Z was the compare's) */
static unsigned char divq(unsigned char a, unsigned char b) { return b / a; }
static unsigned char words(char *s)
{
  unsigned char n = 0;
  while (1)
    {
      while (*s == ' ')
        s++;
      if (*s == 0)
        break;
      n++;
      while (*s != ' ' && *s != 0)
        s++;
    }
  return n;
}

int main(void)
{
  sl = 5; sr = 26; sres = sl - sr;
  CHECK(1, sres == 5 - 26);
  ll = 0x10000; lr = 1; lres = ll - lr;
  CHECK(2, lres == 0xffff);

  CHECK(3, isub(0x58, uc(0)) == 0x58);
  CHECK(4, isub(0x100, uc(1)) == 0xff);
  CHECK(5, lsub(0x10000, uc(1)) == 0xffff);
  CHECK(6, lsub(0, uc(1)) == -1);
  CHECK(7, strcmp("X", "") > 0);
  CHECK(8, strcmp("", "X") < 0);
  CHECK(9, strcmp("abc", "abc") == 0);

  {
    struct pair p = { 3, 0x100 };
    CHECK(10, sum_pair(1, p) == 0x104);
  }

  vb.a = 5; vb.b = 2; vb.c = 7;
  CHECK(11, vb.a == 5 && vb.b == 2 && vb.c == 7);
  vb.b = 0xff;                        /* truncates to 3 */
  CHECK(12, vb.a == 5 && vb.b == 3 && vb.c == 7);
  vb.a |= 2; vb.c &= 5;
  CHECK(13, vb.a == 7 && vb.b == 3 && vb.c == 5);
  vw.x = 0xabc; vw.y = -3;
  CHECK(14, vw.x == 0xabc && vw.y == -3);
  vw.y = 7; vw.x = 0;
  CHECK(15, vw.x == 0 && vw.y == 7);

  {
    unsigned long l; int i;
    set31(&l);
    i = (int) l;
    l = (unsigned long)(2U << i);
    CHECK(16, l == 0);
    i = 3;
    CHECK(17, (0x8000U >> i) == 0x1000);
  }

  ABS(0x40) = 0x5a;
  CHECK(18, scratch == 0x5a && ABS(0x40) == 0x5a);

  {
    jmp_buf jb;
    static int n;
    n = 0;
    if (setjmp(jb) == 0)
      { n = 1; longjmp(jb, 7); n = 2; }
    else
      n += 10;
    CHECK(19, n == 11);
  }

  CHECK(20, divq(1, 2) == 2 && divq(7, 77) == 11);
  {
    char buf[12];
    strcpy(buf, " a bb  ccc ");
    CHECK(21, words(buf) == 3 && words("") == 0);
  }

  DONE();
  return 0;
}
