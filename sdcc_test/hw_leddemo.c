/* hw_leddemo.c - an Adafruit NeoPixel Jewel (7 WS2812B, G R B) on uo_out[0]
 * driven by the third chip's WS2812B driver with DMA (ttsky26d RESPIN 35),
 * the patterns of Ken's TinyQV prism-test/leddemo.c.  Each frame is the 7
 * pixels in a buffer on a 256-byte boundary in the PSRAM: START, wait for
 * FRAME, then the pattern's delay (Timer 3: prescaler 63, a tick 64 clocks,
 * 781 ticks a ms at 50 MHz).  uo_out[0] must be in mux 3 (debug register
 * 0x1b bits 1:0 = 3: arty_run.py REGS=0x1b=0x5457 after its 0x5455).
 * Build: sdcc -mlisa --rev3 (make hw_leddemo_rev3.ihx REV3=1); run with
 * arty_run.py CACHE=1 STOP= (it never ends). */
#include "harness.h"
#include <tt07.h>

__sfr __at(0x22C) TIDX;
__sfr __at(0x22D) TDATA;

#define LED_COUNT        7
#define BRIGHTNESS_FULL  256
#define BUF              ((unsigned char *)0x10000)   /* BASE 0x100 */

static unsigned int gBrightness = 64;   /* 8.8 fixed point: 256 = 100% */

static void delay_ms(unsigned int ms)
{
  unsigned int t;
  while (ms--) {
    t = T3_CNT_L;
    t |= (unsigned int)T3_CNT_H << 8;
    for (;;) {
      unsigned int n = T3_CNT_L;
      n |= (unsigned int)T3_CNT_H << 8;
      if ((unsigned int)(n - t) >= 781)
        break;
    }
  }
}

static void flush_leds(unsigned int time_ms)
{
  WS_STAT = WS_FRAME;
  WS_STAT = WS_START;                   /* the cache is flushed, the 7 pixels sent */
  while (!(WS_STAT & WS_FRAME))
    ;
  delay_ms(time_ms);
}

