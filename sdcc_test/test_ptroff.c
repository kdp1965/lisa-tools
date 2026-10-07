/*
 * test_ptroff.c - reads and struct pushes through pointers with the
 * offset folded into the access (SDCCopt's offsetFoldGet / offsetFoldUse
 * for lisa): p->member, p[k] and &local[k] in data and code space,
 * through the gptrget.s helpers (__gptrcodeo, __gptrgeto, __gptrprev) by
 * default and inline with --opt-code-speed.  The 303-byte structs put a
 * member past the 255 the fold stops at; built for the TT07 cache.
 */
#include "harness.h"

struct s { char a; char b; int c; long d; };
struct big { char head; char pad[150]; char mid; char pad2[150]; char tail; };   /* mid at 151, tail at 302 */

static const struct s cs = { 1, 2, 0x1234, 0x76543210L };
static const char ca[] = "hello";
static struct s rs = { 3, 4, 0x55, 0x10203040L };
static char ram[6] = "world";
static struct big bg;
static const struct big __code cbg = { 7, { 0 }, 8, { 0 }, 9 };

static int sum(struct s v) { return v.a + v.b + v.c + (int)v.d; }
static char rd_code1(const char __code *p) { return p[1]; }
static char rd_code2(const char __code *p) { return p[2]; }
static int rd_scc(const struct s __code *p) { return p->c; }
static long rd_scd(const struct s __code *p) { return p->d; }
static char rd_gen1(const char *p) { return p[1]; }
static char rd_gen4(const char *p) { return p[4]; }
static int rd_gc(const struct s *p) { return p->c; }
static long rd_gd(const struct s *p) { return p->d; }
static char rd_prev(const char *p) { return p[-1]; }
static char rd_mid(struct big *p) { return p->mid; }
static char rd_tail(struct big *p) { return p->tail; }
static char rd_cmid(const struct big __code *p) { return p->mid; }
static char rd_ctail(const struct big __code *p) { return p->tail; }
static int push_code(const struct s __code *p) { return sum(*p); }
static int push_gen(const struct s *p) { return sum(*p); }
static int push_ram(struct s *p) { return sum(*p); }
static int push_next(const struct s *p) { return sum(p[1]); }
static char at(char *p) { return *p; }

int main(void)
{
  char buf[8] = "abcdefg";
  const struct s pair[2] = { { 1, 1, 1, 1L }, { 2, 2, 2, 2L } };

  bg.mid = 5;
  bg.tail = 6;
  CHECK(1, rd_code1(ca) == 'e');
  CHECK(2, rd_code2(ca) == 'l');
  CHECK(3, rd_scc(&cs) == 0x1234);
  CHECK(4, rd_scd(&cs) == 0x76543210L);
  CHECK(5, rd_gen1(ca) == 'e' && rd_gen4(ca) == 'o');
  CHECK(6, rd_gen1(ram) == 'o' && rd_gen4(ram) == 'd');
  CHECK(7, rd_gc(&cs) == 0x1234 && rd_gd(&cs) == 0x76543210L);
  CHECK(8, rd_gc(&rs) == 0x55 && rd_gd(&rs) == 0x10203040L);
  CHECK(9, push_code(&cs) == 0x1237 + 0x3210);
  CHECK(10, push_gen(&cs) == 0x1237 + 0x3210);
  CHECK(11, push_gen(&rs) == 0x5c + 0x3040);
  CHECK(12, push_ram(&rs) == 0x5c + 0x3040);
  CHECK(13, rd_prev(ram + 2) == 'o' && rd_prev(ca + 1) == 'h');
  CHECK(14, rd_mid(&bg) == 5 && rd_tail(&bg) == 6);
  CHECK(15, rd_cmid(&cbg) == 8 && rd_ctail(&cbg) == 9);
  CHECK(16, at(&buf[3]) == 'd' && at(buf + 6) == 'g');
  CHECK(17, push_next(pair) == 8);
  DONE();
  return 0;
}
