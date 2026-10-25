/* hw_oled.c - an SSD1309 128x64 I2C OLED on the third chip's I2C master in
 * io mux mode 3 (ttsky26d: the custom Pmod's CE latch, SCL / SDA on uio[4]
 * / uio[5]; on the Arty SW2 up - fpga/arty_a7/m3_pmod.v - puts them on JD
 * pins 1 / 2).  400 kHz (prescale 24 at 50 MHz).  The OLED's address is
 * 0x3C (or 0x3D with its address jumper): both are probed and the one that
 * answers is used.  Then the SSD1309's init, a frame from a 1 KB buffer in
 * the PSRAM (a border, three lines of a 5x7 font, a checker strip) and a
 * bar moving across for a while, with the time a full-screen update takes
 * (Timer 3, 64-clock ticks).  Build: sdcc -mlisa --rev3 (make
 * hw_oled_rev3.ihx REV3=1); run with arty_run.py M3=1 CACHE=1. */
#include "harness.h"
#include <tt07.h>

#define STA   0x80
#define STO   0x40
#define WR    0x10
#define TIP   0x02
#define RXACK 0x80
#ifndef FRAMES
#define FRAMES 120                      /* the moving bar's (-DFRAMES=2 for the testbench) */
#endif

static unsigned char fb[1024];          /* 8 pages of 128 columns, bit 0 the top row */
static unsigned char addr;

static unsigned char i2c_byte(unsigned char b, unsigned char cmd)
{
  I2C_TX_DATA = b;
  I2C_CMD = cmd | WR;
  while (I2C_STATUS & TIP)
    ;
  return !(I2C_STATUS & RXACK);
}

static unsigned char probe(unsigned char a)
{
  return i2c_byte(a << 1, STA | STO);
}

/* a transaction: the control byte (0x00 commands, 0x40 data), then n bytes */
static unsigned char send(unsigned char ctl, const unsigned char *p, unsigned int n)
{
  unsigned char ok = i2c_byte(addr << 1, STA);
  ok &= i2c_byte(ctl, n ? 0 : STO);
  while (n--)
    ok &= i2c_byte(*p++, n ? 0 : STO);
  return ok;
}

static const unsigned char init_cmds[] = {
  0xAE,             /* display off */
  0xD5, 0xA0,       /* clock divide / oscillator */
  0xA8, 0x3F,       /* multiplex 64 */
  0xD3, 0x00,       /* display offset 0 */
  0x40,             /* start line 0 */
  0xA1,             /* segment remap: column 127 is SEG0 */
  0xC8,             /* COM scan from COM63 */
  0xDA, 0x12,       /* COM pins: alternative */
  0x81, 0x8F,       /* contrast */
  0xD9, 0x22,       /* precharge */
  0xDB, 0x34,       /* VCOMH */
  0xA4,             /* the display follows RAM */
  0xA6,             /* not inverted */
  0x20, 0x00,       /* horizontal addressing */
  0xAF              /* display on */
};

static const unsigned char window[] = { 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07 };

static const unsigned char font_digits[10][5] = {
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x42,0x61,0x51,0x49,0x46},
  {0x21,0x41,0x45,0x4B,0x31}, {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, {0x36,0x49,0x49,0x49,0x36},
  {0x06,0x49,0x49,0x29,0x1E}
};
static const unsigned char font_alpha[26][5] = {
  {0x7E,0x11,0x11,0x11,0x7E}, {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, {0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A}, {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F}, {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, {0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F}, {0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43}
};

/* a string (A-Z, 0-9, space) at page pg, column x, 6 columns a character */
static void text(unsigned char pg, unsigned char x, const char *s)
{
  const unsigned char *g;
  unsigned char i;

  for (; *s && x <= 122; s++, x += 6) {
    if (*s >= 'A' && *s <= 'Z')
      g = font_alpha[*s - 'A'];
    else if (*s >= '0' && *s <= '9')
      g = font_digits[*s - '0'];
    else
      continue;
    for (i = 0; i < 5; i++)
      fb[(unsigned int)pg * 128 + x + i] = g[i];
  }
}

static unsigned char flush(void)
{
  unsigned char ok = send(0x00, window, sizeof window);
  ok &= send(0x40, fb, sizeof fb);
  return ok;
}

static unsigned int cnt(void)
{
  unsigned char lo = T3_CNT_L;
  return ((unsigned int)T3_CNT_H << 8) | lo;
}

void main(void)
{
  unsigned int i, t, ticks;
  unsigned char ok, a3c, a3d, x, frame;

  puts("oled\n");
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  I2C_CTRL = 0;
  I2C_PRESCALE_LO = 24;
  I2C_PRESCALE_HI = 0;
  I2C_CTRL = 1;

  a3c = probe(0x3C);
  a3d = probe(0x3D);
  puts("probe 3c "); puts(a3c ? "ack" : "nak");
  puts(", 3d "); puts(a3d ? "ack\n" : "nak\n");
  CHECK(1, a3c || a3d);
  addr = a3c ? 0x3C : 0x3D;

  ok = send(0x00, init_cmds, sizeof init_cmds);
  CHECK(2, ok);

  for (i = 0; i < sizeof fb; i++)
    fb[i] = 0;
  for (i = 0; i < 128; i++) {           /* the border */
    fb[i] |= 0x01;
    fb[7 * 128 + i] |= 0x80;
  }
  for (i = 0; i < 8; i++) {
    fb[i * 128] = 0xFF;
    fb[i * 128 + 127] = 0xFF;
  }
  text(1, 34, "LISA REV 3");
  text(3, 31, "SSD1309 I2C");
  text(5, 19, "HELLO FROM LISA");
  for (i = 8; i < 120; i++)             /* a checker strip at the bottom */
    fb[6 * 128 + i] = (i & 4) ? 0xAA : 0x55;
  t = cnt();
  ok = flush();
  ticks = cnt() - t;
  puts("frame "); puthex(ticks); puts(" ticks\n");
  CHECK(3, ok);

  /* a bar across page 6 */
  ok = 1;
  for (frame = 0; frame < FRAMES; frame++) {
    x = 8 + frame % 112;
    for (i = 8; i < 120; i++)
      fb[6 * 128 + i] = (i >= x && i < x + 8) ? 0xFF : ((i & 4) ? 0xAA : 0x55);
    ok &= flush();
  }
  CHECK(4, ok);
  DONE();
  for (;;)
    ;
}