/* HSV (h 0-359, s, v 0-255) to RGB */
static void hsv2rgb(unsigned int h, unsigned char s, unsigned char v,
                    unsigned char *out_r, unsigned char *out_g, unsigned char *out_b)
{
  unsigned char r, g, b, i, p, q, t;
  unsigned int f;

  if (s == 0) {
    r = g = b = v;
  } else {
    h %= 360;
    i = (unsigned char)(h / 60);
    f = ((h % 60) * 255u) / 60;          /* 0-255 */
    p = (unsigned char)(((unsigned int)v * (255 - s)) / 255);
    q = (unsigned char)(((unsigned int)v * (255 - ((unsigned int)s * f) / 255)) / 255);
    t = (unsigned char)(((unsigned int)v * (255 - ((unsigned int)s * (255 - f)) / 255)) / 255);
    switch (i) {
    case 0:  r = v; g = t; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = t; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
  }
  *out_r = r;
  *out_g = g;
  *out_b = b;
}

/* a pixel scaled by gBrightness, G R B in the buffer */
static void set_color(unsigned char idx, unsigned char r, unsigned char g, unsigned char b)
{
  unsigned char *p = BUF + 3 * idx;
  p[0] = (unsigned char)(((unsigned int)g * gBrightness) >> 8);
  p[1] = (unsigned char)(((unsigned int)r * gBrightness) >> 8);
  p[2] = (unsigned char)(((unsigned int)b * gBrightness) >> 8);
}

static void set_all(unsigned char r, unsigned char g, unsigned char b)
{
  unsigned char i;
  for (i = 0; i < LED_COUNT; i++)
    set_color(i, r, g, b);
}

static const unsigned char palette[6][3] = {
  {255,   0,   0},    /* red */
  {255, 128,   0},    /* yellow */
  {  0,   0, 255},    /* blue */
  {  0, 255,   0},    /* green */
  {255,   0, 128},    /* purple */
  {  0,   0,   0},    /* black */
};

/* 1: a colour-wipe fade, red -> green -> blue */
static void pattern_wipe(void)
{
  static const unsigned char colors[3][3] = { {255, 0, 0}, {0, 255, 0}, {0, 0, 255} };
  unsigned char stage, t;
  const unsigned char *c1, *c2;

  for (stage = 0; stage < 3; stage++) {
    c1 = colors[stage];
    c2 = colors[(stage + 1) % 3];
    for (t = 0; t <= 100; t++) {
      set_all((unsigned char)(((100u - t) * c1[0] + (unsigned int)t * c2[0]) / 100),
              (unsigned char)(((100u - t) * c1[1] + (unsigned int)t * c2[1]) / 100),
              (unsigned char)(((100u - t) * c1[2] + (unsigned int)t * c2[2]) / 100));
      flush_leds(20);
    }
  }
}

/* 2: a rainbow chase */
static void pattern_rainbow(void)
{
  unsigned int cycle;
  unsigned char i, r, g, b;

  for (cycle = 0; cycle < 360; cycle += 2) {
    for (i = 0; i < LED_COUNT; i++) {
      hsv2rgb((cycle + (360u * i) / LED_COUNT) % 360, 255, 128, &r, &g, &b);
      set_color(i, r, g, b);
    }
    flush_leds(30);
  }
}

/* 3: a fade between two colours */
static void pattern_fade_two(const unsigned char *c1, const unsigned char *c2)
{
  unsigned char t;

  for (t = 0; t <= 200; t++) {
    set_all((unsigned char)(((200u - t) * c1[0] + (unsigned int)t * c2[0]) / 200),
            (unsigned char)(((200u - t) * c1[1] + (unsigned int)t * c2[1]) / 200),
            (unsigned char)(((200u - t) * c1[2] + (unsigned int)t * c2[2]) / 200));
    flush_leds(15);
  }
}

/* 4: circle the moon (the six outer LEDs 1-6 around the centre 0) */
static void pattern_circle(unsigned char direction)
{
  unsigned char color, x, i, led, prev, prev2;
  const unsigned char *c;

  for (color = 0; color < 5; color++) {
    c = palette[color];
    set_all(0, 0, 0);
    for (x = 0; x < 3; x++) {
      for (i = 1; i < LED_COUNT; i++) {
        led = direction ? LED_COUNT - i : i;
        prev = direction ? led + 1 : led - 1;
        if (prev < 1) prev = 6;
        if (prev > 6) prev = 1;
        prev2 = direction ? prev + 1 : prev - 1;
        if (prev2 < 1) prev2 = 6;
        if (prev2 > 6) prev2 = 1;
        set_color(led, c[0], c[1], c[2]);
        flush_leds(80);
        set_color(led, c[0] / 3, c[1] / 3, c[2] / 3);
        set_color(prev, c[0] / 6, c[1] / 6, c[2] / 6);
        set_color(prev2, 0, 0, 0);
      }
    }
  }
}

/* 5: star crossed */
static void pattern_star_crossed(void)
{
  unsigned char color, i, led = 0;
  const unsigned char *c;

  set_all(0, 0, 0);
  for (color = 0; color < 5; color++) {
    c = palette[color];
    for (i = 0; i < LED_COUNT + 2; i++) {
      set_color(led % 6 + 1, 0, 0, 0);
      led += 3;
      set_color(led % 6 + 1, c[0], c[1], c[2]);
      flush_leds(100);

      set_color(led % 6 + 1, 0, 0, 0);
      led -= 2;
      set_color(led % 6 + 1, c[0], c[1], c[2]);
      flush_leds(100);
    }
  }
}

/* 6: zip */
static void pattern_zip(void)
{
  unsigned char color, i;
  unsigned int save = gBrightness;
  const unsigned char *c;

  set_all(0, 0, 0);
  for (color = 0; color < 6; color++) {
    c = palette[color];
    for (i = 1; i < LED_COUNT; i++) {
      set_color(i, c[0], c[1], c[2]);
      flush_leds(120);
    }
    gBrightness = BRIGHTNESS_FULL;
    set_color(1, c[0], c[1], c[2]);
    flush_leds(40);
    gBrightness = save;
  }
}

static const unsigned char magenta[3] = {255, 0, 255};
static const unsigned char cyan[3]    = {0, 255, 255};

void main(void)
{
  puts("led demo: 7 WS2812B on uo_out[0], DMA\n");
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  TIDX = 0;
  TDATA = 0x00; TDATA = 0x01;           /* BASE 0x100: the buffer at 0x10000 */
  TDATA = LED_COUNT; TDATA = 0;         /* LEN */
  WS_CTRL = WS_EN | WS_PIN(0) | WS_DMA;
  set_all(0, 0, 0);
  flush_leds(1);

  for (;;) {
    puts("zip\n");          pattern_zip();
    puts("star crossed\n"); pattern_star_crossed();
    puts("wipe\n");         pattern_wipe();
    puts("circle\n");       pattern_circle(1);
    puts("rainbow\n");      pattern_rainbow();
    puts("fade\n");         pattern_fade_two(magenta, cyan);
    puts("circle\n");       pattern_circle(0);
    if (WS_STAT & WS_UNDERRUN) {
      puts("UNDERRUN\n");
      WS_STAT = WS_UNDERRUN;
    }
  }
}
