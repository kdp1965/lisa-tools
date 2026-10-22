/* test_spiflash.c - the third chip's SPI master (ttsky26d RESPIN 29,
 * src/lisa_spi.v, [p:0x28-0x2B]) with a SPI NOR flash on CS0: lisa_sim
 * --rev3 --spi-flash FILE (lisa_sim/spi.h, the W25Q128JV's commands).
 * The JEDEC ID, the write enable in the status, a 4K erase, a program that
 * wraps inside its 256-byte page and one across two pages, the data back
 * through read (03) and fast read (0B), DONE and its interrupt request
 * (INT_STATUS bit 7). REV3=1 only. */
#include "harness.h"

__sfr __at(0x20F) INT_STATUS;
__sfr __at(0x228) SPI_DATA;
__sfr __at(0x229) SPI_CTRL;
__sfr __at(0x22A) SPI_DIV;
__sfr __at(0x22B) SPI_STATUS;

#define CS0   0x01
#define IE    0x80

static unsigned char xfer(unsigned char b)
{
  SPI_DATA = b;
  while (SPI_STATUS & 1)
    ;
  return SPI_DATA;
}

static void cmd_addr(unsigned char c, unsigned long a)
{
  xfer(c);
  xfer(a >> 16);
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

static void wren(void)
{
  SPI_CTRL = CS0;
  xfer(0x06);
  SPI_CTRL = 0;
}

static void program(unsigned long a, unsigned int n, unsigned char seed)
{
  unsigned int i;
  wren();
  SPI_CTRL = CS0;
  cmd_addr(0x02, a);
  for (i = 0; i < n; i++)
    xfer(seed + i);
  SPI_CTRL = 0;
}

void main(void)
{
  unsigned int i;
  unsigned char a, b, c, bad;

  SPI_DIV = 1;
  CHECK(1, SPI_DIV == 1 && SPI_STATUS == 0);

  SPI_CTRL = CS0;                          /* the JEDEC ID */
  xfer(0x9f);
  a = xfer(0); b = xfer(0); c = xfer(0);
  SPI_CTRL = 0;
  CHECK(2, a == 0xef && b == 0x40 && c == 0x18);

  CHECK(3, (status() & 2) == 0);           /* the write enable */
  wren();
  CHECK(4, (status() & 2) == 2);

  SPI_CTRL = CS0;                          /* erase the 4K sector at 0x12000 */
  cmd_addr(0x20, 0x12345);
  SPI_CTRL = 0;
  CHECK(5, (status() & 2) == 0);

  program(0x12000, 300, 0x40);             /* wraps: 0x12000-0x120ff, then 0x12000-0x1202b again (ANDed) */
  program(0x12180, 256, 0x11);             /* across 0x12180-0x121ff and wraps to 0x12100 */

  bad = 0;
  SPI_CTRL = CS0;
  cmd_addr(0x03, 0x12000);
  for (i = 0; i < 0x200; i++) {
    unsigned char v = xfer(0), e;
    if (i < 44)
      e = (unsigned char)(0x40 + i) & (unsigned char)(0x40 + 256 + i);
    else if (i < 0x100)
      e = 0x40 + i;
    else if (i < 0x180)
      e = 0x11 + (i - 0x100) + 0x80;       /* the second program's wrapped half */
    else
      e = 0x11 + (i - 0x180);
    if (v != e)
      bad++;
  }
  SPI_CTRL = 0;
  CHECK(6, bad == 0);

  SPI_CTRL = CS0;                          /* fast read: a dummy byte after the address */
  cmd_addr(0x0b, 0x12181);
  xfer(0);
  a = xfer(0); b = xfer(0);
  SPI_CTRL = 0;
  CHECK(7, a == 0x12 && b == 0x13);

  SPI_CTRL = CS0;                          /* the rest of the sector: still erased */
  cmd_addr(0x03, 0x12ffe);
  a = xfer(0); b = xfer(0);
  SPI_CTRL = 0;
  CHECK(8, a == 0xff && b == 0xff);

  SPI_CTRL = IE;                           /* DONE and its request, cleared by reading DATA */
  SPI_DATA = 0x5a;
  while (SPI_STATUS & 1)
    ;
  a = SPI_STATUS;
  b = INT_STATUS;
  c = SPI_DATA;
  CHECK(9, (a & 2) && (b & 0x80) && c == 0xff && !(SPI_STATUS & 2) && !(INT_STATUS & 0x80));
  SPI_CTRL = 0;

  DONE();
}
