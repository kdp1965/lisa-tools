/* hw_spiflash_wr.c - write test of a real SPI flash (an SST25VF064C on the
 * Arty) through the third chip's SPI master (ttsky26d RESPIN 29), pins as
 * hw_spiflash.c (0x1b = 0x2880, MISO on ui_in[0]).  Sector 0 is erased and
 * rewritten (Ken: its data doesn't matter):
 *   1 the status register's block protection cleared (EWSR 50, WRSR 01 00)
 *   2 a 4K sector erase (20) of sector 0, BUSY polled, all 4096 bytes FF
 *   3 16 single-byte programs (02) at 0x000, read back
 *   4 a 256-byte page program (02) at 0x100, read back (03) and fast read (0B)
 *   5 the bytes outside what was programmed still FF
 * Build: sdcc -mlisa --rev3 --stack-loc 0x1fffff (run with CACHE=1). */
#include "harness.h"

__sfr __at(0x228) SPI_DATA;
__sfr __at(0x229) SPI_CTRL;
__sfr __at(0x22A) SPI_DIV;
__sfr __at(0x22B) SPI_STATUS;

#define CS0   0x01

static unsigned char xfer(unsigned char b)
{
  SPI_DATA = b;
  while (SPI_STATUS & 1)
    ;
  return SPI_DATA;
}

static void put2(unsigned char v)
{
  const char *h = "0123456789abcdef";
  putc(h[v >> 4]);
  putc(h[v & 15]);
}

static void cmd(unsigned char c)
{
  SPI_CTRL = CS0;
  xfer(c);
  SPI_CTRL = 0;
}

static void cmd_addr(unsigned char c, unsigned int a)   /* a 24-bit address, sector 0 only */
{
  xfer(c);
  xfer(0);
  xfer(a >> 8);
  xfer(a);
}

static unsigned char status(void)
{
  unsigned char s;
  SPI_CTRL = CS0;
  xfer(0x05);
  s = xfer(0);
  SPI_CTRL = 0;
  return s;
}

/* poll BUSY; the number of status reads it took */
static unsigned int wait_ready(void)
{
  unsigned int n = 0;
  while (status() & 1)
    n++;
  return n;
}

static unsigned char pat(unsigned int i)
{
  return (unsigned char)(i * 7 + (i >> 8) + 0x35);
}

void main(void)
{
  unsigned int i, n, bad;
  unsigned char s;

  SPI_DIV = 1;                               /* 12.5 MHz at 50 MHz */

  /* 1: unprotect */
  cmd(0x50);                                 /* EWSR */
  SPI_CTRL = CS0;
  xfer(0x01); xfer(0x00);                    /* WRSR 00 */
  SPI_CTRL = 0;
  wait_ready();
  s = status();
  puts("sr after wrsr "); put2(s); putc('\n');
  CHECK(1, (s & 0x3c) == 0);

  /* 2: erase sector 0 */
  cmd(0x06);
  s = status();
  SPI_CTRL = CS0;
  cmd_addr(0x20, 0);
  SPI_CTRL = 0;
  n = wait_ready();
  puts("erase: wel "); put2(s); puts(", status polls "); puthex(n); putc('\n');
  bad = 0;
  SPI_CTRL = CS0;
  cmd_addr(0x03, 0);
  for (i = 0; i < 4096; i++)
    if (xfer(0) != 0xff)
      bad++;
  SPI_CTRL = 0;
  puts("erased: not ff "); puthex(bad); putc('\n');
  CHECK(2, (s & 2) && n != 0 && bad == 0);

  /* 3: single-byte programs at 0x000-0x00f */
  for (i = 0; i < 16; i++) {
    cmd(0x06);
    SPI_CTRL = CS0;
    cmd_addr(0x02, i);
    xfer(pat(i));
    SPI_CTRL = 0;
    wait_ready();
  }
  bad = 0;
  SPI_CTRL = CS0;
  cmd_addr(0x03, 0);
  for (i = 0; i < 16; i++)
    if (xfer(0) != pat(i))
      bad++;
  SPI_CTRL = 0;
  puts("byte program: bad "); puthex(bad); putc('\n');
  CHECK(3, bad == 0);

  /* 4: a 256-byte page program at 0x100 */
  cmd(0x06);
  SPI_CTRL = CS0;
  cmd_addr(0x02, 0x100);
  for (i = 0x100; i < 0x200; i++)
    xfer(pat(i));
  SPI_CTRL = 0;
  n = wait_ready();
  bad = 0;
  SPI_CTRL = CS0;
  cmd_addr(0x03, 0x100);
  for (i = 0x100; i < 0x200; i++)
    if (xfer(0) != pat(i))
      bad++;
  SPI_CTRL = 0;
  puts("page program: polls "); puthex(n); puts(", bad "); puthex(bad); putc('\n');
  SPI_CTRL = CS0;
  cmd_addr(0x0b, 0x1f0);
  xfer(0);                                   /* fast read's dummy byte */
  puts("fast read 1f0:");
  n = 0;
  for (i = 0x1f0; i < 0x200; i++) {
    s = xfer(0);
    putc(' '); put2(s);
    if (s != pat(i))
      n++;
  }
  SPI_CTRL = 0;
  putc('\n');
  CHECK(4, bad == 0 && n == 0);

  /* 5: the rest still erased */
  bad = 0;
  SPI_CTRL = CS0;
  cmd_addr(0x03, 0);
  for (i = 0; i < 4096; i++) {
    s = xfer(0);
    if (i < 16 || (i >= 0x100 && i < 0x200))
      continue;
    if (s != 0xff)
      bad++;
  }
  SPI_CTRL = 0;
  puts("outside: not ff "); puthex(bad); putc('\n');
  CHECK(5, bad == 0);

  DONE();
  for (;;)
    ;
}
