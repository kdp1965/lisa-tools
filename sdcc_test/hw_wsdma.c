/* hw_wsdma.c - the WS2812B driver's DMA (ttsky26d RESPIN 35) on the Arty:
 * 200-pixel frames from a buffer in the PSRAM at the datasheet's bit (TBIT
 * 62, 1.26 us; a pixel 1512 clocks), the core idle (mode 0) and thrashing
 * the data cache (mode 1: every access a dirty write-back and a fill, the
 * QSPI bus busy), then with REPEAT (mode 2: four frames counted by FRAME).
 * Per mode: UNDERRUN, and the frame's time from Timer 3 in 64-clock ticks
 * against 200 pixels and the reset (4725 + 220 = 4945 ticks); the core's
 * thrash count shows it kept running.  Build: sdcc -mlisa --rev3 (make
 * hw_wsdma_rev3.ihx REV3=1); arty_run.py CACHE=1 [SPI=0x....]. */
#include "harness.h"
#include <tt07.h>

__sfr __at(0x22C) TIDX;
__sfr __at(0x22D) TDATA;

#define NPIX   200
#define BUF    ((unsigned char *)0x10000)
#define THRASH ((volatile unsigned char *)0x20000)

static unsigned int cnt(void)
{
  unsigned char lo = T3_CNT_L;
  return ((unsigned int)T3_CNT_H << 8) | lo;
}

static void report(unsigned char mode, unsigned int ticks, unsigned int n)
{
  puts("mode "); puthex(mode);
  puts(" ticks "); puthex(ticks);
  puts(" thrash "); puthex(n);
  puts(WS_STAT & WS_UNDERRUN ? " UNDERRUN\n" : " no underrun\n");
}

void main(void)
{
  unsigned int i, n, t, ticks;
  unsigned char m, ok = 1, frames;

  puts("ws2812 dma\n");
  for (i = 0; i < NPIX * 3; i++)
    BUF[i] = (unsigned char)(i ^ (i >> 3));
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  TIDX = 0;
  TDATA = 0x00; TDATA = 0x01;               /* BASE 0x100: 0x10000 */
  TDATA = NPIX & 0xff; TDATA = NPIX >> 8;   /* LEN */
  WS_CTRL = WS_EN | WS_PIN(1) | WS_DMA;

  for (m = 0; m < 2; m++) {
    WS_STAT = WS_UNDERRUN | WS_FRAME;
    n = 0;
    t = cnt();
    WS_STAT = WS_START;
    while (!(WS_STAT & WS_FRAME))
      if (m) {
        THRASH[(n & 31) << 7] += 1;         /* one cache line, 32 tags */
        n++;
      }
    ticks = cnt() - t;
    report(m, ticks, n);
    CHECK(m + 1, !(WS_STAT & WS_UNDERRUN));
    ok &= !(WS_STAT & WS_UNDERRUN);
  }

  /* REPEAT: four frames by FRAME, the thrash going on */
  WS_STAT = WS_UNDERRUN | WS_FRAME;
  WS_CTRL = WS_EN | WS_PIN(1) | WS_DMA | WS_REPEAT;
  frames = 0;
  n = 0;
  t = cnt();
  WS_STAT = WS_START;
  while (frames < 4) {
    if (WS_STAT & WS_FRAME) {
      WS_STAT = WS_FRAME;
      frames++;
    }
    THRASH[(n & 31) << 7] += 1;
    n++;
  }
  WS_CTRL = WS_EN | WS_PIN(1) | WS_DMA;
  ticks = cnt() - t;
  while (WS_STAT & WS_ACTIVE)
    ;
  report(2, ticks, n);
  CHECK(3, !(WS_STAT & WS_UNDERRUN));
  DONE();
  for (;;)
    ;
}
