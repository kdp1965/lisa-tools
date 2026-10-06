/* the sdcc_hello main loop: on the chip it printed sum: 1 for '?' 's' */
#include "harness.h"
__sfr __at(0x201) PORTB;
__sfr __at(0x20c) TIMER1_CTRL;
#define TIMER_ROLLOVER 0x80
volatile unsigned char roll;            /* stands in for TIMER1_CTRL */
volatile unsigned char rx_avail, rx_data;
int putchar(int c) { putc(c); return c; }
int main(void)
{
  unsigned int count = 0;
  unsigned int sum = 0;
  unsigned char ticks = 0;
  unsigned char c;
  unsigned char n = 0;
  PORTB = 0x4f;
  for (;;)
    {
      count++;
      roll = (count & 3) == 0 ? TIMER_ROLLOVER : 0;
      if (roll & TIMER_ROLLOVER)
        {
          if (++ticks == 4)
            {
              ticks = 0;
              PORTB ^= 0x08;
            }
        }
      if ((count & 63) == 5) { rx_avail = 1; rx_data = n == 0 ? '?' : 's'; }
      if (rx_avail)
        {
          rx_avail = 0;
          c = rx_data;
          putchar(c);
          sum += c;
          if (c == '\r')
            putchar('\n');
          if (c == 's')
            break;
          n++;
        }
    }
  putc('\n'); puthex(sum); putc('\n');
  CHECK(1, sum == '?' + 's');
  CHECK(2, count == 69);
  DONE();
  return 0;
}
