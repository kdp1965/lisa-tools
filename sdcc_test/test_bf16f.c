/*
 * test_bf16f.c - sdcc -mlisa --bf16-float: the C type float on the
 * bfloat16 unit.  Every result is a bf16 value (8 significant bits) in
 * float storage; the expected patterns are the bf16 results of
 * test_bf16 shifted into the top half.
 */
#include "harness.h"

#ifndef __SDCC_BF16_FLOAT
#error "build with --bf16-float"
#endif

volatile float fa = 1.5f, fb = 2.5f, fc = 0.1f, fd = 0.2f, fe = 100.0f, ff = 7.5f, fpi = 3.14159f, fm = -7.5f;
volatile int si = -1234, s7 = 7; volatile unsigned int ui = 21000, u1000 = 1000; volatile unsigned char uc = 200; volatile signed char sc = -100;
volatile float f1000 = 1000.0f, f3 = 3.0f;

union f32 { float f; unsigned long u; };
static unsigned long bits(float f) { union f32 v; v.f = f; return v.u; }

int main(void)
{
  /* arithmetic rounds to bf16 */
  CHECK(1, bits(fa + fb) == 0x40800000ul);                  /* 4.0 */
  CHECK(2, bits(fc + fd) == 0x3e9a0000ul);                  /* 0.30078125 */
  CHECK(3, bits(fe - ff) == 0x42b90000ul && bits(ff - fe) == 0xc2b90000ul);
  CHECK(4, bits(fb * ff) == 0x41960000ul);                  /* 18.75 */
  CHECK(5, bits(fpi * fpi) == 0x411d0000ul);                /* 9.87 truncated to 9.8125 */
  CHECK(6, bits(fe / ff) == 0x41550000ul);                  /* 13.333 -> 13.3125 */
  CHECK(7, bits(fe / f3) == 0x42050000ul && bits(f1000 * f1000) == 0x49740000ul);
  CHECK(8, bits(fm * fb) == 0xc1960000ul && bits(fm / fb) == 0xc03f0000ul);   /* -18.75, -3 by the divider's ulp */
  CHECK(9, bits(fa + fc) == 0x3fcd0000ul && bits(fe + fc) == 0x42c80000ul);   /* 1.6, 100.1 -> 100 */
  /* int <-> float through the unit */
  CHECK(10, bits((float)si) == 0xc49a0000ul && bits((float)s7) == 0x40e00000ul);   /* -1232, 7 */
  CHECK(11, bits((float)ui) == 0x46a40000ul && bits((float)uc) == 0x43480000ul);   /* 20992, 200 */
  CHECK(12, bits((float)sc) == 0xc2c80000ul && bits((float)u1000) == 0x447a0000ul);
  CHECK(13, (int)(fe / ff) == 13 && (unsigned int)(fb * ff) == 18 && (unsigned char)(fe - ff) == 92);
  CHECK(14, (int)fm == -7 && (signed char)(fm * fb) == -18 && (int)(fc - fe) == -100);   /* 0.1 - 100 rounds to -100 first */
  CHECK(15, (int)(float)si == -1232 && (unsigned int)(float)ui == 20992);
  /* comparisons stay exact on the 32-bit values */
  CHECK(16, fa < fb && !(fb < fa) && fa + fb == 4.0f && fc + fd != 0.3f && fm < fa);
  CHECK(17, fe / ff > 13.0f && fe / ff < 13.5f);
  /* a loop: the mean of 1..100 */
  {
    float s = 0.0f;
    unsigned char i;
    for (i = 1; i <= 100; i++)
      s += (float)i;
    CHECK(18, bits(s) == 0x459d0000ul);                     /* 5050, rounded at every step: 5024 */
    CHECK(19, bits(s / 100.0f) == 0x42490000ul);            /* 50.25 */
  }

  DONE();
  return 0;
}
