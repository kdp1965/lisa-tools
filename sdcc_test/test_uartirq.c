/* test_uartirq.c - the UART1 interrupts of the second chip (RESPIN 25: the
 * enables at 0x214 had no write decode in either chip's RTL - they read 0 and
 * no UART interrupt ever fired; found when NuttX's shell took no input).
 * The enables written and read back; a transmit-buffer-empty interrupt after
 * a byte goes out; two bytes typed by the bench ("Hi": lisa_sim --send Hi
 * --send-at <cycles>, the testbench SEND=Hi SEND_DELAY=<clocks>, arty_run's
 * SEND) through the receive interrupt.  The request is the edge of the
 * condition while the enable is set, acknowledged in INT_STATUS first, as a
 * driver has to.  sdcc --rev2 only. */
#include "harness.h"
#include <tt07.h>

volatile unsigned char tx_ints, rx_n;
volatile unsigned char rx_buf[4];

void uart1_isr(void) __interrupt(LISA_INT_UART1)
{
  INT_STATUS = INT_UART1;               /* first: a byte arriving during the drain raises the next */
  while ((UART1_STATUS & 0x01) != 0)
    {
      unsigned char c = UART1_DATA;
      if (rx_n < 4)
        rx_buf[rx_n] = c;
      rx_n++;
    }
  if ((UART_INT_EN & 0x02) != 0 && (UART1_STATUS & 0x02) != 0)
    {
      tx_ints++;
      UART_INT_EN &= ~0x02;             /* one transmit interrupt per enable */
    }
}

int main(void)
{
  unsigned int t;
  unsigned char u, v;

  INT_ENABLE = 0;
  UART_INT_EN = 0x03;
  v = UART_INT_EN;
  CHECK(1, v == 0x03);                  /* the enables can be written ... */
  UART_INT_EN = 0x00;
  v = UART_INT_EN;
  CHECK(2, v == 0x00);                  /* ... and cleared */

  tx_ints = 0;
  rx_n = 0;
  INT_STATUS = 0xff;
  INT_ENABLE = INT_UART1;
  lisa_ei();

  /* transmit: enable the interrupt, send a byte, the buffer empties */
  UART_INT_EN = 0x02;
  putc('.');
  for (t = 0; t < 20000 && tx_ints == 0; t++)
    ;
  CHECK(3, tx_ints == 1);
  putc('\n');

  /* receive: the bench types "Hi" once the program is running (arty_run
   * types 0.6 s in: a few seconds of patience on the chip) */
  UART_INT_EN = 0x01;
  for (t = 0; t < 60000 && rx_n < 2; t++)
    for (u = 0; u < 10 && rx_n < 2; u++)
      ;
  lisa_di();
  UART_INT_EN = 0;
  INT_ENABLE = 0;
  puts("rx "); puthex(rx_n); putc(' ');
  puthex(rx_buf[0]); putc(' '); puthex(rx_buf[1]); putc('\n');
  CHECK(4, rx_n == 2);
  CHECK(5, rx_buf[0] == 'H' && rx_buf[1] == 'i');
  DONE();
  return 0;
}
