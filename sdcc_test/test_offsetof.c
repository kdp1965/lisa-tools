/* test_offsetof.c - offsetof() of nested members, as a value and cast to a
 * pointer.  sdcc's offsetof is an address computed from a null pointer; a
 * nested member is (constant pointer + constant) + constant, and the
 * optimizer's cast/add folding (SDCCopt.c optimizeCastCast) put the
 * literal in the second add's place and failed validateOpType ("expected
 * symbol, got value") - found in NuttX's nx_start.c with message queues on:
 * (FAR void *)offsetof(struct mqueue_inode_s, cmn.waitfornotempty).
 *
 *  1   a nested member's offset
 *  2   two levels deep
 *  3   cast to a pointer, then back
 *  4   stored through a table of pointers, as nx_start does
 *  5   a member of an array element inside a struct */
#include "harness.h"
#include <stddef.h>

/* NuttX's offsetof (include/stddef.h's fallback): sdcc's own is
   __builtin_offsetof, folded early - this one goes through the optimizer */
#undef offsetof
#define offsetof(t, m) ((size_t)(&(((t *)(0))->m)))

struct inner { unsigned char a; unsigned int b; unsigned char c[3]; };
struct mid { unsigned int x; struct inner in; };
struct outer { unsigned char tag; struct mid m; struct inner arr[2]; };

struct slot { void *list; unsigned char attr; };
static struct slot table[3];

void main(void)
{
  unsigned int off;

  off = offsetof(struct mid, in.b);
  CHECK(1, off == sizeof (unsigned int) + 1);

  off = offsetof(struct outer, m.in.c);
  CHECK(2, off == 1 + sizeof (unsigned int) + 1 + sizeof (unsigned int));

  CHECK(3, (unsigned int)(size_t)(void *)offsetof(struct outer, m.in.b) ==
           1 + sizeof (unsigned int) + 1);

  table[0].list = (void *)offsetof(struct mid, in.b);
  table[1].list = (void *)offsetof(struct outer, m.in.c);
  CHECK(4, (size_t)table[0].list == offsetof(struct mid, in.b) &&
           (size_t)table[1].list == offsetof(struct outer, m.in.c));

  off = offsetof(struct outer, arr[1].c);
  CHECK(5, off == offsetof(struct outer, arr) + sizeof (struct inner) +
                  offsetof(struct inner, c));
  DONE();
}
