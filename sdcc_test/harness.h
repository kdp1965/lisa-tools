/*
 * harness.h - minimal test harness for the SDCC LISA port.
 *
 * Output goes to UART1 (data address 0x210/0x211 = peripheral 0x10/0x11),
 * which lisa_sim -b echoes to stdout and the TT07 board forwards through
 * the debug UART.  Each CHECK prints "ok NNNN" or "FAIL NNNN"; main()
 * finishes with "ALL PASSED" or "SOME FAILED".
 */
#ifndef HARNESS_H
#define HARNESS_H

__sfr __at(0x210) UART_TX;
__sfr __at(0x211) UART_STATUS;

static void putc(char c)
{
  while (!(UART_STATUS & 2))
    ;
  UART_TX = c;
}

static void puts(const char *s)
{
  while (*s)
    putc(*s++);
}

static void puthex(unsigned int v)
{
  const char *h = "0123456789abcdef";
  putc(h[(v >> 12) & 15]);
  putc(h[(v >> 8) & 15]);
  putc(h[(v >> 4) & 15]);
  putc(h[v & 15]);
}

static unsigned char fails;

#define CHECK(n, cond)                                  \
  do {                                                  \
    if (cond) puts("ok "); else { puts("FAIL "); fails++; } \
    puthex(n);                                          \
    putc('\n');                                         \
  } while (0)

#define DONE() puts(fails ? "SOME FAILED\n" : "ALL PASSED\n")

#endif
