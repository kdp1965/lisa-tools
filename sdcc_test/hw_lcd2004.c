/* hw_lcd2004.c - a 20x4 HD44780 character LCD on a PCF8574 I2C backpack,
 * through the chip's I2C master in io mux mode 3 (SCL / SDA on uio[4] /
 * uio[5]; the Arty's JD pins 1 / 2 with SW2 up), 100 kHz.  The backpack's
 * address is probed (0x20-0x27 PCF8574, 0x38-0x3F PCF8574A) and the first
 * that answers used.  The usual backpack wiring: P0 RS, P1 RW, P2 E, P3 the
 * backlight, P4-P7 D4-D7.  The HD44780's 4-bit init, four lines of text,
 * then a counter on line 4 for a while.  Build: make hw_lcd2004_rev3.ihx
 * REV3=1; arty_run.py M3=1 CACHE=1. */
#include "harness.h"
#include <tt07.h>

#define STA   0x80
#define STO   0x40
#define WR    0x10
#define TIP   0x02
#define RXACK 0x80

#define RS    0x01
#define EN    0x04
#define BL    0x08

#ifndef PRESCALE
#define PRESCALE 99                     /* 100 kHz at 50 MHz; 999 = 10 kHz */
#endif
#ifndef COUNTS
#define COUNTS 200
#endif

static unsigned char addr;

static unsigned char lines_or, lines_and = 0xff;   /* SCL bit 0, SDA bit 1 seen during bytes */

static unsigned char i2c_byte(unsigned char b, unsigned char cmd)
{
  unsigned char v;

  I2C_TX_DATA = b;
  I2C_CMD = cmd | WR;
  while (I2C_STATUS & TIP) {
    v = PORTC;                          /* uio[7:4]: SCL / SDA as the pins see them */
    lines_or |= v;
    lines_and &= v;
  }
  return !(I2C_STATUS & RXACK);
}

static unsigned char probe(unsigned char a)
{
  return i2c_byte(a << 1, STA | STO);
}

static void delay_us(unsigned int us)
{
  unsigned int t, c, n;

  n = (unsigned int)((unsigned long)us * 25 / 32) + 1;   /* 64-clock ticks of 1.28 us */
  t = T3_CNT_L; t |= (unsigned int)T3_CNT_H << 8;
  do {
    c = T3_CNT_L;
    c |= (unsigned int)T3_CNT_H << 8;
  } while ((unsigned int)(c - t) < n);
}

static unsigned char ok = 1;

/* one byte to the PCF8574's pins */
static void pins(unsigned char v)
{
  ok &= i2c_byte(addr << 1, STA);
  ok &= i2c_byte(v | BL, STO);
}

/* a 4-bit nibble (in bits 7:4) strobed on E */
static void nibble(unsigned char v)
{
  pins(v);
  pins(v | EN);
  pins(v);
  delay_us(50);
}

static void lcd_write(unsigned char b, unsigned char rs)
{
  nibble((b & 0xF0) | rs);
  nibble((unsigned char)(b << 4) | rs);
}

static void lcd_cmd(unsigned char c)
{
  lcd_write(c, 0);
  if (c <= 3)
    delay_us(2000);                     /* clear / home take 1.5 ms */
}

static const unsigned char row_addr[4] = { 0x00, 0x40, 0x14, 0x54 };

static void lcd_at(unsigned char row, unsigned char col)
{
  lcd_cmd(0x80 | (row_addr[row] + col));
}

static void lcd_puts(const char *s)
{
  while (*s)
    lcd_write(*s++, RS);
}

static void put2(unsigned char v)
{
  const char *h = "0123456789abcdef";
  putc(h[v >> 4]);
  putc(h[v & 15]);
}

void main(void)
{
  unsigned char a;
  unsigned int n, v;
  char num[6];

  puts("lcd2004\n");
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  I2C_CTRL = 0;
  I2C_PRESCALE_LO = PRESCALE & 0xff;
  I2C_PRESCALE_HI = PRESCALE >> 8;
  I2C_CTRL = 1;

  PORTC_DIR = 0;
  puts("idle SCL/SDA (port C 1:0): "); put2(PORTC & 3); putc('\n');
  addr = 0;
  puts("ack:");
  for (a = 0x08; a < 0x78; a++)
    if (probe(a)) {
      putc(' '); put2(a);
      if (!addr && ((a >= 0x20 && a <= 0x27) || (a >= 0x38 && a <= 0x3F)))
        addr = a;
    }
  putc('\n');
  puts("during the scan: some high "); put2(lines_or & 3);
  puts(", some low "); put2(~lines_and & 3); putc('\n');
  CHECK(1, addr != 0);
  if (!addr) {
    DONE();
    for (;;)
      ;
  }
  puts("backpack at "); put2(addr); putc('\n');

  /* the HD44780's power-on: 8-bit mode three times, then 4-bit */
  pins(0);
  delay_us(50000);
  nibble(0x30); delay_us(4500);
  nibble(0x30); delay_us(150);
  nibble(0x30);
  nibble(0x20);                         /* 4-bit */
  lcd_cmd(0x28);                        /* 4-bit, 2 lines (4 on a 20x4), 5x8 */
  lcd_cmd(0x08);                        /* display off */
  lcd_cmd(0x01);                        /* clear */
  lcd_cmd(0x06);                        /* increment, no shift */
  lcd_cmd(0x0C);                        /* display on, no cursor */
  CHECK(2, ok);

  lcd_at(0, 0); lcd_puts("LISA rev 3");
  lcd_at(1, 0); lcd_puts("I2C master, mode 3");
  lcd_at(2, 0); lcd_puts("PCF8574 + HD44780");
  lcd_at(3, 0); lcd_puts("count:");
  CHECK(3, ok);

  for (n = 0; n < COUNTS; n++) {
    v = n;
    num[5] = 0;
    for (a = 5; a-- > 0; ) {
      num[a] = (char)('0' + v % 10);
      v /= 10;
    }
    lcd_at(3, 7);
    lcd_puts(num);
    delay_us(20000);
  }
  CHECK(4, ok);
  DONE();
  for (;;)
    ;
}
