/*
 * monitor.c - a small interactive monitor for the LISA core on the TT07
 * chip, compiled with sdcc -mlisa --tt07-cache (the 32K data space behind
 * the data cache: data below 0x3800, the stack at 0x7fff).
 *
 * Talks on UART1 (the debug UART the demo board forwards).  The first
 * Enter prints the bring-up banner, then "lisa> " waits for commands:
 *
 *   help                this list
 *   mr addr [len]       dump len (16) bytes of data RAM
 *   mw addr v [v ...]   write bytes from addr on
 *   stack [depth]       SP and the depth (32) bytes above it
 *   free                data end, the 0x3800 limit, the stack's use
 *   banner              the owl again
 *
 * Numbers are hex, with or without 0x.  Backspace edits the line.
 */
#include <tt07.h>

/* ---- the UART ------------------------------------------------------- */

/* Output goes through a queue that is pumped while waiting for input, so
   that echoing a character never blocks the receive loop: a burst of
   input (a pasted line) arrives every 87 us at 115200 baud, which is how
   long one echoed character takes, and the receiver holds one byte. */
/* Everything the receive loop touches lives in one 64-byte block: the
   data cache has four 16-byte lines for the lower half of memory, indexed
   by address bits 5:4, so a block this size stays resident and a
   character never costs a line fill (50 us on the SPI RAM, against the
   87 us between characters). */
#define OQ 16
static struct {
  char line[40];
  char oq[OQ];
  unsigned char oq_head, oq_tail;
  unsigned char lo_byte, hi_byte;
} g;
#define line g.line
#define oq g.oq
#define oq_head g.oq_head
#define oq_tail g.oq_tail
#define lo_byte g.lo_byte
#define hi_byte g.hi_byte

static void oq_pump(void)
{
  if (oq_head != oq_tail && (UART1_STATUS & UART_TX_EMPTY)) {
    UART1_DATA = oq[oq_tail];
    oq_tail = (oq_tail + 1) & (OQ - 1);
  }
}

static void putc(char c)
{
  unsigned char next = (oq_head + 1) & (OQ - 1);
  while (next == oq_tail)
    oq_pump();
  oq[oq_head] = c;
  oq_head = next;
}

static void puts(const char *s)
{
  while (*s)
    putc(*s++);
}

static void putnl(void)
{
  putc('\r');
  putc('\n');
}

static char getc(void)
{
  while (!(UART1_STATUS & UART_RX_AVAIL))
    oq_pump();
  return UART1_DATA;
}

static void puthex2(unsigned char v)
{
  static const char hex[] = "0123456789abcdef";
  putc(hex[v >> 4]);
  putc(hex[v & 15]);
}

static void puthex4(unsigned int v)
{
  puthex2(v >> 8);
  puthex2((unsigned char)v);
}

static void putdec(unsigned int v)
{
  char buf[6];
  unsigned char n = 0;
  do {
    buf[n++] = '0' + v % 10;
    v /= 10;
  } while (v);
  while (n)
    putc(buf[--n]);
}

/* ---- the banner (lisa_bringup's) ------------------------------------ */

static const char *const owl[] = {
  "     .{{{}}}}}}.",
  "    {{{{{}}}}}}}.",
  "   {{{{  {{{{{}}}}",
  "  }}}}} _   _ {{{{{",
  "  }}}}  6   6  }}}}",
  " {{{{C    ^    {{{{",
  "}}}}}}\\  '='  /}}}}}",
  "{{{{{{{;.___.;}}}}}}",
  " {{{{{{{)   (}}}}}}'",
  "  ''\"''':   :''''''",
  "  jgs    `@` ",
  0,
};

static void banner(void)
{
  const char *const *p;
  putnl();
  for (p = owl; *p; p++) {
    puts(*p);
    putnl();
  }
  putnl();
  puts("Hello from TT07 LISA!");
  putnl();
  puts("lisa_monitor, sdcc -mlisa --tt07-cache: type help");
  putnl();
}

/* ---- what the linker and the core know ------------------------------ */

#define DATA_LIMIT 0x3800u          /* --tt07-cache, 2K stack: the stack's alias */
#define STACK_TOP  0x7fffu

/* the end of the data areas: s_FINITIALIZED + l_FINITIALIZED (the last area) */
static unsigned int data_end(void)
{
  unsigned int s, l;
  __asm
    ldi  #<s_FINITIALIZED
    sta  _g + 58
    ldi  #>s_FINITIALIZED
    sta  _g + 59
  __endasm;
  s = ((unsigned int)hi_byte << 8) | lo_byte;
  __asm
    ldi  #<l_FINITIALIZED
    sta  _g + 58
    ldi  #>l_FINITIALIZED
    sta  _g + 59
  __endasm;
  l = ((unsigned int)hi_byte << 8) | lo_byte;
  return s + l;
}

static unsigned int get_sp(void)
{
  __asm
    spix
    txa
    sta  _g + 58
    txau
    andi #0x7f
    sta  _g + 59
  __endasm;
  return ((unsigned int)hi_byte << 8) | lo_byte;
}

