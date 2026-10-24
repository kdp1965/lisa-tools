/* hw_ws2812_miss.c - the WS2812B driver (ttsky26d RESPIN 33) fed through
 * data cache misses: 200-pixel frames with the pixels from constants (mode
 * 0), read in order from a buffer in the PSRAM (mode 1: a 16-byte line fill
 * every 5 1/3 pixels), and the worst case (mode 2: each pixel read from a
 * line that a write 8 KB away - the same one of the cache's 8 direct-mapped
 * lines - has just made dirty: a write-back and two fills a pixel).  Per
 * mode: UNDERRUN and the frame's time from Timer 3 in 64-clock ticks (a
 * pixel is 1800 clocks at TBIT 74, 200 of them and the reset 5625 + 220;
 * the loops are as an application would write them, nothing timed inside).  Build: sdcc -mlisa --rev3 (make
 * hw_ws2812_miss_rev3.ihx REV3=1); arty_run.py CACHE=1 [SPI=0x....]. */
#include "harness.h"
#include <tt07.h>

__sfr __at(0x22C) TIDX;
__sfr __at(0x22D) TDATA;
__sfr __at(0x240) WS_CTRL;
__sfr __at(0x241) WS_STAT;
__sfr __at(0x242) WS_T0H;
__sfr __at(0x243) WS_T1H;
__sfr __at(0x244) WS_TBIT;
__sfr __at(0x245) WS_TRST;

#define EN       0x01
#define PIN(n)   ((n) << 1)
#define IE       0x10

#define FULL_A   0x01
#define FULL_B   0x02
#define BUSY     0x04
#define DONE_F   0x08
#define UNDERRUN 0x10

unsigned char ws_half;          /* the half filled next: 0 A, 1 B */

/* ws_put(g, r, b): a pixel into the next half once it is free.  g comes
   in A, r and b at 0(sp) / 1(sp) (sdcc's convention); after the push of g
   they are g 0(sp), r 1(sp), b 2(sp).  A leaf: RA untouched, ret. */
static void ws_put(unsigned char g, unsigned char r, unsigned char b) __naked
{
  (void) g; (void) r; (void) b;
  __asm
    push  a
    lda   _ws_half
    bnz   00002$
00001$:
    lda   _WS_STAT              ; half A: wait for FULL_A clear
    andi  #0x01
    bnz   00001$
    ldi   #0x00
    sta   _TIDX
    ldax  0(sp)
    sta   _TDATA
    ldax  1(sp)
    sta   _TDATA
    ldax  2(sp)
    sta   _TDATA
    ldi   #0x01
    sta   _WS_STAT              ; FULL_A
    sta   _ws_half              ; B next
    ads   #1
    ret
00002$:
    lda   _WS_STAT              ; half B: wait for FULL_B clear
    andi  #0x02
    bnz   00002$
    ldi   #0x03
    sta   _TIDX
    ldax  0(sp)
    sta   _TDATA
    ldax  1(sp)
    sta   _TDATA
    ldax  2(sp)
    sta   _TDATA
    ldi   #0x02
    sta   _WS_STAT              ; FULL_B
    ldi   #0x00
    sta   _ws_half              ; A next
    ads   #1
    ret
  __endasm;
}


#define NPIX 200
#ifndef TBIT
#define TBIT 74
#endif

static unsigned char buf[16384 + 3];

static unsigned int cnt(void)
{
  unsigned char lo = T3_CNT_L;
  return ((unsigned int)T3_CNT_H << 8) | lo;
}

/* the frame's clocks / 64 (Timer 3's prescaler 63); a pixel is 1800 */
static unsigned int ticks;

static unsigned char frame(unsigned char mode)
{
  unsigned char *p = buf;
  unsigned char n, g, st;
  unsigned int t;

  WS_STAT = UNDERRUN;
  ws_half = 0;
  t = cnt();
  if (mode == 0) {
    for (n = 0; n < NPIX; n++)
      ws_put(n, n + 1, n + 2);
  } else if (mode == 1) {
    for (n = 0; n < NPIX; n++) {
      ws_put(p[0], p[1], p[2]);
      p += 3;
    }
  } else {
    for (n = 0; n < NPIX; n++) {
      g = p[0];
      ws_put(g, p[1], p[2]);
      p[8192] = g;                /* the same cache line: dirty, evicted by the next read */
      p += 128;
      if (p >= buf + 8192)
        p = buf;
    }
  }
  while (!(WS_STAT & DONE_F))
    ;
  ticks = cnt() - t;
  st = WS_STAT;
  puts("mode "); puthex(mode);
  puts(" frame x64 clocks "); puthex(ticks);
  puts(st & UNDERRUN ? " UNDERRUN\n" : " no underrun\n");
  return !(st & UNDERRUN);
}

void main(void)
{
  unsigned int i;
  unsigned char m, ok;

  puts("ws2812 through cache misses\n");
  for (i = 0; i < sizeof buf; i++)
    buf[i] = (unsigned char)(i ^ (i >> 7));
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  WS_TBIT = TBIT;
  WS_CTRL = EN | PIN(1);
  for (m = 0; m < 3; m++) {
    ok = frame(m);
    CHECK(m + 1, ok);
  }
  DONE();
  for (;;)
    ;
}
