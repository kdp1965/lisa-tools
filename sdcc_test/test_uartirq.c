/* test_uartirq.c - the UART1 interrupts of the second chip.  RESPIN 25: the
 * enables at 0x214 had no write decode in either chip's RTL (they read 0 and
 * no UART interrupt ever fired - found when NuttX's shell took no input), and
 * the requests are levels since 2026-10-18 (Ken): INT_STATUS bit 2 is
 * "a byte waits and the receive interrupt is enabled, or the transmitter is
 * empty and its interrupt is enabled", not an edge latched until it is
 * acknowledged - a byte already waiting when the enable is set interrupts at
 * once, and writing the bit in INT_STATUS clears nothing.  The bench types
 * "Hi" (lisa_sim --send Hi --send-at <cycles>, the testbench SEND=Hi
 * SEND_DELAY=<clocks>, arty_run's SEND).  sdcc --rev2 only. */
#include "harness.h"
#include <tt07.h>

volatile unsigned char tx_ints, rx_n;
volatile unsigned char rx_buf[4];

void uart1_isr(void) __interrupt(LISA_INT_UART1)
{
  while ((UART1_STATUS & 0x01) != 0)    /* the request stays up until the FIFO is empty */
    {
      unsigned char c = UART1_DATA;
      if (rx_n < 4)
        rx_buf[rx_n] = c;
      rx_n++;
    }
  if ((UART_INT_EN & 0x02) != 0 && (UART1_STATUS & 0x02) != 0)
    {
      tx_ints++;
      UART_INT_EN &= ~0x02;             /* nothing to send: drop the request */
    }
}

int main(void)
{
  unsigned int t;
  unsigned char u, v, w;

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

  /* transmit, a level: once the transmitter is empty (the last line's
   * character takes a while to leave), enabling its interrupt requests one
   * at once (the handler drops the enable again) */
  while ((UART1_STATUS & 0x02) == 0)
    ;
  UART_INT_EN = 0x02;
  v = INT_STATUS;
  INT_STATUS = INT_UART1;               /* acknowledging a level clears nothing */
  w = INT_STATUS;                       /* (both read before anything is printed:
                                         * a print makes the transmitter busy) */
  CHECK(3, (v & INT_UART1) != 0);
  CHECK(4, (w & INT_UART1) != 0);
  lisa_ei();
  for (t = 0; t < 1000 && tx_ints == 0; t++)
    ;
  CHECK(5, tx_ints == 1);
  v = INT_STATUS;
  CHECK(6, (v & INT_UART1) == 0);       /* the enable dropped, the request with it */
  /* enabled while a byte is on its way out: the interrupt when it has left */
  lisa_di();
  putc('.');
  UART_INT_EN = 0x02;
  lisa_ei();
  for (t = 0; t < 20000 && tx_ints == 1; t++)
    ;
  CHECK(7, tx_ints == 2);
  putc('\n');

  /* receive: wait for the typed bytes with the interrupt off (arty_run types
   * 0.6 s in: a few seconds of patience on the chip), then enable it - a
   * byte already waiting must interrupt at once */
  lisa_di();
  for (t = 0; t < 60000 && (UART1_STATUS & 0x01) == 0; t++)
    for (u = 0; u < 10 && (UART1_STATUS & 0x01) == 0; u++)
      ;
  UART_INT_EN = 0x01;
  v = INT_STATUS;
  CHECK(8, (v & INT_UART1) != 0);       /* the level: data waits, enable set */
  lisa_ei();
  for (t = 0; t < 20000 && rx_n < 2; t++)
    ;
  lisa_di();
  v = INT_STATUS;
  UART_INT_EN = 0;
  INT_ENABLE = 0;
  puts("rx "); puthex(rx_n); putc(' ');
  puthex(rx_buf[0]); putc(' '); puthex(rx_buf[1]); putc('\n');
  CHECK(9, rx_n == 2);
  CHECK(10, rx_buf[0] == 'H' && rx_buf[1] == 'i');
  CHECK(11, (v & INT_UART1) == 0);      /* drained: the request is gone */
  DONE();
  return 0;
}
