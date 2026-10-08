/*
 * test_index.c - a[i] and p[i] with a variable index: the address is formed
 * in IX (ldx / ldxx; ldax; addax, addaxu for a two-byte index) for a
 * temporary that only reads and writes through it use (gen.c
 * genIndexedAddr); its slot carries bit 15, so an address that escapes
 * (&a[i] returned, a generic pointer) must take the plain add.
 */
#include "harness.h"

static char a[17] = "0123456789abcdef";
static int w[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
static unsigned char ui = 5;
static int si = -2;

static unsigned int sum_a(void) { unsigned char i; unsigned int s = 0; for (i = 0; i < 16; i++) s += a[i]; return s; }
static int sum_w(void) { unsigned char i; int s = 0; for (i = 0; i < 8; i++) s += w[i]; return s; }
static void bump(unsigned char i) { a[i] = a[i] + 1; }                /* one temporary: a get, then a set */
static char near_at(__near char *p, unsigned char i) { return p[i]; }
static char gen_at(char *p, unsigned char i) { return p[i]; }         /* generic: the space test stays */
static char *addr_of(unsigned char i) { return &a[i]; }              /* the address escapes */
static void fill(unsigned char n) { unsigned char i; for (i = 0; i < n; i++) w[i] = i * 3; }

int main(void)
{
  char *q;
  CHECK(1, a[ui] == '5' && w[ui] == 60);
  CHECK(2, sum_a() == 1122);
  CHECK(3, sum_w() == 360);
  bump(ui); bump(0);
  CHECK(4, a[5] == '6' && a[0] == '1' && a[4] == '4' && a[6] == '6');
  CHECK(5, near_at(a, 7) == '7' && gen_at(a, 8) == '8' && gen_at("xyz", 1) == 'y');
  q = addr_of(9);
  CHECK(6, *q == '9' && q == a + 9 && q - a == 9);
  {
    __near char *np = a + 4;
    char *gp = a + 4;
    CHECK(7, np[si] == '2' && np[ui] == '9' && gp[si] == '2' && gp[ui] == '9');
  }
  {
    unsigned int k = ui + 2;
    CHECK(8, a[k] == '7' && w[k] == 80);
  }
  fill(8);
  CHECK(9, w[0] == 0 && w[7] == 21 && sum_w() == 84);
  {
    char loc[5] = "pqrs";
    CHECK(10, loc[ui - 4] == 'q' && loc[ui - 2] == 's');
  }
  DONE();
  return 0;
}
