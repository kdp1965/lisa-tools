/*
 * test_bf16.c - the bfloat16 unit through <lisa/bf16.h>: arithmetic,
 * compares, integer and float conversions.  The expected bit patterns
 * were computed with IEEE single arithmetic rounded to bf16 (the
 * values here are exact in bf16, so the rounding mode does not matter).
 */
#include "harness.h"
#include <lisa/bf16.h>

volatile bf16_t b1_5 = 0x3fc0, b2_5 = 0x4020, b4 = 0x4080, bq = 0x3e80, b100 = 0x42c8, bm7_5 = 0xc0f0;
volatile bf16_t bpi = 0x4049, bthird = 0x3eab, b3 = 0x4040, b1000 = 0x447a, b0_1 = 0x3dcd, b0_2 = 0x3e4d;
volatile unsigned int u21000 = 21000, u1000 = 1000, u0 = 0, u65535 = 65535;
volatile int s7 = 7, sm7 = -7, sm32768 = -32768;
volatile float f12_34 = 12.34f, fm2_5 = -2.5f;

int main(void)
{
  /* add, sub, mul, div: operand order and sign */
  CHECK(1, bf16_add(b1_5, b2_5) == b4);
  CHECK(2, bf16_add(b2_5, b1_5) == b4);
  CHECK(3, bf16_sub(b100, bm7_5) == 0x42d7 && bf16_sub(bm7_5, b100) == 0xc2d7);   /* 107.5, -107.5 */
  CHECK(4, bf16_sub(b100, b2_5) == 0x42c3);                                       /* 97.5 */
  CHECK(5, bf16_mul(b2_5, b4) == 0x4120 && bf16_mul(b4, b2_5) == 0x4120);         /* 10 */
  CHECK(6, bf16_mul(bpi, bthird) == 0x3f86);                                      /* your fops_test: 1.046875 */
  CHECK(7, bf16_mul(bm7_5, b4) == 0xc1f0 && bf16_mul(bm7_5, bm7_5) == 0x4261);   /* -30, 56.25 */
  CHECK(8, bf16_div(b100, b4) == 0x41c8 && bf16_div(b4, b100) == 0x3d24);         /* 25, 0.04 */
  CHECK(9, bf16_div(BF16_ONE, b3) == bthird);
  CHECK(10, bf16_div(bm7_5, b2_5) == 0xc03f);                                     /* -3, which the divider misses by an ulp (2.984375) */
  CHECK(11, bf16_add(b0_1, b0_2) == 0x3e9a);                                      /* 0.30078125 */
  CHECK(12, bf16_mul(b1000, b1000) == 0x4974 && bf16_div(b100, b3) == 0x4205);    /* 1e6 -> 999424, 33.25 */
  CHECK(13, bf16_add(b100, BF16_ZERO) == b100 && bf16_mul(b100, BF16_ONE) == b100);
  CHECK(14, bf16_neg(b4) == 0xc080 && bf16_abs(bm7_5) == 0x40f0 && bf16_sub(b4, b4) == BF16_ZERO);
  /* compares */
  CHECK(15, bf16_gt(b2_5, b1_5) && !bf16_gt(b1_5, b2_5) && !bf16_gt(b4, b4));
  CHECK(16, bf16_lt(b1_5, b2_5) && bf16_ge(b4, b4) && bf16_le(b4, b4) && !bf16_le(b4, b1_5));
  CHECK(17, bf16_gt(b1_5, bm7_5) && !bf16_gt(bm7_5, b1_5) && bf16_gt(bm7_5, 0xc100));  /* -7.5 > -8 */
  CHECK(18, bf16_eq(b4, b4) && !bf16_eq(b4, b2_5));
  CHECK(19, bf16_cmp(b1_5, b2_5) == -1 && bf16_cmp(b2_5, b1_5) == 1 && bf16_cmp(b4, b4) == 0);
  CHECK(20, bf16_cmp(bm7_5, b1_5) == -1 && bf16_cmp(0xc100, bm7_5) == -1);
  /* integer conversions */
  CHECK(21, bf16_from_uint(u21000) == 0x46a4 && bf16_from_uint(u1000) == b1000);
  CHECK(22, bf16_from_uint(u0) == BF16_ZERO && bf16_from_uint(u65535) == 0x4780);
  CHECK(23, bf16_from_int(s7) == 0x40e0 && bf16_from_int(sm7) == 0xc0e0 && bf16_from_int(sm32768) == 0xc700);
  CHECK(24, bf16_to_uint(b1000) == 1000 && bf16_to_uint(b2_5) == 2 && bf16_to_uint(bq) == 0);
  CHECK(25, bf16_to_uint(bm7_5) == 0 && bf16_to_uint(0x4780) == 65535 && bf16_to_uint(BF16_MAX) == 65535);
  CHECK(26, bf16_to_int(bm7_5) == -7 && bf16_to_int(b100) == 100 && bf16_to_int(0xc700) == -32767);
  CHECK(27, bf16_to_uint(bf16_from_uint(u21000)) == 20992);                       /* 21000 rounds to 20992 */
  /* float conversions (software) */
  CHECK(28, bf16_from_float(f12_34) == 0x4145 && bf16_from_float(fm2_5) == 0xc020);
  CHECK(29, bf16_to_float(b2_5) == 2.5f && bf16_to_float(bm7_5) == -7.5f);
  CHECK(30, bf16_from_float(bf16_to_float(bpi)) == bpi);
  /* a chain: the area of a circle, r = 2.5: pi * 2.5 * 2.5 = 19.6 (fmul truncates: 19.5, not 19.625) */
  CHECK(31, bf16_mul(bf16_mul(bpi, b2_5), b2_5) == 0x419c);
  /* the adder's two silicon defects, and the software add getting them right */
  CHECK(33, bf16_fadd_raw(0x3fa0, 0x3fa0) == 0x4010 && bf16_add(0x3fa0, 0x3fa0) == 0x4020);     /* 1.25 + 1.25 */
  CHECK(34, bf16_fadd_raw(0x2e00, 0xa9d0) == 0x2d80 && bf16_add(0x2e00, 0xa9d0) == 0x2dff);     /* 2^-35 - 1.625 * 2^-44: half the value from the adder */
  CHECK(35, bf16_add(b1_5, b0_1) == 0x3fcd && bf16_add(b100, b0_1) == b100 && bf16_sub(b0_1, b1_5) == 0xbfb3);  /* 1.6, 100.1 -> 100, -1.4 */
  CHECK(36, bf16_add(0x7f7f, 0x7f7f) == BF16_INF && bf16_add(BF16_INF, BF16_NEG_INF) == BF16_NAN && bf16_add(0x0080, 0x0080) == 0x0100);
  CHECK(32, bf16_to_uint(bf16_mul(bf16_mul(bpi, b2_5), b2_5)) == 19);

  DONE();
  return 0;
}