/* ---- the line and its words ----------------------------------------- */

static void readline(void)
{
  unsigned char n = 0;
  char c;
  for (;;) {
    c = getc();
    if (c == '\r' || c == '\n') {
      line[n] = 0;
      putnl();
      return;
    }
    if (c == 8 || c == 127) {
      if (n) {
        n--;
        puts("\b \b");
      }
      continue;
    }
    if (c < ' ')
      continue;
    if (n < sizeof line - 1) {
      line[n++] = c;
      putc(c);
    }
  }
}

static char *skip(char *p)
{
  while (*p == ' ')
    p++;
  return p;
}

/* a hex number at *pp; returns 0 if there is none */
static unsigned char number(char **pp, unsigned int *v)
{
  char *p = skip(*pp);
  unsigned int r = 0;
  unsigned char any = 0;
  if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
    p += 2;
  for (;; p++) {
    char c = *p;
    unsigned char d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
    else break;
    r = (r << 4) | d;
    any = 1;
  }
  if (!any || (*p && *p != ' '))
    return 0;
  *pp = p;
  *v = r;
  return 1;
}

static unsigned char word_is(const char *p, const char *w)
{
  while (*w)
    if (*p++ != *w++)
      return 0;
  return *p == 0 || *p == ' ';
}

/* ---- the commands --------------------------------------------------- */

static void dump(unsigned int addr, unsigned int len)
{
  while (len) {
    unsigned char i, n = len > 16 ? 16 : len;
    puthex4(addr);
    puts(": ");
    for (i = 0; i < n; i++) {
      puthex2(*(volatile unsigned char *)(addr + i));
      putc(' ');
    }
    for (i = n; i < 16; i++)
      puts("   ");
    putc('|');
    for (i = 0; i < n; i++) {
      unsigned char c = *(volatile unsigned char *)(addr + i);
      putc(c >= ' ' && c < 127 ? c : '.');
    }
    putc('|');
    putnl();
    addr += n;
    len -= n;
  }
}

static void cmd_mr(char *p)
{
  unsigned int addr, len = 16;
  if (!number(&p, &addr)) {
    puts("mr addr [len]");
    putnl();
    return;
  }
  number(&p, &len);
  dump(addr, len);
}

static void cmd_mw(char *p)
{
  unsigned int addr, v;
  unsigned char n = 0;
  if (!number(&p, &addr)) {
    puts("mw addr value [value ...]");
    putnl();
    return;
  }
  while (number(&p, &v)) {
    *(volatile unsigned char *)(addr + n) = (unsigned char)v;
    n++;
  }
  if (!n) {
    puts("mw addr value [value ...]");
    putnl();
    return;
  }
  dump(addr, n);
}

static void cmd_stack(char *p)
{
  unsigned int depth = 32, sp = get_sp();
  number(&p, &depth);
  puts("SP ");
  puthex4(sp);
  puts(" (");
  putdec(STACK_TOP - sp);
  puts(" bytes in use); above it:");
  putnl();
  if (depth > STACK_TOP - sp)
    depth = STACK_TOP - sp;
  dump(sp + 1, depth);
}

static void cmd_free(void)
{
  unsigned int end = data_end(), sp = get_sp();
  puts("data  0000-");
  puthex4(end - 1);
  puts(" (");
  putdec(end);
  puts(" bytes of globals)");
  putnl();
  puts("free  ");
  puthex4(end);
  puts("-");
  puthex4(DATA_LIMIT - 1);
  puts(" (");
  putdec(DATA_LIMIT - end);
  puts(" bytes up to the stack's alias at ");
  puthex4(DATA_LIMIT);
  putc(')');
  putnl();
  puts("stack ");
  puthex4(STACK_TOP);
  puts(" down, SP ");
  puthex4(sp);
  puts(" (");
  putdec(STACK_TOP - sp);
  puts(" bytes in use of 2048)");
  putnl();
}

static void cmd_help(void)
{
  puts("help               this list");            putnl();
  puts("mr addr [len]      dump data RAM (hex, 16 bytes)"); putnl();
  puts("mw addr v [v ...]  write bytes");            putnl();
  puts("stack [depth]      SP and the bytes above it"); putnl();
  puts("free               data end, limit 3800, stack use"); putnl();
  puts("banner             the owl");               putnl();
}

void main(void)
{
  unsigned char shown = 0;
  char *p;

  for (;;) {
    if (shown)
      puts("lisa> ");
    readline();
    if (!shown) {
      banner();
      shown = 1;
      continue;
    }
    p = skip(line);
    if (!*p)
      continue;
    if (word_is(p, "help") || word_is(p, "?"))
      cmd_help();
    else if (word_is(p, "mr"))
      cmd_mr(p + 2);
    else if (word_is(p, "mw"))
      cmd_mw(p + 2);
    else if (word_is(p, "stack"))
      cmd_stack(p + 5);
    else if (word_is(p, "free"))
      cmd_free();
    else if (word_is(p, "banner"))
      banner();
    else {
      puts("? ");
      puts(p);
      puts(" (help)");
      putnl();
    }
  }
}
