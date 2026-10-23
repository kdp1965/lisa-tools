/* hw_spiflash.c - the third chip's SPI master (ttsky26d RESPIN 29) against a
 * real SPI flash on the Arty: mux 2 (debug register 0x1b = 0x2880) puts CS0
 * on uo[3] (JB3), SCLK on uo[5] (JB5), MOSI on uo[6] (JB6); MISO from
 * ui_in[0] (JA0, CTRL[6:5] = 0).  Read only: the JEDEC ID (9F), the
 * manufacturer / device ID (90, AB), the status register (05) and the first
 * bytes of the array (03), at several clock dividers; every divider must
 * read the same.  Build: sdcc -mlisa --rev3 (make hw_spiflash_rev3.ihx REV3=1). */
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

static unsigned char id[3], mid[2], rid, sr, dat[16];

static void probe(void)
{
  unsigned char i;

  SPI_CTRL = CS0;
  xfer(0x9f);
  for (i = 0; i < 3; i++) id[i] = xfer(0);
  SPI_CTRL = 0;

  SPI_CTRL = CS0;
  xfer(0x90); xfer(0); xfer(0); xfer(0);
  mid[0] = xfer(0); mid[1] = xfer(0);
  SPI_CTRL = 0;

  SPI_CTRL = CS0;
  xfer(0xab); xfer(0); xfer(0); xfer(0);
  rid = xfer(0);
  SPI_CTRL = 0;

  SPI_CTRL = CS0;
  xfer(0x05);
  sr = xfer(0);
  SPI_CTRL = 0;

  SPI_CTRL = CS0;
  xfer(0x03); xfer(0); xfer(0); xfer(0);
  for (i = 0; i < 16; i++) dat[i] = xfer(0);
  SPI_CTRL = 0;
}

static const unsigned char DIVS[4] = { 24, 7, 3, 1 };

void main(void)
{
  unsigned char d, i, ref[3], same = 1;

  for (d = 0; d < 4; d++) {
    SPI_DIV = DIVS[d];
    probe();
    puts("div "); put2(DIVS[d]);
    puts(" 9f "); put2(id[0]); put2(id[1]); put2(id[2]);
    puts(" 90 "); put2(mid[0]); put2(mid[1]);
    puts(" ab "); put2(rid);
    puts(" sr "); put2(sr);
    puts(" @0");
    for (i = 0; i < 16; i++) { putc(' '); put2(dat[i]); }
    putc('\n');
    if (d == 0)
      for (i = 0; i < 3; i++) ref[i] = id[i];
    else
      for (i = 0; i < 3; i++) if (id[i] != ref[i]) same = 0;
  }
  CHECK(1, ref[0] != 0x00 && ref[0] != 0xff);   /* something answers */
  CHECK(2, same);                                 /* at every divider */
  DONE();
  for (;;)
    ;
}
