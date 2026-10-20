/*
 * test_rev3c.c - the third chip's compiler (sdcc -mlisa --rev3, ttsky26d
 * REV3.md): 24-bit pointers and registers from C.
 *
 * Data beyond 64K (an __at object at 1 MB, a far pointer walk across the
 * 64K boundary), a 2-byte __near pointer against a 3-byte generic one,
 * pointer arithmetic and compares on 3 bytes, a function pointer in a
 * table and in a struct, a pointer to a packed constant beyond the first
 * string, setjmp / longjmp with the 8-byte jmp_buf, the divider's whole
 * 16-bit results by a variable divisor, a long cast of a pointer, and a
 * pointer returned in IX straight through a chain of calls.
 *
 *   sdcc -mlisa --rev3 -o test_rev3c_rev3.ihx test_rev3c.c
 *   lisa_sim --rev3 -b -c 2000000 test_rev3c_rev3.ihx
 */
#include <setjmp.h>
#include <string.h>
#include "harness.h"

__at(0x100000) unsigned char farbuf[64];     /* 1 MB: a 24-bit address */
__at(0x00fff8) unsigned char edge[16];       /* across the 64K boundary */
__xdata unsigned int fararr[8];              /* FDATA: through IX */
__near unsigned char nearbuf[8];             /* DATA: lda / sta */
const unsigned char ktab[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
const char *const kstrs[] = { "zero", "one", "two" };

static int f_add(int a, int b) { return a + b; }
static int f_sub(int a, int b) { return a - b; }
static int f_mul(int a, int b) { return a * b; }
typedef int (*op_t)(int, int);
const op_t ops[] = { f_add, f_sub, f_mul };
struct named { const char *name; op_t fn; };
const struct named table[] = { { "add", f_add }, { "sub", f_sub }, { "mul", f_mul } };

static unsigned char *fill(unsigned char *p, unsigned char n, unsigned char v)
{
  while (n--)
    *p++ = v++;
  return p;
}

static unsigned char sum(const unsigned char *p, unsigned char n)
{
  unsigned char s = 0;
  while (n--)
    s += *p++;
  return s;
}

static unsigned char nsum(__near const unsigned char *p, unsigned char n)
{
  unsigned char s = 0;
  while (n--)
    s += *p++;
  return s;
}

static unsigned char *passthrough(unsigned char *p) { return p; }
static unsigned char *chain(unsigned char *p) { return passthrough(p + 1); }

static jmp_buf jb;
static unsigned char depth;

static void dive(unsigned char n)
{
  unsigned char local[8];
  memset(local, n, sizeof local);
  depth = n + local[7];
  if (n == 0)
    longjmp(jb, 42);
  dive(n - 1);
}

unsigned int udiv(unsigned int a, unsigned int b) { return a / b; }
unsigned int umod(unsigned int a, unsigned int b) { return a % b; }
int sdiv(int a, int b) { return a / b; }
int smod(int a, int b) { return a % b; }

int main(void)
{
  unsigned char *p;
  unsigned char i;
  unsigned long l;

  /* 1-4: data at 1 MB */
  for (i = 0; i < 64; i++)
    farbuf[i] = i * 3;
  CHECK(1, farbuf[0] == 0 && farbuf[63] == 189);
  p = farbuf;
  CHECK(2, sum(p, 10) == 135);
  p = fill(farbuf + 16, 8, 100);
  CHECK(3, p == farbuf + 24 && farbuf[16] == 100 && farbuf[23] == 107);
  l = (unsigned long)p;
  CHECK(4, l == 0x100018ul);

  /* 5-7: across the 64K boundary */
  fill(edge, 16, 1);
  CHECK(5, edge[7] == 8 && edge[8] == 9);
  p = edge + 7;
  CHECK(6, p[1] == 9 && *(p + 2) == 10);
  l = (unsigned long)(edge + 8);
  CHECK(7, l == 0x10000ul);

  /* 8-9: near pointers */
  for (i = 0; i < 8; i++)
    nearbuf[i] = i + 1;
  CHECK(8, nsum(nearbuf, 8) == 36);
  CHECK(9, sum(nearbuf, 8) == 36);       /* the same bytes through a generic pointer */

  /* 10-11: FDATA ints through IX */
  for (i = 0; i < 8; i++)
    fararr[i] = i * 1000u;
  CHECK(10, fararr[7] == 7000u && fararr[3] == 3000u);
  {
    unsigned int *ip = fararr + 2;
    ip[1] += 5;
    CHECK(11, fararr[3] == 3005u && ip - fararr == 2);
  }

  /* 12-14: constants in code space */
  CHECK(12, sum(ktab, 8) == 104);
  CHECK(13, strcmp(kstrs[2], "two") == 0 && strlen(kstrs[0]) == 4);
  {
    const unsigned char *kp = ktab + 5;
    CHECK(14, *kp == 60 && kp[-1] == 50 && kp > ktab);
  }

  /* 15-17: function pointers */
  CHECK(15, ops[0](2, 3) == 5 && ops[1](2, 3) == -1 && ops[2](7, 6) == 42);
  for (i = 0; i < 3; i++)
    if (strcmp(table[i].name, "sub") == 0)
      break;
  CHECK(16, i == 1 && table[i].fn(10, 4) == 6);
  CHECK(17, table[2].fn == f_mul && ops[0] != ops[1]);

  /* 18-19: a pointer back in IX through a chain */
  p = chain(farbuf + 2);
  CHECK(18, p == farbuf + 3 && *p == 9);
  CHECK(19, passthrough(edge) == edge);

  /* 20-21: setjmp / longjmp */
  depth = 0;
  {
    int r = setjmp(jb);
    if (r == 0)
      dive(5);
    CHECK(20, r == 42 && depth == 0);
  }
  CHECK(21, sizeof(jb) == 8);

  /* 22-25: the divider's 16-bit results by a variable divisor */
  CHECK(22, udiv(0xffffu, 1) == 0xffffu && umod(0xffffu, 1) == 0);
  CHECK(23, udiv(50000u, 3) == 16666u && umod(50000u, 3) == 2);
  CHECK(24, umod(60000u, 40000u) == 20000u && udiv(60000u, 40000u) == 1);
  CHECK(25, sdiv(-30000, 7) == -4285 && smod(-30000, 7) == -5);

  /* 26-27: memcpy / memset across 64K */
  memset(edge, 0xaa, 16);
  memcpy(farbuf, edge + 4, 8);
  CHECK(26, farbuf[0] == 0xaa && farbuf[7] == 0xaa && farbuf[8] == 24);
  memcpy(edge + 8, ktab, 8);
  CHECK(27, edge[8] == 10 && edge[15] == 80 && edge[7] == 0xaa);

  DONE();
  return 0;
}
