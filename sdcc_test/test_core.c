#include "harness.h"
unsigned char g8 = 7;
int g16 = -1234;
long g32 = 0x12345678;
unsigned char arr[4] = {1, 2, 3, 4};
const unsigned char ctab[4] = {10, 20, 30, 40};
const int citab[3] = {-1, 300, 32000};
unsigned char add8(unsigned char a, unsigned char b) { return a + b; }
int add16(int a, int b) { return a + b; }
long add32(long a, long b) { return a + b; }
int sub16(int a, int b) { return a - b; }
unsigned char shl(unsigned char a, unsigned char n) { return a << n; }
unsigned int shr16(unsigned int a, unsigned char n) { return a >> n; }
int sar16(int a) { return a >> 3; }
unsigned char cmpu(unsigned char a, unsigned char b) { return a < b; }
unsigned char cmps(signed char a, signed char b) { return a < b; }
unsigned char cmp16(int a, int b) { return a < b; }
unsigned char cmp16u(unsigned int a, unsigned int b) { return a > b; }
unsigned int mul16(unsigned int a, unsigned int b) { return a * b; }
int mul8s(signed char a, signed char b) { return a * b; }
unsigned char sum(const unsigned char *p, unsigned char n) { unsigned char s = 0; while (n--) s += *p++; return s; }
void fill(unsigned char *p, unsigned char n, unsigned char v) { while (n--) *p++ = v++; }
unsigned char sw(unsigned char x) { switch (x) { case 0: return 10; case 1: return 11; case 2: return 12; case 5: return 15; default: return 99; } }
unsigned char (*fp)(unsigned char, unsigned char);
int main(void) {
  unsigned char i, s;
  CHECK(1, add8(200, 100) == 44);
  CHECK(2, add16(-1234, 1300) == 66);
  CHECK(3, add32(0x12345678, 0x11111111) == 0x23456789);
  CHECK(4, sub16(100, 300) == -200);
  CHECK(5, shl(3, 4) == 48);
  CHECK(6, shr16(0x8000, 15) == 1);
  CHECK(7, sar16(-64) == -8);
  CHECK(8, cmpu(3, 200) == 1 && cmpu(200, 3) == 0);
  CHECK(9, cmps(-3, 2) == 1 && cmps(2, -3) == 0 && cmps(-3, -3) == 0);
  CHECK(10, cmp16(-300, 200) == 1 && cmp16(200, -300) == 0 && cmp16(-300, -301) == 0);
  CHECK(11, cmp16u(0x8000, 0x7fff) == 1 && cmp16u(1, 2) == 0);
  CHECK(12, mul16(300, 7) == 2100);
  CHECK(13, mul8s(-5, 7) == -35 && mul8s(-5, -7) == 35);
  CHECK(14, sum(arr, 4) == 10);
  CHECK(15, sum(ctab, 4) == 100);
  CHECK(16, citab[1] == 300 && citab[2] == 32000 && citab[0] == -1);
  fill(arr, 4, 9);
  CHECK(17, arr[0] == 9 && arr[3] == 12);
  CHECK(18, g8 == 7 && g16 == -1234 && g32 == 0x12345678);
  s = 0; for (i = 0; i < 10; i++) s += i;
  CHECK(19, s == 45);
  CHECK(20, sw(0) == 10 && sw(2) == 12 && sw(5) == 15 && sw(3) == 99);
  fp = add8;
  CHECK(21, fp(1, 2) == 3);
  g16 = g16 * 2 + g8;
  CHECK(22, g16 == -2461);
  g32 += 0x100;
  CHECK(23, g32 == 0x12345778);
  g8 |= 0x80; g8 &= 0xf0; g8 ^= 0x11;
  CHECK(24, g8 == 0x91);
  CHECK(25, (g16 / 10) == -246 && (g16 % 10) == -1);
  DONE();
  return fails;
}
