/* test_cmparray.c - comparisons with an array operand.  An array decays
 * to a pointer: compared with a null pointer constant written 0 it is
 * never equal (sdcc refused "array != 0" with "incompatible types" - NuttX
 * NX's DEBUGASSERT(wnd->be->plane != NULL), NULL being 0); and an array
 * against a pointer of another type is compared with the operands swapped
 * and the operator mirrored, where '<' was never mirrored (the code tested
 * '>' twice), so arr < p computed p < arr.
 *
 *  1-4  array == / != 0, both orders, a member array through a pointer
 *  5-6  array < char *, array > char * (the swapped operands)
 *  7    array <= / >= a pointer to itself */
#include "harness.h"

struct holder { int pad; int plane[1]; };

static int arr[4];
static struct holder h;
static struct holder *hp = &h;

void main(void)
{
  char *p = (char *) &arr[2];
  char *q = (char *) arr;

  CHECK(1, arr != 0 && !(arr == 0));
  CHECK(2, 0 != arr && !(0 == arr));
  CHECK(3, hp->plane != 0);
  CHECK(4, !(h.plane == 0));

  CHECK(5, arr < p && !(arr < q));
  CHECK(6, !(arr > p) && arr <= q);
  CHECK(7, arr <= q && arr >= q);
  DONE();
}
