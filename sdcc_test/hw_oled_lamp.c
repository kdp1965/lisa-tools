/* hw_oled_lamp.c - is the OLED's panel lit at all?  Every pixel forced on
 * (0xA5: the display ignores its RAM) at full contrast, in turn:
 *   A: display on only (an SSD1309 module makes its own panel voltage)
 *   B: the SSD1306 charge pump turned on first (0x8D 0x14)
 *   C: the SSD1309 command lock released first (0xFD 0x12), then as B
 * each for about 6 s, the step printed; then all off.  The I2C as
 * hw_oled.c (mode 3, uio[4] / uio[5], JD pins 1 / 2 on the Arty).
 * Build: make hw_oled_lamp_rev3.ihx REV3=1; arty_run.py M3=1 CACHE=1. */
#include "harness.h"
#include <tt07.h>

#define STA   0x80
#define STO   0x40
#define WR    0x10
#define RD    0x20
#define NAK   0x08
#define TIP   0x02
#define RXACK 0x80

static unsigned char addr;

static unsigned char i2c_byte(unsigned char b, unsigned char cmd)
{
  I2C_TX_DATA = b;
  I2C_CMD = cmd | WR;
  while (I2C_STATUS & TIP)
    ;
  return !(I2C_STATUS & RXACK);
}

static unsigned char cmds(const unsigned char *p, unsigned char n)
{
  unsigned char ok = i2c_byte(addr << 1, STA);
  ok &= i2c_byte(0x00, 0);
  while (n--)
    ok &= i2c_byte(*p++, n ? 0 : STO);
  return ok;
}

/* the status byte (a read with no control byte): bit 6 set = display off */
static unsigned char status(void)
{
  if (!i2c_byte((addr << 1) | 1, STA))
    return 0xEE;
  I2C_CMD = RD | NAK | STO;
  while (I2C_STATUS & TIP)
    ;
  return I2C_RX_DATA;
}

static void put2(unsigned char v)
{
  const char *h = "0123456789abcdef";
  putc(h[v >> 4]);
  putc(h[v & 15]);
}

static void wait_s(unsigned char s)
{
  unsigned int t, n;
  while (s--)
    for (n = 0; n < 1000; n++) {          /* 1 ms: 781 ticks of 64 clocks */
      t = T3_CNT_L; t |= (unsigned int)T3_CNT_H << 8;
      for (;;) {
        unsigned int c = T3_CNT_L;
        c |= (unsigned int)T3_CNT_H << 8;
        if ((unsigned int)(c - t) >= 781)
          break;
      }
    }
}

static const unsigned char step_a[] = { 0xAE, 0x81, 0xFF, 0xAF, 0xA5 };
static const unsigned char step_b[] = { 0xAE, 0x8D, 0x14, 0x81, 0xFF, 0xAF, 0xA5 };
static const unsigned char step_c[] = { 0xFD, 0x12, 0xAE, 0x8D, 0x14, 0xD9, 0xF1, 0x81, 0xFF, 0xAF, 0xA5 };
static const unsigned char all_off[] = { 0xA4, 0xAE };

void main(void)
{
  unsigned char ok;

  puts("oled lamp test\n");
  T3_PRE = 63; T3_TOP_L = 0xff; T3_TOP_H = 0xff; T3_CTRL = T3_CTRL_EN;
  I2C_CTRL = 0;
  I2C_PRESCALE_LO = 99;                 /* 100 kHz */
  I2C_PRESCALE_HI = 0;
  I2C_CTRL = 1;
  addr = i2c_byte(0x3C << 1, STA | STO) ? 0x3C : 0x3D;
  puts(addr == 0x3C ? "0x3c\n" : "0x3d (or nothing)\n");

  cmds(all_off, sizeof all_off);
  puts("status after AE (off): "); put2(status()); putc('\n');
  ok = cmds(step_a, sizeof step_a);
  puts("status after AF (on):  "); put2(status()); putc('\n');
  puts("A: display on, all pixels on: "); puts(ok ? "acked\n" : "NOT acked\n");
  wait_s(6);
  ok = cmds(step_b, sizeof step_b);
  puts("B: + charge pump, all pixels on: "); puts(ok ? "acked\n" : "NOT acked\n");
  wait_s(6);
  ok = cmds(step_c, sizeof step_c);
  puts("C: + unlock, precharge F1, all pixels on: "); puts(ok ? "acked\n" : "NOT acked\n");
  wait_s(6);
  cmds(all_off, sizeof all_off);
  puts("off\n");
  DONE();
  for (;;)
    ;
}
