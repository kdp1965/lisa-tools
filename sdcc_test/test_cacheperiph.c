/*
 * test_cacheperiph.c - a cached store followed closely by a peripheral access
 * (the data cache on the SPI RAM, hw_test.mjs spiram=): what the monitor's
 * receive loop does between a character and its UART status poll.  Each
 * loop stores a byte in the lower half of memory, touches a peripheral
 * register at once, and reads the byte back.
 */
#include "harness.h"
#include <tt07.h>

static volatile unsigned char buf[32];
static volatile unsigned char tmp;

int main(void)
{
  unsigned char i, s, bad1 = 0, bad2 = 0, bad3 = 0, bad4 = 0, bad5 = 0;
  for (i = 0; i < 200; i++) {                     /* store, then a peripheral read */
    buf[i & 15] = i;
    s = UART1_STATUS;
    if (buf[i & 15] != i) bad1++;
  }
  CHECK(1, bad1 == 0);
  for (i = 0; i < 200; i++) {                     /* store, then a peripheral write */
    buf[i & 15] = i;
    TIMER1_PREDIV_LO = i;
    if (buf[i & 15] != i) bad2++;
  }
  CHECK(2, bad2 == 0);
  for (i = 0; i < 200; i++) {                     /* a peripheral read between a store and its read-back, in another line */
    buf[i & 15] = i;
    s = UART1_STATUS;
    buf[16 + (i & 15)] = (unsigned char)~i;
    s = UART1_STATUS;
    if (buf[i & 15] != i || buf[16 + (i & 15)] != (unsigned char)~i) bad3++;
  }
  CHECK(3, bad3 == 0);
  for (i = 0; i < 200; i++) {                     /* a read-modify-write, then a peripheral read */
    buf[i & 15] = i;
    buf[i & 15]++;
    s = UART1_STATUS;
    if (buf[i & 15] != (unsigned char)(i + 1)) bad4++;
  }
  CHECK(4, bad4 == 0);
  for (i = 0; i < 200; i++) {                     /* the monitor's shape: store, store to a counter, poll, poll */
    buf[i & 15] = i;
    tmp = i;
    s = UART1_STATUS;
    s = UART1_STATUS;
    if (buf[i & 15] != i || tmp != i) bad5++;
  }
  CHECK(5, bad5 == 0);
  tmp = s;
  DONE();
  return 0;
}
