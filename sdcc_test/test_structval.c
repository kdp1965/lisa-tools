/* test_structval.c - structs and unions by value: passed, returned and
 * assigned (the lisa ABI, sdcc-lisa src/lisa/PLAN.md: a struct result in a
 * slot the caller reserves above the return address, a struct argument
 * pushed whole, a struct copy through memcpy; one read through a generic
 * pointer is pushed with __gptrprev).  Asked on the NuttX list: older SDCC
 * releases and some ports had none of these.
 *
 *  1    assignment, s1 = s2
 *  2    a struct, a union and a char passed by value together
 *  3    a union returned by value
 *  4    a struct returned, passed and returned again, nested calls
 *  5    the callee's changes to its copy do not reach the caller's
 *  6    a struct from code space (const) and through a pointer, by value
 *  7    a struct holding an array and a struct, returned and assigned
 *  8    a struct after a byte argument (the byte travels in A) */
#include "harness.h"

struct pt { int x, y; char tag; long big; };
union u { long l; unsigned char b[4]; };
struct box { struct pt corner; unsigned char cell[5]; int w; };

static const struct pt origin = { -7, 9, 'o', 123456L };

static struct pt mk(int x, int y, char t, long b)
{
  struct pt p;
  p.x = x; p.y = y; p.tag = t; p.big = b;
  return p;
}

static long sum(struct pt p, union u v, char k)
{
  return p.x + p.y + p.tag + p.big + v.l + k;
}

static union u swapu(union u v)
{
  union u r;
  r.b[0] = v.b[3]; r.b[1] = v.b[2]; r.b[2] = v.b[1]; r.b[3] = v.b[0];
  return r;
}

static struct pt add(struct pt a, struct pt b)
{
  a.x += b.x; a.y += b.y; a.big += b.big;
  return a;
}

static int clobber(struct pt p)
{
  p.x = 0; p.y = 0; p.big = 0;
  return p.tag;
}

static struct box grow(struct box b, int by)
{
  unsigned char i;
  for (i = 0; i < 5; i++)
    b.cell[i] += by;
  b.w += by;
  b.corner = add(b.corner, mk(by, by, 0, by));
  return b;
}

static int after(char k, struct pt p)
{
  return k * 100 + p.x + p.y;
}

void main(void)
{
  struct pt a, b, c;
  const struct pt *pp = &origin;
  union u v, w;
  struct box x, y;
  unsigned char i;

  a = mk(1, 2, 3, 100000L);
  b = a;
  CHECK(1, b.x == 1 && b.y == 2 && b.tag == 3 && b.big == 100000L);

  v.l = 0x11223344L;
  CHECK(2, sum(b, v, 5) == 1 + 2 + 3 + 100000L + 0x11223344L + 5);

  w = swapu(v);
  CHECK(3, w.l == 0x44332211L && v.l == 0x11223344L);

  c = add(a, add(mk(10, 20, 0, 1L), mk(100, 200, 0, 2L)));
  CHECK(4, c.x == 111 && c.y == 222 && c.big == 100003L && c.tag == 3);

  CHECK(5, clobber(a) == 3 && a.x == 1 && a.y == 2 && a.big == 100000L);

  c = add(origin, *pp);
  CHECK(6, c.x == -14 && c.y == 18 && c.tag == 'o' && c.big == 246912L);

  x.corner = a;
  for (i = 0; i < 5; i++)
    x.cell[i] = i;
  x.w = 40;
  y = grow(x, 3);
  CHECK(7, y.cell[0] == 3 && y.cell[4] == 7 && y.w == 43 && y.corner.x == 4 &&
           y.corner.big == 100003L && x.cell[4] == 4 && x.w == 40);

  CHECK(8, after(7, mk(20, 3, 0, 0)) == 723);
  DONE();
}
